/* ESP32 센서 데이터 MQTT 전송 - 메인 파일
 *
 * 이 코드는 ESP32에서 센서 데이터를 읽어 MQTT를 통해 Jetson으로 전송합니다.
 */

#include <stdio.h>
#include <inttypes.h>

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "config.h"
#include "wifi_handler.h"
#include "mqtt_handler.h"
#include "sensor_task.h"
#include "airmouse.h"
#include "button_handler.h"  // 버튼 핸들러 추가
#include "vibration_motor.h"  // 진동모터 핸들러 추가

/**
 * @brief 메인 함수 - ESP32 부팅 시 자동 실행
 */
void app_main(void)
{
    ESP_LOGI(TAG_MAIN, "=== ESP32 Sensor MQTT System Started ===");
    ESP_LOGI(TAG_MAIN, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG_MAIN, "IDF version: %s", esp_get_idf_version());

    // 로그 레벨 설정
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG_MAIN, ESP_LOG_INFO);
    esp_log_level_set(TAG_WIFI, ESP_LOG_INFO);
    esp_log_level_set(TAG_MQTT, ESP_LOG_INFO);
    esp_log_level_set(TAG_SENSOR, ESP_LOG_INFO);
    esp_log_level_set("AIRMOUSE", ESP_LOG_INFO);

    // NVS 초기화
    ESP_ERROR_CHECK(nvs_flash_init());

    // Wi-Fi 연결
    if (!wifi_init_and_connect()) {
        ESP_LOGE(TAG_MAIN, "Wi-Fi connection failed, halting system");
        return;
    }

    // 에어마우스 초기화
    if (!airmouse_init()) {
        ESP_LOGE(TAG_MAIN, "AirMouse initialization failed");
        return;
    }

    // 버튼 초기화
    if (!button_init()) {
        ESP_LOGE(TAG_MAIN, "Button initialization failed");
        return;
    }

    // 진동모터 초기화 - 비활성화됨
    /*
    if (!vibration_motor_init()) {
        ESP_LOGE(TAG_MAIN, "Vibration motor initialization failed");
        return;
    }
    */

    // MQTT 시작
    mqtt_init_and_start();

    // WatchTower 프로토콜: MQTT 연결 후 ready 상태를 발행하고 start 명령을 대기
    // sensor_task는 WatchTower에서 start 명령이 오면 자동으로 시작됨

    ESP_LOGI(TAG_MAIN, "System initialization complete");
    ESP_LOGI(TAG_MAIN, "Available commands:");
    ESP_LOGI(TAG_MAIN, "  - start: Start sensor task");
    ESP_LOGI(TAG_MAIN, "  - stop: Stop sensor task");
    ESP_LOGI(TAG_MAIN, "  - airmouse_mode: Switch to airmouse mode");
    ESP_LOGI(TAG_MAIN, "  - sensor_mode: Switch to sensor mode");
    ESP_LOGI(TAG_MAIN, "  - calibrate: Calibrate airmouse");
    ESP_LOGI(TAG_MAIN, "  - vibrate: Start vibration (with intensity/duration)");
    ESP_LOGI(TAG_MAIN, "  - vibrate_stop: Stop vibration");
    ESP_LOGI(TAG_MAIN, "  - vibrate_pattern: Run vibration pattern");
    ESP_LOGI(TAG_MAIN, "Waiting for commands from WatchTower...");
}
