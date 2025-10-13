/* 센서 태스크 헤더
 * 센서 데이터 읽기 및 전송 관리
 */

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <stdint.h>
#include <stdbool.h>
#include "mpu6050.h"

/**
 * @brief 센서 태스크 시작
 */
void sensor_task_start(void);

/**
 * @brief MPU6050 센서 데이터 읽기
 *
 * @param data MPU6050 센서 데이터를 저장할 포인터
 * @return true 성공, false 실패
 */
bool sensor_read_data(mpu6050_data_t *data);

/**
 * @brief 전송 주기 설정
 *
 * @param interval_ms 전송 주기 (밀리초), 최소 100ms
 */
void sensor_set_publish_interval(uint32_t interval_ms);

/**
 * @brief 현재 전송 주기 조회
 *
 * @return 현재 전송 주기 (밀리초)
 */
uint32_t sensor_get_publish_interval(void);

#endif // SENSOR_TASK_H
