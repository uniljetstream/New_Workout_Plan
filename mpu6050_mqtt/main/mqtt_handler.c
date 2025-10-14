/* MQTT 핸들러 구현 */

#include "mqtt_handler.h"
#include "sensor_task.h"
#include "config.h"
#include "airmouse.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

// MQTT 클라이언트 핸들
static esp_mqtt_client_handle_t mqtt_client = NULL;

// MQTT 연결 상태
static bool mqtt_connected = false;

// 센서 동작 상태
static bool sensor_running = false;

// 고정 크기 버퍼들 (메모리 재사용)
static char mqtt_payload_buffer[512];
static char status_payload_buffer[256];
static char mode_change_payload_buffer[256];

/**
 * @brief MQTT 이벤트 핸들러
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:  //연결 성공 이벤트가 발생하면
        ESP_LOGI(TAG_MQTT, "MQTT Connected to broker");
        mqtt_connected = true;  //mqtt_client 플래그를 true로 바꾸고

        // 명령 토픽 구독 (WatchTower에서 오는 명령)
        // watchtower/command/joystick
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_COMMAND, 1);
        ESP_LOGI(TAG_MQTT, "Subscribed to topic: %s (msg_id=%d)", MQTT_TOPIC_COMMAND, msg_id);

        // 연결 후 ready 상태 발행
        mqtt_publish_status("ready");
        
        // 자동으로 센서 태스크 시작 (에어마우스 모드로)
        ESP_LOGI(TAG_MQTT, "Auto-starting sensor task in airmouse mode");
        airmouse_set_mode(AIRMOUSE_MODE_MOUSE);
        sensor_running = true;
        sensor_task_start();
        break;

    case MQTT_EVENT_DISCONNECTED:   //연결 실패 이벤트가 발생하면
        ESP_LOGI(TAG_MQTT, "MQTT Disconnected");    
        mqtt_connected = false; //mqtt_connected 를 false로 바꿈
        
        // 자동 재연결 시도
        ESP_LOGI(TAG_MQTT, "Attempting to reconnect...");
        esp_mqtt_client_reconnect(mqtt_client);
        break;

    case MQTT_EVENT_SUBSCRIBED: //int msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_COMMAND, 1); 에서 발생한 구독이 성공하면 발생하는 이벤트
        ESP_LOGI(TAG_MQTT, "MQTT Subscribed, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:   //데이터 수신 이벤트가 발생하면
        // WatchTower 명령 수신
        ESP_LOGI(TAG_MQTT, "MQTT Data received");
        ESP_LOGI(TAG_MQTT, "TOPIC: %.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG_MQTT, "DATA: %.*s", event->data_len, event->data);

        // JSON 파싱을 위한 간단한 문자열 검색
        // WatchTower 명령 형식: {"command":"start","mode":"t_pose","timestamp":1234567890}
        // 또는 {"command":"stop","timestamp":1234567890}

        if (strstr(event->data, "\"command\":\"start\"") != NULL) {
            ESP_LOGI(TAG_MQTT, "Received START command from WatchTower");
            sensor_running = true;
            sensor_task_start();  // 센서 태스크 시작
            mqtt_publish_status("ready");
        }
        else if (strstr(event->data, "\"command\":\"stop\"") != NULL) {
            ESP_LOGI(TAG_MQTT, "Received STOP command from WatchTower");
            sensor_running = false;
            sensor_task_stop();  // 센서 태스크 중지
            mqtt_publish_status("stopped");
        }
        else if (strstr(event->data, "\"command\":\"airmouse_mode\"") != NULL) {
            ESP_LOGI(TAG_MQTT, "Received AIRMOUSE MODE command from WatchTower");
            airmouse_set_mode(AIRMOUSE_MODE_MOUSE);
            mqtt_publish_mode_change(AIRMOUSE_MODE_MOUSE);
        }
        else if (strstr(event->data, "\"command\":\"sensor_mode\"") != NULL) {
            ESP_LOGI(TAG_MQTT, "Received SENSOR MODE command from WatchTower");
            airmouse_set_mode(AIRMOUSE_MODE_SENSOR);
            mqtt_publish_mode_change(AIRMOUSE_MODE_SENSOR);
        }
        else if (strstr(event->data, "\"command\":\"calibrate\"") != NULL) {
            ESP_LOGI(TAG_MQTT, "Received CALIBRATE command from WatchTower");
            if (airmouse_start_calibration()) {
                mqtt_publish_status("calibrated");
            } else {
                mqtt_publish_status("calibration_failed");
            }
        }
        // 이전 호환성을 위한 주기 변경 명령
        else if (strncmp(event->data, "INTERVAL:", 9) == 0) {
            uint32_t new_interval = atoi(event->data + 9);
            sensor_set_publish_interval(new_interval);
            ESP_LOGI(TAG_MQTT, "Publish interval changed to %lu ms", new_interval);
        }
        break;

    case MQTT_EVENT_ERROR:  //에러 처리. 에러를 출력함.
        ESP_LOGE(TAG_MQTT, "MQTT Error");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(TAG_MQTT, "Transport error: %s",
                    strerror(event->error_handle->esp_transport_sock_errno));
        }
        mqtt_connected = false;
        break;

    default:
        ESP_LOGD(TAG_MQTT, "MQTT event id: %d", event->event_id);
        break;
    }
}

/**
 * @brief MQTT 초기화 및 시작
 */
void mqtt_init_and_start(void)
{   
    //mqtt 설정 구조체, 브로커 url 설정
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .session.keepalive = 60,        // Keep-alive 시간 (60초)
        .session.disable_clean_session = false,  // Clean session 활성화
        .network.reconnect_timeout_ms = 10000,   // 재연결 타임아웃 (10초)
        .network.timeout_ms = 10000,             // 네트워크 타임아웃 (10초)
    };

    //mqtt 클라이언트 생성
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    //mqtt 이벤트 핸들러 등록, mqtt client에서 모든 이벤트가 발생하면 mqtt_event_handler()가 호출됨.
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    //mqtt clinet 시작
    esp_mqtt_client_start(mqtt_client);

    ESP_LOGI(TAG_MQTT, "MQTT client started, broker: %s", MQTT_BROKER_URL);
}

/**
 * @brief MQTT 연결 상태 확인
 */
bool mqtt_is_connected(void)
{
    return mqtt_connected;
}

/**
 * @brief MQTT 클라이언트 핸들 가져오기
 */
esp_mqtt_client_handle_t mqtt_get_client(void)
{
    return mqtt_client;
}

/**
 * @brief MPU6050 센서 데이터 발행 (WatchTower 프로토콜)
 */
void mqtt_publish_mpu6050_data(const mpu6050_data_t *data)
{
    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG_MQTT, "MQTT not connected, skipping publish");
        return;
    }

    // WatchTower 프로토콜에 맞춘 JSON 형식 (고정 버퍼 재사용)
    // {"accel_x": 0.0, "accel_y": 0.0, "accel_z": 0.0, "gyro_x": 0.0, "gyro_y": 0.0, "gyro_z": 0.0, "timestamp": 1234567890}
    int64_t timestamp = esp_timer_get_time() / 1000;  // 밀리초 단위로 변환
    snprintf(mqtt_payload_buffer, sizeof(mqtt_payload_buffer),
             "{\"accel_x\":%.3f,\"accel_y\":%.3f,\"accel_z\":%.3f,"
             "\"gyro_x\":%.2f,\"gyro_y\":%.2f,\"gyro_z\":%.2f,"
             "\"timestamp\":%lld}",
             data->accel_x, data->accel_y, data->accel_z,
             data->gyro_x, data->gyro_y, data->gyro_z,
             (long long)timestamp);

    // MQTT 발행 (joystick/sensor/data 토픽)
    int msg_id = esp_mqtt_client_publish(mqtt_client,
                                          MQTT_TOPIC_SENSOR_DATA,
                                          mqtt_payload_buffer,
                                          0,    // 길이 (0 = 자동)
                                          1,    // QoS 1
                                          0);   // retain 플래그

    if (msg_id != -1) {
        ESP_LOGI(TAG_MQTT, "Published joystick data (msg_id=%d)", msg_id);
        ESP_LOGI(TAG_MQTT, "Accel(g): X=%.3f Y=%.3f Z=%.3f | Gyro(°/s): X=%.2f Y=%.2f Z=%.2f",
                 data->accel_x, data->accel_y, data->accel_z,
                 data->gyro_x, data->gyro_y, data->gyro_z);
    } else {
        ESP_LOGE(TAG_MQTT, "Failed to publish joystick data");
    }
}

/**
 * @brief 조이스틱 상태 발행 (WatchTower 프로토콜)
 */
void mqtt_publish_status(const char *status)
{
    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG_MQTT, "MQTT not connected, skipping status publish");
        return;
    }

    // WatchTower 프로토콜: {"status": "ready"} 또는 {"status": "stopped"} (고정 버퍼 재사용)
    int64_t timestamp = esp_timer_get_time() / 1000;
    snprintf(status_payload_buffer, sizeof(status_payload_buffer),
             "{\"status\":\"%s\",\"timestamp\":%lld}",
             status, (long long)timestamp);

    int msg_id = esp_mqtt_client_publish(mqtt_client,
                                          MQTT_TOPIC_STATUS,
                                          status_payload_buffer,
                                          0, 1, 0);

    if (msg_id != -1) {
        ESP_LOGI(TAG_MQTT, "Published joystick status: %s", status);
    } else {
        ESP_LOGE(TAG_MQTT, "Failed to publish joystick status");
    }
}

/**
 * @brief 에어마우스 데이터 발행
 */
void mqtt_publish_airmouse_data(const mouse_data_t *mouse_data, bool button_pressed)
{
    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG_MQTT, "MQTT not connected, skipping airmouse publish");

        // 연결 재시도
        ESP_LOGI(TAG_MQTT, "Attempting to reconnect MQTT...");
        esp_mqtt_client_reconnect(mqtt_client);
        return;
    }

    // 에어마우스 데이터 JSON 형식 (고정 버퍼 재사용)
    int64_t timestamp = esp_timer_get_time() / 1000;
    snprintf(mqtt_payload_buffer, sizeof(mqtt_payload_buffer),
             "{\"mode\":\"airmouse\","
             "\"mouse_x\":%.2f,\"mouse_y\":%.2f,"
             "\"scroll_delta\":%d,"
             "\"button_pressed\":%s,"
             "\"timestamp\":%lld}",
             mouse_data->mouse_x, mouse_data->mouse_y,
             mouse_data->scroll_delta,
             button_pressed ? "true" : "false",
             (long long)timestamp);

    int msg_id = esp_mqtt_client_publish(mqtt_client,
                                          MQTT_TOPIC_SENSOR_DATA,
                                          mqtt_payload_buffer,
                                          0, 1, 0);

    if (msg_id != -1) {
        ESP_LOGI(TAG_MQTT, "Published airmouse data (msg_id=%d)", msg_id);
        ESP_LOGI(TAG_MQTT, "Mouse: X=%.2f Y=%.2f | Scroll: %d | Button: %s",
                 mouse_data->mouse_x, mouse_data->mouse_y,
                 mouse_data->scroll_delta,
                 button_pressed ? "PRESSED" : "RELEASED");
    } else {
        ESP_LOGE(TAG_MQTT, "Failed to publish airmouse data");
        
        // 전송 실패 시 재연결 시도
        ESP_LOGI(TAG_MQTT, "Publish failed, attempting reconnect...");
        esp_mqtt_client_reconnect(mqtt_client);
    }
}

/**
 * @brief 에어마우스 모드 변경 명령 발행
 */
void mqtt_publish_mode_change(airmouse_mode_t mode)
{
    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG_MQTT, "MQTT not connected, skipping mode change publish");
        return;
    }

    int64_t timestamp = esp_timer_get_time() / 1000;
    snprintf(mode_change_payload_buffer, sizeof(mode_change_payload_buffer),
             "{\"mode_change\":\"%s\",\"timestamp\":%lld}",
             mode == AIRMOUSE_MODE_SENSOR ? "sensor" : "airmouse",
             (long long)timestamp);

    int msg_id = esp_mqtt_client_publish(mqtt_client,
                                          MQTT_TOPIC_STATUS,
                                          mode_change_payload_buffer,
                                          0, 1, 0);

    if (msg_id != -1) {
        ESP_LOGI(TAG_MQTT, "Published mode change: %s", 
                 mode == AIRMOUSE_MODE_SENSOR ? "SENSOR" : "AIRMOUSE");
    } else {
        ESP_LOGE(TAG_MQTT, "Failed to publish mode change");
    }
}
