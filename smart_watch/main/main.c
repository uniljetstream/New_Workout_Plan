/**
 * ESP32 IoT 프로젝트 메인
 *
 * LCD 터치스크린과 센서를 사용하는 IoT 시스템입니다.
 * MQTT 프로토콜을 통해 데이터를 전송합니다.
 *
 * 주요 기능:
 * - ST7789 LCD 디스플레이 (240x280)
 * - CST816S 터치 입력
 * - LVGL GUI
 * - 센서 데이터 수집 (추가 예정)
 * - MQTT 통신 (추가 예정)
 */

#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "hardware.h"
#include "ui.h"
#include "wifi.h"
#include "sensor.h"
#include "watch_mqtt_client.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 IoT 시스템 시작 ===");
    
    // Watchdog 타이머 비활성화 (리셋 방지)
    esp_task_wdt_deinit();
    ESP_LOGI(TAG, "Watchdog 타이머 비활성화됨");
    
    // 추가 안정성 설정
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 30000,  // 30초 타임아웃
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // 모든 코어 모니터링
        .trigger_panic = false  // 패닉 대신 리셋 방지
    };
    esp_task_wdt_reconfigure(&twdt_config);
    ESP_LOGI(TAG, "Watchdog 재설정 완료 (30초 타임아웃)");
    
    // 메모리 상태 확인
    ESP_LOGI(TAG, "초기 힙 메모리: %d bytes", esp_get_free_heap_size());

    // ========================================================================
    // 1. 하드웨어 초기화 (LCD, 터치 센서)
    // ========================================================================
    if (hardware_init() != ESP_OK) {
        ESP_LOGE(TAG, "하드웨어 초기화 실패!");
        return;
    }

    // ========================================================================
    // 2. WiFi 초기화
    // ========================================================================
    if (wifi_init() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi 초기화 실패!");
        return;
    }

    // ========================================================================
    // 3. 심박 센서 초기화
    // ========================================================================
    if (heart_rate_sensor_init() != ESP_OK) {
        ESP_LOGE(TAG, "심박 센서 초기화 실패!");
        return;
    }

    // ========================================================================
    // 4. MQTT 모듈 초기화
    // ========================================================================
    if (watch_mqtt_client_init() != ESP_OK) {
        ESP_LOGE(TAG, "MQTT 모듈 초기화 실패!");
    }

    // ========================================================================
    // 5. UI 초기화 (LVGL)
    // ========================================================================
    if (ui_init() != ESP_OK) {
        ESP_LOGE(TAG, "UI 초기화 실패!");
        return;
    }

    // ========================================================================
    // 6. 저장된 WiFi 자격 증명으로 자동 연결 시도
    // ========================================================================
    ESP_LOGI(TAG, "저장된 WiFi 자격 증명 확인 중...");
    esp_err_t auto_connect_result = wifi_auto_connect(wifi_status_callback);
    if (auto_connect_result == ESP_OK) {
        ESP_LOGI(TAG, "저장된 WiFi로 자동 연결 시도 중...");
    } else {
        ESP_LOGI(TAG, "저장된 WiFi 없음. 수동 설정 필요");
    }

    // ========================================================================
    // 7. UI 태스크 시작
    // ========================================================================
    if (ui_start_task() != ESP_OK) {
        ESP_LOGE(TAG, "UI 태스크 시작 실패!");
        return;
    }

    // ========================================================================
    // 8. 심박 센서 태스크 시작
    // ========================================================================
    if (heart_rate_sensor_start() != ESP_OK) {
        ESP_LOGE(TAG, "심박 센서 태스크 시작 실패!");
    } else {
        ESP_LOGI(TAG, "심박 센서 동작 중");
    }

    // ========================================================================
    // 9. 추가 통신 초기화 (TODO: MQTT 외 확장)

    ESP_LOGI(TAG, "=== 시스템 초기화 완료 ===");
    
    // 메모리 사용량 출력
    ESP_LOGI(TAG, "사용 가능한 힙 메모리: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "최소 힙 메모리: %d bytes", esp_get_minimum_free_heap_size());
    
    if (auto_connect_result != ESP_OK) {
        ESP_LOGI(TAG, "WiFi 설정을 위해 WiFi 버튼을 눌러주세요.");
    }
}
