/**
 * UI 모듈 헤더
 *
 * LVGL UI 초기화 및 화면 생성 관련 함수를 제공합니다.
 */

#ifndef UI_H
#define UI_H

#include "lvgl.h"
#include "esp_err.h"

/**
 * LVGL 초기화 및 UI 생성
 *
 * - LVGL 라이브러리 초기화
 * - 디스플레이 버퍼 할당
 * - 디스플레이/터치 드라이버 등록
 * - 기본 UI 생성
 * - LVGL 타이머 시작
 *
 * @return ESP_OK: 성공, 그 외: 에러 코드
 */
esp_err_t ui_init(void);

/**
 * LVGL 처리 태스크 시작
 *
 * FreeRTOS 태스크를 생성하여 LVGL 이벤트를 처리합니다.
 *
 * @return ESP_OK: 성공, 그 외: 에러 코드
 */
esp_err_t ui_start_task(void);

#endif // UI_H
