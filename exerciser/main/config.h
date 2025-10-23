/* Exerciser 설정 헤더 파일
 * 진동모터 제어를 위한 설정
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ========== Wi-Fi 설정 ==========
// menuconfig에서 설정 (idf.py menuconfig -> Exerciser Configuration)
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASSWORD CONFIG_WIFI_PASSWORD
#define WIFI_MAX_RETRY CONFIG_WIFI_MAX_RETRY

// ========== MQTT 브로커 설정 ==========
// menuconfig에서 설정
#define MQTT_BROKER_URL CONFIG_MQTT_BROKER_URL

// ========== MQTT 토픽 설정 ==========
// joystick과 동일한 토픽 사용 (진동모터 명령 수신)
#define MQTT_TOPIC_COMMAND CONFIG_MQTT_TOPIC_COMMAND      // watchtower/command/joystick
#define MQTT_TOPIC_STATUS CONFIG_MQTT_TOPIC_STATUS         // exerciser/status

// ========== 진동모터 설정 ==========
#define VIBRATION_MOTOR_GPIO 10         // 진동모터 GPIO 핀
#define VIBRATION_DURATION_MS 1000      // 진동 지속 시간 (1초)

// ========== 로그 태그 ==========
#define TAG_MAIN "EXERCISER_MAIN"
#define TAG_WIFI "EXERCISER_WIFI"
#define TAG_MQTT "EXERCISER_MQTT"

#endif // CONFIG_H
