/* Wi-Fi 핸들러 헤더
 * Wi-Fi 연결 및 관리
 */

#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <stdbool.h>

/**
 * @brief Wi-Fi 초기화 및 연결
 *
 * @return true 연결 성공, false 연결 실패
 */
bool wifi_init_and_connect(void);

#endif // WIFI_HANDLER_H
