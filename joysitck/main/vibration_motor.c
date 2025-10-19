/* 진동모터 제어 구현 (간단한 GPIO 방식)
 * PN-VM102 진동모터 직접 제어
 */

#include "vibration_motor.h"
#include "config.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static const char *TAG = "VIBRATION";

// 진동모터 상태 변수
static bool motor_initialized = false;
static bool motor_running = false;
static TimerHandle_t motor_timer = NULL;

/**
 * @brief 진동모터 타이머 콜백 함수
 */
static void motor_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Vibration timer expired, stopping motor");
    vibration_motor_stop();
}

/**
 * @brief 진동모터 초기화
 */
bool vibration_motor_init(void)
{
    ESP_LOGI(TAG, "Initializing vibration motor on GPIO %d...", VIBRATION_MOTOR_GPIO);

    // GPIO 설정
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << VIBRATION_MOTOR_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure vibration motor GPIO: %s", esp_err_to_name(ret));
        return false;
    }

    // LEDC 타이머 설정 (PWM을 위한 타이머)
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,  // 0-1023 duty range
        .freq_hz = 1000,  // 1kHz PWM frequency
        .clk_cfg = LEDC_AUTO_CLK
    };
    
    ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(ret));
        return false;
    }

    // 초기 상태: 모터 정지 (PWM duty = 0)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // 타이머 생성 (진동 지속 시간 제어용)
    motor_timer = xTimerCreate("motor_timer", pdMS_TO_TICKS(1000), pdFALSE, NULL, motor_timer_callback);
    if (motor_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create motor timer");
        return false;
    }

    motor_initialized = true;
    motor_running = false;

    ESP_LOGI(TAG, "Vibration motor initialized successfully");
    ESP_LOGI(TAG, "Motor configuration: GPIO=%d, Active Level=%d", 
             VIBRATION_MOTOR_GPIO, VIBRATION_MOTOR_ACTIVE_LEVEL);

    return true;
}

/**
 * @brief 진동모터 진동 시작
 */
bool vibration_motor_start(uint8_t intensity, uint32_t duration_ms)
{
    if (!motor_initialized) {
        ESP_LOGE(TAG, "Motor not initialized");
        return false;
    }

    // 강도에 따른 PWM 설정 (0-100% -> 0-100% duty cycle)
    ESP_LOGI(TAG, "Starting vibration: intensity=%d%%, duration=%dms", intensity, duration_ms);

    // LEDC 채널 설정 (PWM을 통한 강도 제어)
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = VIBRATION_MOTOR_GPIO,
        .duty = (intensity * 1023) / 100,  // 0-100% -> 0-1023 duty
        .hpoint = 0
    };
    
    esp_err_t ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    motor_running = true;

    // 지속 시간이 설정된 경우 타이머 시작
    if (duration_ms > 0) {
        if (xTimerIsTimerActive(motor_timer)) {
            xTimerStop(motor_timer, 0);
        }
        xTimerChangePeriod(motor_timer, pdMS_TO_TICKS(duration_ms), 0);
        xTimerStart(motor_timer, 0);
    }

    return true;
}

/**
 * @brief 진동모터 정지
 */
bool vibration_motor_stop(void)
{
    if (!motor_initialized) {
        ESP_LOGE(TAG, "Motor not initialized");
        return false;
    }

    ESP_LOGI(TAG, "Stopping vibration motor");

    // PWM 정지 (duty cycle을 0으로 설정)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // 타이머 정지
    if (xTimerIsTimerActive(motor_timer)) {
        xTimerStop(motor_timer, 0);
    }

    motor_running = false;

    return true;
}

/**
 * @brief 진동모터 상태 확인
 */
bool vibration_motor_is_running(void)
{
    return motor_running;
}

/**
 * @brief 진동모터 강도 설정 (단순 모드에서는 무시)
 */
bool vibration_motor_set_intensity(uint8_t intensity)
{
    if (!motor_initialized) {
        ESP_LOGE(TAG, "Motor not initialized");
        return false;
    }

    ESP_LOGW(TAG, "set_intensity() ignored in simple GPIO mode (intensity=%d%%)", intensity);
    return true;  // 항상 성공으로 반환
}

/**
 * @brief 진동모터 패턴 실행 (간단한 버전)
 */
bool vibration_motor_run_pattern(const uint8_t* pattern, uint8_t pattern_length, uint32_t interval_ms)
{
    if (!motor_initialized) {
        ESP_LOGE(TAG, "Motor not initialized");
        return false;
    }

    if (pattern == NULL || pattern_length == 0) {
        ESP_LOGE(TAG, "Invalid pattern parameters");
        return false;
    }

    ESP_LOGI(TAG, "Running vibration pattern: length=%d, interval=%dms", pattern_length, interval_ms);
    ESP_LOGW(TAG, "Pattern execution simplified - only first pattern executed");

    // 간단히 첫 번째 패턴만 실행 (0이 아니면 진동 시작)
    if (pattern[0] > 0) {
        return vibration_motor_start(100, interval_ms);
    } else {
        return vibration_motor_stop();
    }
}

/**
 * @brief 진동모터 종료
 */
void vibration_motor_deinit(void)
{
    if (!motor_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing vibration motor");

    // 진동 정지
    vibration_motor_stop();

    // 타이머 삭제
    if (motor_timer != NULL) {
        xTimerDelete(motor_timer, 0);
        motor_timer = NULL;
    }

    // GPIO 핀을 비활성 상태로 설정
    gpio_set_level(VIBRATION_MOTOR_GPIO, !VIBRATION_MOTOR_ACTIVE_LEVEL);

    motor_initialized = false;
    motor_running = false;

    ESP_LOGI(TAG, "Vibration motor deinitialized");
}