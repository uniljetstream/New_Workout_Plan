/* 설정 헤더 파일
 * 모든 설정을 여기에서 관리
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ========== Wi-Fi 설정 ==========
// menuconfig에서 설정 (idf.py menuconfig -> ESP32 MPU6050 MQTT Configuration)
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASSWORD CONFIG_WIFI_PASSWORD
#define WIFI_MAX_RETRY CONFIG_WIFI_MAX_RETRY

// ========== MQTT 브로커 설정 ==========
// menuconfig에서 설정 (idf.py menuconfig -> ESP32 MPU6050 MQTT Configuration)
#define MQTT_BROKER_URL CONFIG_MQTT_BROKER_URL

// ========== MQTT 토픽 설정 ==========
// menuconfig에서 설정 (idf.py menuconfig -> ESP32 MPU6050 MQTT Configuration)
// WatchTower 프로토콜에 맞춘 토픽
#define MQTT_TOPIC_SENSOR_DATA CONFIG_MQTT_TOPIC_SENSOR_DATA   // joystick/sensor/data
#define MQTT_TOPIC_STATUS CONFIG_MQTT_TOPIC_STATUS             // joystick/status
#define MQTT_TOPIC_COMMAND CONFIG_MQTT_TOPIC_COMMAND           // watchtower/command/joystick

// ========== 센서 설정 ==========
#define DEFAULT_PUBLISH_INTERVAL_MS 200  // 기본 전송 주기: 200ms (더 안정적)

// ========== MPU6050 I2C 설정 ==========
#define I2C_MASTER_SCL_IO 22           // I2C 클럭 핀 (SCL)
#define I2C_MASTER_SDA_IO 21           // I2C 데이터 핀 (SDA)
#define I2C_MASTER_NUM I2C_NUM_0       // I2C 포트 번호
#define I2C_MASTER_FREQ_HZ 400000      // I2C 주파수 (400kHz)

// ========== 버튼 GPIO 설정 ==========
#define BUTTON_GPIO 19                  // 버튼 GPIO 핀 (GPIO 0 = BOOT 버튼)
#define BUTTON_ACTIVE_LEVEL 0          // 버튼 활성 레벨 (0 = Active LOW)
#define BUTTON_DEBOUNCE_MS 50          // 디바운싱 시간 (50ms)

// ========== 진동모터 설정 ==========
#define VIBRATION_MOTOR_GPIO 18         // 진동모터 GPIO 핀
#define VIBRATION_DURATION_MS 1000      // 진동 지속 시간 (1초)
#define GYRO_THRESHOLD_DEG_S 200.0f     // 자이로센서 임계값 (도/초)

// ========== 로그 태그 ==========
#define TAG_MAIN "ESP32_MAIN"
#define TAG_WIFI "ESP32_WIFI"
#define TAG_MQTT "ESP32_MQTT"
#define TAG_SENSOR "ESP32_SENSOR"

#endif // CONFIG_H
