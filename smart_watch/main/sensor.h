/**
 * 심박 센서 모듈 헤더
 *
 * 실제 심박 센서 드라이버와 애플리케이션 사이의 추상화 계층을 제공합니다.
 * 하드웨어별 구현이 완료되면 이 파일의 API를 통해 시스템에 통합합니다.
 */

#ifndef SENSOR_H
#define SENSOR_H

#include "esp_err.h"
#include <stdint.h>

/**
 * 센서 데이터 업데이트 콜백 함수 타입
 *
 * @param bpm     새로 측정된 심박수(BPM)
 * @param ctx     등록 시 제공한 사용자 정의 포인터
 */
typedef void (*heart_rate_sensor_cb_t)(uint16_t bpm, void *ctx);

/**
 * 심박 센서 하드웨어 초기화
 *
 * - 전원/통신 라인 설정
 * - 필요한 드라이버 초기화
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t heart_rate_sensor_init(void);

/**
 * 심박 센서 측정 태스크 시작
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t heart_rate_sensor_start(void);

/**
 * 심박 센서 측정 태스크 정지
 */
void heart_rate_sensor_stop(void);

/**
 * 최신 심박수 읽기
 *
 * @return 마지막으로 측정된 BPM 값 (0이면 아직 측정되지 않음)
 */
uint16_t heart_rate_sensor_get_bpm(void);

/**
 * 데이터 업데이트 콜백 등록
 *
 * @param callback   새 측정값이 들어올 때 호출되는 콜백
 * @param ctx        콜백에 전달할 사용자 정의 포인터
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t heart_rate_sensor_register_callback(heart_rate_sensor_cb_t callback, void *ctx);

#endif // SENSOR_H

