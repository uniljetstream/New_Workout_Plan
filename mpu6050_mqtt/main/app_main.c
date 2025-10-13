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

    // NVS 초기화
    ESP_ERROR_CHECK(nvs_flash_init());

    // Wi-Fi 연결
    if (!wifi_init_and_connect()) {
        ESP_LOGE(TAG_MAIN, "Wi-Fi connection failed, halting system");
        return;
    }

    // MQTT 시작
    mqtt_init_and_start();

    // 센서 데이터 전송 태스크 시작
    sensor_task_start();

    ESP_LOGI(TAG_MAIN, "System initialization complete");
}
