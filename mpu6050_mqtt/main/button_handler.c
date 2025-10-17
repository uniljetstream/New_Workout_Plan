/* 버튼 핸들러 구현
 * GPIO 버튼 입력 처리 및 디바운싱
 */

#include "button_handler.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUTTON";

// 버튼 상태 변수
static bool button_initialized = false;
static bool previous_button_state = false;
static int64_t last_debounce_time = 0;

/**
 * @brief 버튼 초기화
 */
bool button_init(void)
{
    ESP_LOGI(TAG, "Initializing button on GPIO %d...", BUTTON_GPIO);

    // GPIO 설정
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,  // 풀업 저항 활성화 (Active LOW용)
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE  // 인터럽트 사용 안함 (폴링 방식)
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIO: %s", esp_err_to_name(ret));
        return false;
    }

    button_initialized = true;
    previous_button_state = false;
    last_debounce_time = esp_timer_get_time();

    ESP_LOGI(TAG, "Button initialized successfully");
    ESP_LOGI(TAG, "Button configuration: GPIO=%d, Active Level=%d, Debounce=%dms",
             BUTTON_GPIO, BUTTON_ACTIVE_LEVEL, BUTTON_DEBOUNCE_MS);

    return true;
}

/**
 * @brief 디바운싱을 적용한 버튼 상태 읽기
 */
bool button_is_pressed(void)
{
    if (!button_initialized) {
        return false;
    }

    // 현재 GPIO 레벨 읽기
    int gpio_level = gpio_get_level(BUTTON_GPIO);

    // Active level과 비교하여 버튼 눌림 판단
    bool current_pressed = (gpio_level == BUTTON_ACTIVE_LEVEL);

    // 디바운싱: 상태가 변경되었는지 확인
    int64_t current_time = esp_timer_get_time();
    int64_t time_since_last_change = (current_time - last_debounce_time) / 1000; // us -> ms

    // 이전 상태와 다르면 디바운싱 타이머 리셋
    if (current_pressed != previous_button_state) {
        last_debounce_time = current_time;
    }

    // 디바운싱 시간이 지났으면 상태 업데이트
    if (time_since_last_change > BUTTON_DEBOUNCE_MS) {
        previous_button_state = current_pressed;
    }

    return previous_button_state;
}

/**
 * @brief 버튼 클릭 이벤트 감지 (눌림 → 떼어짐 전환)
 *
 * 이 함수는 연속 클릭을 방지하기 위해 사용됩니다.
 * 버튼이 눌렸다가 떼어질 때만 true를 반환합니다.
 */
bool button_get_click_event(void)
{
    static bool previous_state = false;
    bool current_state = button_is_pressed();

    // 버튼이 눌렸다가 떼어진 경우 (falling edge)
    bool clicked = (previous_state == true && current_state == false);

    previous_state = current_state;

    if (clicked) {
        ESP_LOGI(TAG, "Button click event detected");
    }

    return clicked;
}

/**
 * @brief 버튼 핸들러 종료
 */
void button_deinit(void)
{
    if (button_initialized) {
        // GPIO 리셋
        gpio_reset_pin(BUTTON_GPIO);
        button_initialized = false;
        ESP_LOGI(TAG, "Button deinitialized");
    }
}
