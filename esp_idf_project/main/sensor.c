/**
 * 심박 센서 모듈 구현 (MAX30102 기반)
 *
 * I2C 버스를 공유하여 터치 컨트롤러와 함께 동작합니다.
 * FIFO 데이터를 읽어 간단한 피크 감지를 수행하고 BPM을 계산합니다.
 */

#include "sensor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <inttypes.h>

// MAX30102 기본 설정
#define MAX30102_I2C_PORT        I2C_NUM_0
#define MAX30102_ADDR            0x57

// 레지스터 주소
#define REG_INTR_STATUS_1        0x00
#define REG_INTR_STATUS_2        0x01
#define REG_INTR_ENABLE_1        0x02
#define REG_INTR_ENABLE_2        0x03
#define REG_FIFO_WR_PTR          0x04
#define REG_OVF_COUNTER          0x05
#define REG_FIFO_RD_PTR          0x06
#define REG_FIFO_DATA            0x07
#define REG_FIFO_CONFIG          0x08
#define REG_MODE_CONFIG          0x09
#define REG_SPO2_CONFIG          0x0A
#define REG_LED1_PA              0x0C
#define REG_LED2_PA              0x0D
#define REG_PILOT_PA             0x10
#define REG_PART_ID              0xFF

#define INT_PPG_READY            0x40
#define INT_FIFO_FULL            0x80

#define MAX_FIFO_SAMPLES         32

static const char *TAG = "HEART_SENSOR";

static TaskHandle_t s_sensor_task = NULL;
static SemaphoreHandle_t s_data_mutex = NULL;
static uint16_t s_latest_bpm = 0;
static heart_rate_sensor_cb_t s_data_callback = NULL;
static void *s_callback_ctx = NULL;

// 피크 검출용 내부 상태
static uint32_t s_dc_estimate = 0;
static uint32_t s_last_ir_value = 0;
static bool s_prev_above_threshold = false;
static uint64_t s_last_peak_time_us = 0;
static uint16_t s_bpm_buffer[8] = {0};
static size_t s_bpm_index = 0;
static size_t s_bpm_count = 0;

// ---------------------------------------------------------------------------
// 내부 유틸리티
// ---------------------------------------------------------------------------

static esp_err_t write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(MAX30102_I2C_PORT, MAX30102_ADDR, buf, sizeof(buf), pdMS_TO_TICKS(50));
}

static esp_err_t read_reg(uint8_t reg, uint8_t *out_value)
{
    return i2c_master_write_read_device(MAX30102_I2C_PORT, MAX30102_ADDR, &reg, 1, out_value, 1, pdMS_TO_TICKS(50));
}

static esp_err_t read_regs(uint8_t reg, uint8_t *buffer, size_t len)
{
    return i2c_master_write_read_device(MAX30102_I2C_PORT, MAX30102_ADDR, &reg, 1, buffer, len, pdMS_TO_TICKS(50));
}

static esp_err_t max30102_reset(void)
{
    esp_err_t ret = write_reg(REG_MODE_CONFIG, 0x40);  // Reset bit set
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Reset 완료 대기
    const uint8_t retries = 20;
    for (uint8_t i = 0; i < retries; ++i)
    {
        uint8_t mode = 0;
        ret = read_reg(REG_MODE_CONFIG, &mode);
        if (ret == ESP_OK && !(mode & 0x40))
        {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return ESP_ERR_TIMEOUT;
}

static esp_err_t max30102_shutdown(bool enable)
{
    uint8_t mode = 0;
    esp_err_t ret = read_reg(REG_MODE_CONFIG, &mode);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (enable)
    {
        mode |= 0x80;  // SHDN bit
    }
    else
    {
        mode &= ~0x80;
    }

    return write_reg(REG_MODE_CONFIG, mode);
}

static esp_err_t max30102_clear_fifo(void)
{
    esp_err_t ret = write_reg(REG_FIFO_WR_PTR, 0x00);
    if (ret != ESP_OK) return ret;
    ret = write_reg(REG_OVF_COUNTER, 0x00);
    if (ret != ESP_OK) return ret;
    return write_reg(REG_FIFO_RD_PTR, 0x00);
}

static esp_err_t max30102_configure(void)
{
    // 인터럽트: PPG ready, FIFO full
    esp_err_t ret = write_reg(REG_INTR_ENABLE_1, INT_PPG_READY | INT_FIFO_FULL);
    if (ret != ESP_OK) return ret;
    ret = write_reg(REG_INTR_ENABLE_2, 0x00);
    if (ret != ESP_OK) return ret;

    // FIFO 설정: sample averaging 4, FIFO 롤오버 비활성, almost full = 32-4
    ret = write_reg(REG_FIFO_CONFIG, (0x02 << 5) | (0x0F)); // average 4 samples, almost full = 15
    if (ret != ESP_OK) return ret;

    // SPO2 설정: LED 펄스 폭 411us (18bit), 샘플 레이트 100Hz, 범위 4096nA
    ret = write_reg(REG_SPO2_CONFIG, (0x03 << 5) | (0x03 << 2) | 0x03);
    if (ret != ESP_OK) return ret;

    // LED 파워 설정 (적색/IR)
    ret = write_reg(REG_LED1_PA, 0x24); // ~7.6mA
    if (ret != ESP_OK) return ret;
    ret = write_reg(REG_LED2_PA, 0x24); // ~7.6mA
    if (ret != ESP_OK) return ret;
    ret = write_reg(REG_PILOT_PA, 0x7F); // 최대(테스트용)
    if (ret != ESP_OK) return ret;

    // FIFO 초기화
    ret = max30102_clear_fifo();
    if (ret != ESP_OK) return ret;

    // 모드 설정: SpO2 모드 (적색+IR)
    ret = write_reg(REG_MODE_CONFIG, 0x03);
    if (ret != ESP_OK) return ret;

    // 상태 레지스터 읽어서 클리어
    uint8_t dummy = 0;
    read_reg(REG_INTR_STATUS_1, &dummy);
    read_reg(REG_INTR_STATUS_2, &dummy);

    return ESP_OK;
}

static esp_err_t max30102_read_sample(uint32_t *ir_out)
{
    uint8_t buffer[6] = {0};
    esp_err_t ret = read_regs(REG_FIFO_DATA, buffer, sizeof(buffer));
    if (ret != ESP_OK)
    {
        return ret;
    }

    // FIFO 데이터: RED(3바이트) + IR(3바이트)
    uint32_t ir_raw = ((uint32_t)buffer[3] << 16) | ((uint32_t)buffer[4] << 8) | buffer[5];
    ir_raw &= 0x3FFFF; // 상위 18비트만 사용

    *ir_out = ir_raw;
    return ESP_OK;
}

static void update_latest_bpm(uint16_t bpm)
{
    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        s_latest_bpm = bpm;
        xSemaphoreGive(s_data_mutex);
    }

    if (s_data_callback)
    {
        s_data_callback(bpm, s_callback_ctx);
    }
}

static uint16_t smooth_bpm(uint16_t bpm)
{
    s_bpm_buffer[s_bpm_index] = bpm;
    s_bpm_index = (s_bpm_index + 1) % (sizeof(s_bpm_buffer) / sizeof(s_bpm_buffer[0]));

    if (s_bpm_count < sizeof(s_bpm_buffer) / sizeof(s_bpm_buffer[0]))
    {
        s_bpm_count++;
    }

    uint32_t sum = 0;
    for (size_t i = 0; i < s_bpm_count; ++i)
    {
        sum += s_bpm_buffer[i];
    }

    return (uint16_t)(sum / s_bpm_count);
}

static void process_sample(uint32_t ir_value)
{
    // DC 성분 추정 (LPF) 및 기준 업데이트
    if (s_dc_estimate == 0)
    {
        s_dc_estimate = ir_value;
    }
    else
    {
        s_dc_estimate = (s_dc_estimate * 31 + ir_value) / 32;
    }

    uint32_t ac_component = (ir_value > s_dc_estimate) ? (ir_value - s_dc_estimate) : 0;
    uint32_t dynamic_threshold = s_dc_estimate / 18; // 약 5% 수준

    bool above_threshold = ac_component > dynamic_threshold && ir_value > s_last_ir_value;
    uint64_t now_us = esp_timer_get_time();

    if (above_threshold && !s_prev_above_threshold)
    {
        if (s_last_peak_time_us != 0)
        {
            uint64_t interval_us = now_us - s_last_peak_time_us;
            if (interval_us > 250000 && interval_us < 2000000) // 30~240 BPM 범위
            {
                uint16_t bpm = (uint16_t)(60000000ULL / interval_us);
                uint16_t filtered = smooth_bpm(bpm);
                ESP_LOGD(TAG, "Peak detected: raw=%" PRIu32 ", bpm=%u, filtered=%u", ir_value, bpm, filtered);
                update_latest_bpm(filtered);
            }
        }

        s_last_peak_time_us = now_us;
    }

    s_prev_above_threshold = above_threshold;
    s_last_ir_value = ir_value;
}

static void sensor_task(void *arg)
{
    ESP_LOGI(TAG, "심박 센서 태스크 시작");

    while (1)
    {
        uint8_t int_status = 0;
        if (read_reg(REG_INTR_STATUS_1, &int_status) == ESP_OK)
        {
            if (int_status & INT_FIFO_FULL)
            {
                ESP_LOGW(TAG, "MAX30102 FIFO overflow");
                max30102_clear_fifo();
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            if (int_status & INT_PPG_READY)
            {
                uint8_t wr_ptr = 0, rd_ptr = 0;
                if (read_reg(REG_FIFO_WR_PTR, &wr_ptr) == ESP_OK &&
                    read_reg(REG_FIFO_RD_PTR, &rd_ptr) == ESP_OK)
                {
                    uint8_t sample_count = (wr_ptr - rd_ptr) & 0x1F;
                    if (sample_count == 0)
                    {
                        // 포인터가 같은 경우에도 새 데이터가 있을 수 있으므로 최소 1개는 읽어본다.
                        sample_count = 1;
                    }

                    if (sample_count > MAX_FIFO_SAMPLES)
                    {
                        sample_count = MAX_FIFO_SAMPLES;
                    }

                    for (uint8_t i = 0; i < sample_count; ++i)
                    {
                        uint32_t ir_value = 0;
                        esp_err_t ret = max30102_read_sample(&ir_value);
                        if (ret == ESP_OK)
                        {
                            process_sample(ir_value);
                        }
                        else
                        {
                            ESP_LOGW(TAG, "FIFO 샘플 읽기 실패: %s", esp_err_to_name(ret));
                            break;
                        }
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 약 50Hz 주기
    }
}

// ---------------------------------------------------------------------------
// 공개 API
// ---------------------------------------------------------------------------

esp_err_t heart_rate_sensor_init(void)
{
    if (!s_data_mutex)
    {
        s_data_mutex = xSemaphoreCreateMutex();
        if (!s_data_mutex)
        {
            ESP_LOGE(TAG, "심박 센서 뮤텍스 생성 실패");
            return ESP_ERR_NO_MEM;
        }
    }

    esp_err_t ret = max30102_reset();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "MAX30102 리셋 실패: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = max30102_configure();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "MAX30102 구성 실패: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = max30102_shutdown(false);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "MAX30102 기동 실패: %s", esp_err_to_name(ret));
        return ret;
    }

    uint8_t part_id = 0;
    ret = read_reg(REG_PART_ID, &part_id);
    if (ret != ESP_OK || part_id != 0x15)
    {
        ESP_LOGE(TAG, "MAX30102 Part ID 불일치 (읽은 값: 0x%02X)", part_id);
        return (ret == ESP_OK) ? ESP_ERR_INVALID_RESPONSE : ret;
    }
    ESP_LOGI(TAG, "MAX30102 Part ID 확인: 0x%02X", part_id);

    s_dc_estimate = 0;
    s_last_ir_value = 0;
    s_prev_above_threshold = false;
    s_last_peak_time_us = 0;
    s_bpm_index = 0;
    s_bpm_count = 0;
    memset(s_bpm_buffer, 0, sizeof(s_bpm_buffer));
    update_latest_bpm(0);

    ESP_LOGI(TAG, "심박 센서 초기화 완료");
    return ESP_OK;
}

esp_err_t heart_rate_sensor_start(void)
{
    if (s_sensor_task)
    {
        ESP_LOGW(TAG, "심박 센서 태스크가 이미 실행 중입니다");
        return ESP_OK;
    }

    esp_err_t ret = max30102_shutdown(false);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "MAX30102 전원 온 실패: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = max30102_clear_fifo();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "MAX30102 FIFO 초기화 실패: %s", esp_err_to_name(ret));
        return ret;
    }

    BaseType_t task_ret = xTaskCreate(sensor_task, "heart_rate_sensor", 4096, NULL, 4, &s_sensor_task);
    if (task_ret != pdPASS)
    {
        ESP_LOGE(TAG, "심박 센서 태스크 생성 실패");
        s_sensor_task = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "심박 센서 태스크가 시작되었습니다");
    return ESP_OK;
}

void heart_rate_sensor_stop(void)
{
    if (s_sensor_task)
    {
        vTaskDelete(s_sensor_task);
        s_sensor_task = NULL;
    }

    max30102_shutdown(true);
    s_prev_above_threshold = false;
    s_last_peak_time_us = 0;
    s_dc_estimate = 0;
    s_last_ir_value = 0;
    s_bpm_index = 0;
    s_bpm_count = 0;
    memset(s_bpm_buffer, 0, sizeof(s_bpm_buffer));
    update_latest_bpm(0);
}

uint16_t heart_rate_sensor_get_bpm(void)
{
    uint16_t bpm = 0;
    if (!s_data_mutex)
    {
        return 0;
    }
    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        bpm = s_latest_bpm;
        xSemaphoreGive(s_data_mutex);
    }
    return bpm;
}

esp_err_t heart_rate_sensor_register_callback(heart_rate_sensor_cb_t callback, void *ctx)
{
    s_data_callback = callback;
    s_callback_ctx = ctx;
    return ESP_OK;
}
