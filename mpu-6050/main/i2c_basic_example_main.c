/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* i2c - MPU6050 예제

   I2C를 초기화하고 센서 레지스터를 읽고 쓰는 방법을 보여주는 간단한 예제입니다.

   이 예제에서 사용하는 센서는 MPU6050 6축 관성 측정 장치입니다.
   - 3축 가속도계 (X, Y, Z)
   - 3축 자이로스코프 (X, Y, Z)
   - 온도 센서
*/
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

static const char *TAG = "example";

// I2C 설정
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL        /*!< I2C 클럭 핀 (SCL) */
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA        /*!< I2C 데이터 핀 (SDA) */
#define I2C_MASTER_NUM I2C_NUM_0                       /*!< I2C 포트 번호 */
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQUENCY /*!< I2C 클럭 주파수 */
#define I2C_MASTER_TX_BUF_DISABLE 0                    /*!< I2C 송신 버퍼 비활성화 */
#define I2C_MASTER_RX_BUF_DISABLE 0                    /*!< I2C 수신 버퍼 비활성화 */
#define I2C_MASTER_TIMEOUT_MS 1000                     /*!< I2C 타임아웃 (밀리초) */

// MPU6050 레지스터 주소
#define MPU6050_SENSOR_ADDR 0x68         /*!< MPU6050 I2C 주소 (AD0=GND일 때) */
#define MPU6050_WHO_AM_I_REG_ADDR 0x75   /*!< WHO_AM_I 레지스터 (센서 ID 확인용) */
#define MPU6050_PWR_MGMT_1_REG_ADDR 0x6B /*!< 전원 관리 레지스터 */
#define MPU6050_RESET_BIT 7              /*!< 리셋 비트 위치 */
#define MPU6050_ACCEL_XOUT_H 0x3B        /*!< 가속도계 X축 상위 바이트 시작 주소 */
#define MPU6050_GYRO_XOUT_H 0x43         /*!< 자이로스코프 X축 상위 바이트 시작 주소 */
#define MPU6050_TEMP_OUT_H 0x41          /*!< 온도 센서 상위 바이트 */
#define MPU6050_ACCEL_CONFIG_REG 0x1C    /*!< 가속도계 범위 설정 레지스터 */
#define MPU6050_GYRO_CONFIG_REG 0x1B     /*!< 자이로스코프 범위 설정 레지스터 */

#define CALIBRATION_SAMPLES 200 /*!< 보정을 위한 샘플 개수 */

// 가속도계 측정 범위
typedef enum
{
    ACCEL_RANGE_2G = 0x00, // ±2g  (감도: 16384 LSB/g)
    ACCEL_RANGE_4G = 0x08, // ±4g  (감도: 8192 LSB/g)
    ACCEL_RANGE_8G = 0x10, // ±8g  (감도: 4096 LSB/g)
    ACCEL_RANGE_16G = 0x18 // ±16g (감도: 2048 LSB/g)
} mpu6050_accel_range_t;

// 자이로스코프 측정 범위
typedef enum
{
    GYRO_RANGE_250 = 0x00,  // ±250°/s  (감도: 131 LSB/°/s)
    GYRO_RANGE_500 = 0x08,  // ±500°/s  (감도: 65.5 LSB/°/s)
    GYRO_RANGE_1000 = 0x10, // ±1000°/s (감도: 32.8 LSB/°/s)
    GYRO_RANGE_2000 = 0x18  // ±2000°/s (감도: 16.4 LSB/°/s)
} mpu6050_gyro_range_t;

// 보정 오프셋 구조체
typedef struct
{
    int16_t accel_x_offset; // 가속도계 X축 오프셋
    int16_t accel_y_offset; // 가속도계 Y축 오프셋
    int16_t accel_z_offset; // 가속도계 Z축 오프셋
    int16_t gyro_x_offset;  // 자이로 X축 오프셋
    int16_t gyro_y_offset;  // 자이로 Y축 오프셋
    int16_t gyro_z_offset;  // 자이로 Z축 오프셋
} mpu6050_calibration_t;

/**
 * @brief MPU6050 레지스터에서 데이터 읽기
 *
 * @param dev_handle I2C 디바이스 핸들
 * @param reg_addr 읽을 레지스터 주소
 * @param data 읽은 데이터를 저장할 버퍼
 * @param len 읽을 바이트 수
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
static esp_err_t mpu6050_register_read(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS);
}

/**
 * @brief MPU6050 레지스터에 1바이트 쓰기
 *
 * @param dev_handle I2C 디바이스 핸들
 * @param reg_addr 쓸 레지스터 주소
 * @param data 쓸 데이터 (1바이트)
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
static esp_err_t mpu6050_register_write_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);
}

/**
 * @brief 가속도계 측정 범위 설정
 *
 * @param dev_handle I2C 디바이스 핸들
 * @param range 가속도계 측정 범위
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
static esp_err_t mpu6050_set_accel_range(i2c_master_dev_handle_t dev_handle, mpu6050_accel_range_t range)
{
    return mpu6050_register_write_byte(dev_handle, MPU6050_ACCEL_CONFIG_REG, range);
}

/**
 * @brief 자이로스코프 측정 범위 설정
 *
 * @param dev_handle I2C 디바이스 핸들
 * @param range 자이로스코프 측정 범위
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
static esp_err_t mpu6050_set_gyro_range(i2c_master_dev_handle_t dev_handle, mpu6050_gyro_range_t range)
{
    return mpu6050_register_write_byte(dev_handle, MPU6050_GYRO_CONFIG_REG, range);
}

/**
 * @brief 가속도계 감도 계산 (LSB/g)
 *
 * @param range 가속도계 측정 범위
 * @return float 감도 값
 */
static float mpu6050_get_accel_sensitivity(mpu6050_accel_range_t range)
{
    switch (range)
    {
    case ACCEL_RANGE_2G:
        return 16384.0;
    case ACCEL_RANGE_4G:
        return 8192.0;
    case ACCEL_RANGE_8G:
        return 4096.0;
    case ACCEL_RANGE_16G:
        return 2048.0;
    default:
        return 16384.0;
    }
}

/**
 * @brief 자이로스코프 감도 계산 (LSB/°/s)
 *
 * @param range 자이로스코프 측정 범위
 * @return float 감도 값
 */
static float mpu6050_get_gyro_sensitivity(mpu6050_gyro_range_t range)
{
    switch (range)
    {
    case GYRO_RANGE_250:
        return 131.0;
    case GYRO_RANGE_500:
        return 65.5;
    case GYRO_RANGE_1000:
        return 32.8;
    case GYRO_RANGE_2000:
        return 16.4;
    default:
        return 131.0;
    }
}

/**
 * @brief MPU6050 센서 초기화
 *
 * @param dev_handle I2C 디바이스 핸들
 * @param accel_range 가속도계 측정 범위
 * @param gyro_range 자이로스코프 측정 범위
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
static esp_err_t mpu6050_init(i2c_master_dev_handle_t dev_handle, mpu6050_accel_range_t accel_range,
                              mpu6050_gyro_range_t gyro_range)
{
    esp_err_t ret;

    // MPU6050 깨우기 (슬립 비트 해제)
    ret = mpu6050_register_write_byte(dev_handle, MPU6050_PWR_MGMT_1_REG_ADDR, 0x00);
    if (ret != ESP_OK)
    {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // 센서가 안정화될 때까지 대기

    // 가속도계 범위 설정
    ret = mpu6050_set_accel_range(dev_handle, accel_range);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "가속도계 범위 설정 실패");
        return ret;
    }

    // 자이로스코프 범위 설정
    ret = mpu6050_set_gyro_range(dev_handle, gyro_range);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "자이로스코프 범위 설정 실패");
        return ret;
    }

    ESP_LOGI(TAG, "가속도계 범위: ±%dg, 자이로 범위: ±%d°/s 설정 완료",
             (accel_range == ACCEL_RANGE_2G)   ? 2
             : (accel_range == ACCEL_RANGE_4G) ? 4
             : (accel_range == ACCEL_RANGE_8G) ? 8
                                               : 16,
             (gyro_range == GYRO_RANGE_250)    ? 250
             : (gyro_range == GYRO_RANGE_500)  ? 500
             : (gyro_range == GYRO_RANGE_1000) ? 1000
                                               : 2000);

    return ESP_OK;
}

/**
 * @brief 가속도계와 자이로스코프 데이터 읽기
 *
 * @param dev_handle I2C 디바이스 핸들
 * @param accel_x 가속도계 X축 값 저장 포인터
 * @param accel_y 가속도계 Y축 값 저장 포인터
 * @param accel_z 가속도계 Z축 값 저장 포인터
 * @param gyro_x 자이로 X축 값 저장 포인터
 * @param gyro_y 자이로 Y축 값 저장 포인터
 * @param gyro_z 자이로 Z축 값 저장 포인터
 * @param temp 온도 값 저장 포인터
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
static esp_err_t mpu6050_read_sensor_data(i2c_master_dev_handle_t dev_handle, int16_t *accel_x, int16_t *accel_y,
                                          int16_t *accel_z, int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                                          int16_t *temp)
{
    uint8_t data[14];

    // 모든 센서 데이터를 한 번에 읽기 (가속도계 + 온도 + 자이로)
    esp_err_t ret = mpu6050_register_read(dev_handle, MPU6050_ACCEL_XOUT_H, data, 14);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // 16비트 값으로 변환
    *accel_x = (int16_t)((data[0] << 8) | data[1]);
    *accel_y = (int16_t)((data[2] << 8) | data[3]);
    *accel_z = (int16_t)((data[4] << 8) | data[5]);
    *temp = (int16_t)((data[6] << 8) | data[7]);
    *gyro_x = (int16_t)((data[8] << 8) | data[9]);
    *gyro_y = (int16_t)((data[10] << 8) | data[11]);
    *gyro_z = (int16_t)((data[12] << 8) | data[13]);

    return ESP_OK;
}

/**
 * @brief MPU6050 센서 보정
 * @note 보정 전에 센서를 평평하고 안정적인 표면에 놓으세요
 *
 * @param dev_handle I2C 디바이스 핸들
 * @param cal 보정 오프셋을 저장할 구조체 포인터
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
static esp_err_t mpu6050_calibrate(i2c_master_dev_handle_t dev_handle, mpu6050_calibration_t *cal)
{
    ESP_LOGI(TAG, "보정 시작... 센서를 평평한 곳에 가만히 두세요!");

    int32_t accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
    int32_t gyro_x_sum = 0, gyro_y_sum = 0, gyro_z_sum = 0;
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t temp;

    // 샘플 수집
    for (int i = 0; i < CALIBRATION_SAMPLES; i++)
    {
        esp_err_t ret =
            mpu6050_read_sensor_data(dev_handle, &accel_x, &accel_y, &accel_z, &gyro_x, &gyro_y, &gyro_z, &temp);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "보정 실패: 샘플 %d", i);
            return ret;
        }

        accel_x_sum += accel_x;
        accel_y_sum += accel_y;
        accel_z_sum += accel_z;
        gyro_x_sum += gyro_x;
        gyro_y_sum += gyro_y;
        gyro_z_sum += gyro_z;

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // 평균 계산
    // 주의: Z축 오프셋은 현재 사용 중인 가속도계 범위의 1g 감도 값을 빼야 함
    // 기본값: 16384 (±2g 범위)
    cal->accel_x_offset = accel_x_sum / CALIBRATION_SAMPLES;
    cal->accel_y_offset = accel_y_sum / CALIBRATION_SAMPLES;
    cal->accel_z_offset = (accel_z_sum / CALIBRATION_SAMPLES) - 16384; // 중력 1g 보상
    cal->gyro_x_offset = gyro_x_sum / CALIBRATION_SAMPLES;
    cal->gyro_y_offset = gyro_y_sum / CALIBRATION_SAMPLES;
    cal->gyro_z_offset = gyro_z_sum / CALIBRATION_SAMPLES;

    ESP_LOGI(TAG, "보정 완료!");
    ESP_LOGI(TAG, "가속도계 오프셋: X=%d Y=%d Z=%d", cal->accel_x_offset, cal->accel_y_offset, cal->accel_z_offset);
    ESP_LOGI(TAG, "자이로 오프셋:   X=%d Y=%d Z=%d", cal->gyro_x_offset, cal->gyro_y_offset, cal->gyro_z_offset);

    return ESP_OK;
}

/**
 * @brief 센서 값에 보정 적용
 *
 * @param cal 보정 오프셋 구조체 포인터
 * @param accel_x 가속도계 X축 값 포인터
 * @param accel_y 가속도계 Y축 값 포인터
 * @param accel_z 가속도계 Z축 값 포인터
 * @param gyro_x 자이로 X축 값 포인터
 * @param gyro_y 자이로 Y축 값 포인터
 * @param gyro_z 자이로 Z축 값 포인터
 */
static void mpu6050_apply_calibration(mpu6050_calibration_t *cal, int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                                      int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
    *accel_x -= cal->accel_x_offset;
    *accel_y -= cal->accel_y_offset;
    *accel_z -= cal->accel_z_offset;
    *gyro_x -= cal->gyro_x_offset;
    *gyro_y -= cal->gyro_y_offset;
    *gyro_z -= cal->gyro_z_offset;
}

/**
 * @brief 메인 애플리케이션 함수
 */
void app_main(void)
{
    uint8_t data[2];
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;

    // I2C 버스 먼저 초기화 (디바이스 없이)
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    ESP_LOGI(TAG, "I2C 버스 초기화 완료: SDA=%d, SCL=%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    // MPU6050 디바이스 추가
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

    /* MPU6050 WHO_AM_I 레지스터 읽기 (전원 인가 시 0x68 또는 0x72 값을 가져야 함) */
    esp_err_t ret = mpu6050_register_read(dev_handle, MPU6050_WHO_AM_I_REG_ADDR, data, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WHO_AM_I 레지스터 읽기 실패: %s (0x%x)", esp_err_to_name(ret), ret);
        ESP_LOGE(TAG, "확인 사항:");
        ESP_LOGE(TAG, "  - MPU6050이 SCL=%d, SDA=%d에 연결되어 있는지", I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO);
        ESP_LOGE(TAG, "  - MPU6050 VCC와 GND가 연결되어 있는지");
        ESP_LOGE(TAG, "  - I2C 주소가 올바른지 (AD0 핀에 따라 0x68 또는 0x69)");
        goto cleanup;
    }

    ESP_LOGI(TAG, "WHO_AM_I = 0x%X", data[0]);

    // 측정 범위 설정 (필요에 따라 변경 가능)
    mpu6050_accel_range_t accel_range = ACCEL_RANGE_2G; // ±2g
    mpu6050_gyro_range_t gyro_range = GYRO_RANGE_250;   // ±250°/s

    // MPU6050 초기화
    ret = mpu6050_init(dev_handle, accel_range, gyro_range);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "MPU6050 초기화 실패: %s", esp_err_to_name(ret));
        goto cleanup;
    }
    ESP_LOGI(TAG, "MPU6050 초기화 성공");

    // 감도 계산
    float accel_sensitivity = mpu6050_get_accel_sensitivity(accel_range);
    float gyro_sensitivity = mpu6050_get_gyro_sensitivity(gyro_range);

    // 센서 보정
    mpu6050_calibration_t calibration = {0};
    ret = mpu6050_calibrate(dev_handle, &calibration);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "MPU6050 보정 실패: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    ESP_LOGI(TAG, "센서 데이터 읽기 시작 (보정 완료)...");

    // 센서 데이터 연속 읽기
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t temperature;

    while (1)
    {
        ret =
            mpu6050_read_sensor_data(dev_handle, &accel_x, &accel_y, &accel_z, &gyro_x, &gyro_y, &gyro_z, &temperature);
        if (ret == ESP_OK)
        {
            // 보정 적용
            mpu6050_apply_calibration(&calibration, &accel_x, &accel_y, &accel_z, &gyro_x, &gyro_y, &gyro_z);

            // 물리 단위로 변환 (설정된 감도 사용)
            float accel_x_g = accel_x / accel_sensitivity;
            float accel_y_g = accel_y / accel_sensitivity;
            float accel_z_g = accel_z / accel_sensitivity;

            float gyro_x_dps = gyro_x / gyro_sensitivity;
            float gyro_y_dps = gyro_y / gyro_sensitivity;
            float gyro_z_dps = gyro_z / gyro_sensitivity;

            // 온도 변환: temp = (raw / 340) + 36.53
            float temp_c = (temperature / 340.0) + 36.53;

            ESP_LOGI(TAG, "가속도(g): X=%6.3f Y=%6.3f Z=%6.3f | 자이로(°/s): X=%7.2f Y=%7.2f Z=%7.2f | 온도: %.2f°C",
                     accel_x_g, accel_y_g, accel_z_g, gyro_x_dps, gyro_y_dps, gyro_z_dps, temp_c);
        }
        else
        {
            ESP_LOGE(TAG, "센서 데이터 읽기 실패: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms마다 읽기
    }

cleanup:
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C 종료 완료");
}
