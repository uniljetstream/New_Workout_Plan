/* 진동모터 제어 헤더 파일
 * GPIO 18을 사용하여 진동모터 제어
 */

#ifndef VIBRATION_MOTOR_H
#define VIBRATION_MOTOR_H

#include <stdbool.h>

/**
 * @brief 진동모터 초기화
 * @return true: 성공, false: 실패
 */
bool vibration_motor_init(void);

/**
 * @brief 진동모터 트리거 (지정된 시간 동안 울림)
 * @return true: 성공, false: 실패
 */
bool vibration_motor_trigger(void);

/**
 * @brief 진동모터 트리거 (사용자 지정 시간 동안 울림)
 * @param duration_ms 진동 지속 시간 (밀리초)
 * @return true: 성공, false: 실패
 */
bool vibration_motor_trigger_duration(float duration_ms);

/**
 * @brief 진동모터 즉시 정지
 * @return true: 성공, false: 실패
 */
bool vibration_motor_stop(void);

/**
 * @brief 진동모터 핸들러 종료
 */
void vibration_motor_deinit(void);

#endif // VIBRATION_MOTOR_H