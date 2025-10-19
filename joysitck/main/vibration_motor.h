/* 진동모터 제어 헤더
 * PN-VM102 진동모터 PWM 제어
 */

#ifndef VIBRATION_MOTOR_H
#define VIBRATION_MOTOR_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 진동모터 초기화
 *
 * @return true 성공, false 실패
 */
bool vibration_motor_init(void);

/**
 * @brief 진동모터 진동 시작
 *
 * @param intensity 진동 강도 (0~100, 0=정지, 100=최대)
 * @param duration_ms 진동 지속 시간 (밀리초, 0=무한정)
 * @return true 성공, false 실패
 */
bool vibration_motor_start(uint8_t intensity, uint32_t duration_ms);

/**
 * @brief 진동모터 정지
 *
 * @return true 성공, false 실패
 */
bool vibration_motor_stop(void);

/**
 * @brief 진동모터 상태 확인
 *
 * @return true 진동 중, false 정지 상태
 */
bool vibration_motor_is_running(void);

/**
 * @brief 진동모터 강도 설정
 *
 * @param intensity 진동 강도 (0~100)
 * @return true 성공, false 실패
 */
bool vibration_motor_set_intensity(uint8_t intensity);

/**
 * @brief 진동모터 패턴 실행
 *
 * @param pattern 패턴 배열 (강도 값들의 배열)
 * @param pattern_length 패턴 길이
 * @param interval_ms 패턴 간격 (밀리초)
 * @return true 성공, false 실패
 */
bool vibration_motor_run_pattern(const uint8_t* pattern, uint8_t pattern_length, uint32_t interval_ms);

/**
 * @brief 진동모터 종료
 */
void vibration_motor_deinit(void);

#endif // VIBRATION_MOTOR_H

