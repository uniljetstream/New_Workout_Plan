/**
 * WiFi 모듈 헤더
 *
 * WiFi 스캔, 연결 및 관리 기능을 제공합니다.
 */

#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include "esp_wifi.h"
#include <stdbool.h>

#define MAX_WIFI_SCAN_RESULTS 20

/**
 * WiFi 스캔 결과 구조체
 */
typedef struct {
    char ssid[33];
    int8_t rssi;
    wifi_auth_mode_t authmode;
    uint8_t channel;
} wifi_scan_result_t;

/**
 * WiFi 상태 콜백 함수 타입
 */
typedef void (*wifi_status_cb_t)(bool connected, const char *message);

/**
 * WiFi 모듈 초기화
 */
esp_err_t wifi_init(void);

/**
 * WiFi AP 스캔 시작
 * @return 스캔된 AP 개수
 */
uint16_t wifi_scan(wifi_scan_result_t *results, uint16_t max_results);

/**
 * WiFi 연결
 * @param ssid WiFi SSID
 * @param password WiFi 비밀번호 (NULL이면 open network)
 * @param callback 연결 상태 콜백
 */
esp_err_t wifi_connect(const char *ssid, const char *password, wifi_status_cb_t callback);

/**
 * WiFi 연결 해제
 */
esp_err_t wifi_disconnect(void);

/**
 * WiFi 연결 상태 확인
 */
bool wifi_is_connected(void);

/**
 * 연결된 WiFi SSID 가져오기
 */
const char* wifi_get_connected_ssid(void);

/**
 * 저장된 WiFi 자격 증명으로 자동 연결 시도
 * @param callback 연결 상태 콜백
 * @return ESP_OK: 저장된 자격 증명 있음, ESP_ERR_NOT_FOUND: 저장된 자격 증명 없음
 */
esp_err_t wifi_auto_connect(wifi_status_cb_t callback);

/**
 * 저장된 WiFi 자격 증명 삭제
 */
esp_err_t wifi_clear_saved_credentials(void);

#endif // WIFI_H
