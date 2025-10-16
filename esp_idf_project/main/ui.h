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

/**
 * WiFi 연결 상태 콜백 함수
 *
 * WiFi 연결 성공/실패 시 호출되는 콜백 함수입니다.
 * 배경색을 변경하고 리셋 타이머를 설정합니다.
 *
 * @param connected 연결 상태 (true: 성공, false: 실패)
 * @param message 상태 메시지
 */
void wifi_status_callback(bool connected, const char *message);

#endif // UI_H
