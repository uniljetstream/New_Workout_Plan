/* 버튼 핸들러 헤더
 * GPIO 버튼 입력 처리 및 디바운싱
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 버튼 초기화
 *
 * @return true 성공, false 실패
 */
bool button_init(void);

/**
 * @brief 버튼 상태 읽기 (디바운싱 적용)
 *
 * @return true 버튼이 눌림, false 버튼이 떼어짐
 */
bool button_is_pressed(void);

/**
 * @brief 버튼이 눌렸다가 떼어지는 이벤트 감지
 * 이전 상태를 저장하여 버튼이 눌렸다가 떼어질 때만 true 반환
 *
 * @return true 클릭 이벤트 발생, false 클릭 없음
 */
bool button_get_click_event(void);

/**
 * @brief 버튼 핸들러 종료
 */
void button_deinit(void);

#endif // BUTTON_HANDLER_H
