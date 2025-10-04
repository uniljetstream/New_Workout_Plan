/* MQTT 핸들러 헤더
 * MQTT 통신 관리
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdbool.h>
#include "mqtt_client.h"
#include "mpu6050.h"

/**
 * @brief MQTT 초기화 및 시작
 */
void mqtt_init_and_start(void);

/**
 * @brief MQTT 연결 상태 확인
 *
 * @return true 연결됨, false 연결 안 됨
 */
bool mqtt_is_connected(void);

/**
 * @brief MQTT 클라이언트 핸들 가져오기
 *
 * @return MQTT 클라이언트 핸들
 */
esp_mqtt_client_handle_t mqtt_get_client(void);

/**
 * @brief MPU6050 센서 데이터 발행
 *
 * @param data MPU6050 센서 데이터
 */
void mqtt_publish_mpu6050_data(const mpu6050_data_t *data);

#endif // MQTT_HANDLER_H
