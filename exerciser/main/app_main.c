/* Exerciser - 진동모터 제어 전용
 *
 * MQTT 명령을 받아 진동모터만 제어하는 단순화된 ESP32 프로그램
 */

#include <stdio.h>
#include <inttypes.h>

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "config.h"
#include "wifi_handler.h"
#include "mqtt_handler.h"
#include "vibration_motor.h"

/**
 * @brief 메인 함수 - ESP32 부팅 시 자동 실행
 */
void app_main(void)
{
    ESP_LOGI(TAG_MAIN, "=== Exerciser Vibration Motor System Started ===");
    ESP_LOGI(TAG_MAIN, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG_MAIN, "IDF version: %s", esp_get_idf_version());

    // 로그 레벨 설정
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG_MAIN, ESP_LOG_INFO);
    esp_log_level_set(TAG_WIFI, ESP_LOG_INFO);
    esp_log_level_set(TAG_MQTT, ESP_LOG_INFO);

    // NVS 초기화
    ESP_ERROR_CHECK(nvs_flash_init());

    // 진동모터 초기화
    if (!vibration_motor_init()) {
        ESP_LOGE(TAG_MAIN, "Vibration motor initialization failed");
        return;
    }

    // Wi-Fi 연결
    if (!wifi_init_and_connect()) {
        ESP_LOGE(TAG_MAIN, "Wi-Fi connection failed, halting system");
        return;
    }

    // MQTT 시작
    mqtt_init_and_start();

    ESP_LOGI(TAG_MAIN, "System initialization complete");
    ESP_LOGI(TAG_MAIN, "Available MQTT commands:");
    ESP_LOGI(TAG_MAIN, "  - vibration_trigger: Trigger vibration with default duration");
    ESP_LOGI(TAG_MAIN, "  - vibration ON/OFF: Turn vibration on/off with custom time");
    ESP_LOGI(TAG_MAIN, "Waiting for commands from WatchTower...");

    // 시작 알림 진동 (500ms)
    vTaskDelay(pdMS_TO_TICKS(1000));
    vibration_motor_trigger_duration(500);
}
