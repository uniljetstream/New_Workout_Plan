/* MQTT 핸들러 헤더
 * MQTT 통신 관리
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdbool.h>
#include "mqtt_client.h"
#include "mpu6050.h"
#include "airmouse.h"

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
 * @brief MPU6050 센서 데이터 발행 (WatchTower 프로토콜)
 *
 * @param data MPU6050 센서 데이터
 */
void mqtt_publish_mpu6050_data(const mpu6050_data_t *data);

/**
 * @brief 조이스틱 상태 발행 (WatchTower 프로토콜)
 *
 * @param status 상태 문자열 ("ready", "stopped" 등)
 */
void mqtt_publish_status(const char *status);

/**
 * @brief 에어마우스 데이터 발행
 *
 * @param mouse_data 마우스 데이터
 * @param button_pressed 버튼 눌림 상태
 */
void mqtt_publish_airmouse_data(const mouse_data_t *mouse_data, bool button_pressed);

/**
 * @brief 에어마우스 모드 변경 명령 발행
 *
 * @param mode 에어마우스 모드
 */
void mqtt_publish_mode_change(airmouse_mode_t mode);

/**
 * @brief 진동모터 트리거 명령 발행
 */
void mqtt_publish_vibration_trigger(void);

/**
 * @brief 진동모터 활성화/비활성화 명령 발행
 *
 * @param enabled true: 활성화, false: 비활성화
 */
void mqtt_publish_vibration_enable(bool enabled);

#endif // MQTT_HANDLER_H
