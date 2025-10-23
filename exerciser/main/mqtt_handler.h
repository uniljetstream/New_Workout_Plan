/* MQTT 핸들러 헤더 파일 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdbool.h>
#include "mqtt_client.h"

/**
 * @brief MQTT 초기화 및 시작
 */
void mqtt_init_and_start(void);

/**
 * @brief MQTT 연결 상태 확인
 */
bool mqtt_is_connected(void);

/**
 * @brief MQTT 클라이언트 핸들 가져오기
 */
esp_mqtt_client_handle_t mqtt_get_client(void);

/**
 * @brief Exerciser 상태 발행
 */
void mqtt_publish_status(const char *status);

#endif // MQTT_HANDLER_H
