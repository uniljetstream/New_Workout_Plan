/**
 * 워크아웃 MQTT 모듈
 *
 * 심박 센서 데이터를 외부 API로 전송하기 위한 MQTT 헬퍼입니다.
 */

#ifndef WATCH_MQTT_CLIENT_H
#define WATCH_MQTT_CLIENT_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MQTT 모듈 초기화
 *
 * - MQTT 클라이언트를 생성하고 이벤트 핸들러를 등록합니다.
 * - Wi-Fi 이벤트(IP 획득/연결 해제)를 감시하여 자동으로 연결을 관리합니다.
 */
esp_err_t watch_mqtt_client_init(void);

/**
 * 모듈 종료
 *
 * 필요시 명시적으로 MQTT 클라이언트를 정지하고 정리합니다.
 */
void watch_mqtt_client_deinit(void);

/**
 * 심박수 발행
 *
 * @param bpm 현재 측정된 심박수(BPM)
 * @return ESP_OK 또는 에러 코드
 */
esp_err_t watch_mqtt_client_publish_heart_rate(uint16_t bpm);

/**
 * MQTT 브로커와의 연결 상태
 *
 * @return true이면 브로커에 연결된 상태
 */
bool watch_mqtt_client_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // WATCH_MQTT_CLIENT_H
