/* 에어마우스 기능 헤더
 * MPU6050 센서를 이용한 에어마우스 구현
 */

#ifndef AIRMOUSE_H
#define AIRMOUSE_H

#include <stdint.h>
#include <stdbool.h>
#include "mpu6050.h"

// 에어마우스 모드 열거형
typedef enum {
    AIRMOUSE_MODE_SENSOR = 0,    // 센서 모드 (기존 운동 감지)
    AIRMOUSE_MODE_MOUSE = 1       // 에어마우스 모드
} airmouse_mode_t;

// 마우스 데이터 구조체
typedef struct {
    float mouse_x;          // 마우스 X 좌표 변화량 (픽셀)
    float mouse_y;          // 마우스 Y 좌표 변화량 (픽셀)
    int scroll_delta;       // 스크롤 변화량 (-1: 아래, 0: 없음, 1: 위)
} mouse_data_t;

// 캘리브레이션 데이터 구조체
typedef struct {
    float accel_x_offset;   // 가속도 X축 오프셋
    float accel_y_offset;   // 가속도 Y축 오프셋
    float accel_z_offset;   // 가속도 Z축 오프셋
    float gyro_x_offset;    // 자이로 X축 오프셋
    float gyro_y_offset;    // 자이로 Y축 오프셋
    float gyro_z_offset;    // 자이로 Z축 오프셋
    bool calibrated;        // 캘리브레이션 완료 여부
} calibration_data_t;

// 필터링 설정 구조체
typedef struct {
    float alpha;            // 저역통과 필터 계수 (0.0 ~ 1.0)
    float threshold;        // 움직임 임계값
    uint32_t smoothing_samples; // 스무딩 샘플 수
} filter_config_t;

// 감도 설정 구조체
typedef struct {
    float sensitivity_x;    // X축 감도 (1.0 = 기본값)
    float sensitivity_y;    // Y축 감도 (1.0 = 기본값)
    float sensitivity_z;    // Z축 감도 (1.0 = 기본값)
    float max_speed;        // 최대 마우스 속도 (픽셀/초)
} sensitivity_config_t;

// 제스처 인식 설정 구조체
typedef struct {
    float scroll_threshold;     // 스크롤 감지 임계값
} gesture_config_t;

/**
 * @brief 에어마우스 초기화
 *
 * @return true 성공, false 실패
 */
bool airmouse_init(void);

/**
 * @brief 에어마우스 모드 설정
 *
 * @param mode 에어마우스 모드
 */
void airmouse_set_mode(airmouse_mode_t mode);

/**
 * @brief 현재 에어마우스 모드 조회
 *
 * @return 현재 모드
 */
airmouse_mode_t airmouse_get_mode(void);

/**
 * @brief 센서 캘리브레이션 시작
 * 정지 상태에서 3초간 데이터를 수집하여 기준값 설정
 *
 * @return true 성공, false 실패
 */
bool airmouse_start_calibration(void);

/**
 * @brief 캘리브레이션 상태 확인
 *
 * @return true 캘리브레이션 완료, false 진행 중 또는 미완료
 */
bool airmouse_is_calibrated(void);

/**
 * @brief 센서 데이터를 마우스 데이터로 변환
 *
 * @param sensor_data MPU6050 센서 데이터
 * @param mouse_data 변환된 마우스 데이터
 * @return true 성공, false 실패
 */
bool airmouse_convert_sensor_to_mouse(const mpu6050_data_t *sensor_data, mouse_data_t *mouse_data);

/**
 * @brief 필터링 설정
 *
 * @param config 필터링 설정
 */
void airmouse_set_filter_config(const filter_config_t *config);

/**
 * @brief 감도 설정
 *
 * @param config 감도 설정
 */
void airmouse_set_sensitivity_config(const sensitivity_config_t *config);

/**
 * @brief 제스처 인식 설정
 *
 * @param config 제스처 설정
 */
void airmouse_set_gesture_config(const gesture_config_t *config);

/**
 * @brief 현재 필터링 설정 조회
 *
 * @param config 필터링 설정을 저장할 포인터
 */
void airmouse_get_filter_config(filter_config_t *config);

/**
 * @brief 현재 감도 설정 조회
 *
 * @param config 감도 설정을 저장할 포인터
 */
void airmouse_get_sensitivity_config(sensitivity_config_t *config);

/**
 * @brief 현재 제스처 설정 조회
 *
 * @param config 제스처 설정을 저장할 포인터
 */
void airmouse_get_gesture_config(gesture_config_t *config);

/**
 * @brief 에어마우스 종료 및 리소스 해제
 */
void airmouse_deinit(void);

#endif // AIRMOUSE_H
