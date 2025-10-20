/* 진동모터 제어 구현
 * GPIO 18을 사용하여 진동모터 제어
 */

#include "vibration_motor.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "VIBRATION_MOTOR";

// 진동모터 상태 변수
static bool vibration_initialized = false;
static bool vibration_active = false;
static esp_timer_handle_t vibration_timer = NULL;

/**
 * @brief 진동모터 타이머 콜백 함수 (자동 정지)
 */
static void vibration_timer_callback(void *arg)
{
    gpio_set_level(VIBRATION_MOTOR_GPIO, 0); // 진동모터 끄기
    vibration_active = false;
    ESP_LOGI(TAG, "Vibration motor auto-stopped after %dms", VIBRATION_DURATION_MS);
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

    // 진동모터 초기 상태: 꺼짐
    gpio_set_level(VIBRATION_MOTOR_GPIO, 0);

    // 타이머 생성 (자동 정지용)
    esp_timer_create_args_t timer_args = {
        .callback = vibration_timer_callback,
        .name = "vibration_timer"
    };

    ret = esp_timer_create(&timer_args, &vibration_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create vibration timer: %s", esp_err_to_name(ret));
        gpio_reset_pin(VIBRATION_MOTOR_GPIO);
        return false;
    }

    vibration_initialized = true;
    vibration_active = false;

    ESP_LOGI(TAG, "Vibration motor initialized successfully");
    ESP_LOGI(TAG, "Vibration duration: %dms", VIBRATION_DURATION_MS);

    return true;
}

/**
 * @brief 진동모터 트리거 (지정된 시간 동안 울림)
 */
bool vibration_motor_trigger(void)
{
    return vibration_motor_trigger_duration(VIBRATION_DURATION_MS);
}

bool vibration_motor_trigger_duration(float duration_ms)
{
    if (!vibration_initialized) {
        ESP_LOGE(TAG, "Vibration motor not initialized");
        return false;
    }

    // 시간 유효성 검사 (0.1초 ~ 10초)
    if (duration_ms < 100.0f || duration_ms > 10000.0f) {
        ESP_LOGW(TAG, "Invalid duration %.2f ms, using default %.2f ms", duration_ms, VIBRATION_DURATION_MS);
        duration_ms = VIBRATION_DURATION_MS;
    }

    // 기존 타이머가 실행 중이면 정지
    if (vibration_active) {
        esp_timer_stop(vibration_timer);
        ESP_LOGI(TAG, "Stopping previous vibration");
    }

    // 진동모터 켜기
    gpio_set_level(VIBRATION_MOTOR_GPIO, 1);
    vibration_active = true;

    // 지정된 시간 후 자동 정지 타이머 시작
    esp_err_t ret = esp_timer_start_once(vibration_timer, (uint64_t)(duration_ms * 1000)); // us 단위
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start vibration timer: %s", esp_err_to_name(ret));
        gpio_set_level(VIBRATION_MOTOR_GPIO, 0);
        vibration_active = false;
        return false;
    }

    ESP_LOGI(TAG, "Vibration motor triggered for %.2f seconds", duration_ms / 1000.0f);
    return true;
}

/**
 * @brief 진동모터 즉시 정지
 */
bool vibration_motor_stop(void)
{
    if (!vibration_initialized) {
        ESP_LOGE(TAG, "Vibration motor not initialized");
        return false;
    }

    // 타이머 정지
    if (vibration_active) {
        esp_timer_stop(vibration_timer);
    }

    // 진동모터 끄기
    gpio_set_level(VIBRATION_MOTOR_GPIO, 0);
    vibration_active = false;

    ESP_LOGI(TAG, "Vibration motor stopped manually");
    return true;
}

/**
 * @brief 진동모터 핸들러 종료
 */
void vibration_motor_deinit(void)
{
    if (vibration_initialized) {
        // 타이머 정리
        if (vibration_timer) {
            esp_timer_stop(vibration_timer);
            esp_timer_delete(vibration_timer);
            vibration_timer = NULL;
        }

        // GPIO 리셋
        gpio_reset_pin(VIBRATION_MOTOR_GPIO);
        
        vibration_initialized = false;
        vibration_active = false;
        
        ESP_LOGI(TAG, "Vibration motor deinitialized");
    }
}