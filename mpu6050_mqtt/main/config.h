/* 설정 헤더 파일
 * 모든 설정을 여기에서 관리
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ========== Wi-Fi 설정 ==========
#define WIFI_SSID "embA"
#define WIFI_PASSWORD "embA1234"
#define WIFI_MAX_RETRY 5

// ========== MQTT 브로커 설정 ==========
#define MQTT_BROKER_URL "mqtt://10.10.16.111:1883"

// ========== MQTT 토픽 설정 ==========
#define MQTT_TOPIC_SENSOR_DATA "esp32/sensor/data"
#define MQTT_TOPIC_COMMAND "esp32/command"
#define MQTT_TOPIC_RESPONSE "esp32/response"

// ========== 센서 설정 ==========
#define DEFAULT_PUBLISH_INTERVAL_MS 5000  // 기본 전송 주기: 5초

// ========== MPU6050 I2C 설정 ==========
#define I2C_MASTER_SCL_IO 22           // I2C 클럭 핀 (SCL)
#define I2C_MASTER_SDA_IO 21           // I2C 데이터 핀 (SDA)
#define I2C_MASTER_NUM I2C_NUM_0       // I2C 포트 번호
#define I2C_MASTER_FREQ_HZ 400000      // I2C 주파수 (400kHz)

// ========== 로그 태그 ==========
#define TAG_MAIN "ESP32_MAIN"
#define TAG_WIFI "ESP32_WIFI"
#define TAG_MQTT "ESP32_MQTT"
#define TAG_SENSOR "ESP32_SENSOR"

#endif // CONFIG_H
