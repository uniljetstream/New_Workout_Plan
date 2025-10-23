/* MQTT 핸들러 구현 - Exerciser 전용 (진동모터 제어만) */

#include "mqtt_handler.h"
#include "config.h"
#include "vibration_motor.h"

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

// 고정 크기 버퍼들 (메모리 재사용)
static char status_payload_buffer[256];

/**
 * @brief MQTT 이벤트 핸들러
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT Connected to broker");
        mqtt_connected = true;

        // 명령 토픽 구독 (WatchTower에서 오는 명령)
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_COMMAND, 1);
        ESP_LOGI(TAG_MQTT, "Subscribed to topic: %s (msg_id=%d)", MQTT_TOPIC_COMMAND, msg_id);

        // 연결 후 ready 상태 발행
        mqtt_publish_status("ready");
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT Disconnected");
        mqtt_connected = false;

        // 자동 재연결 시도
        ESP_LOGI(TAG_MQTT, "Attempting to reconnect...");
        esp_mqtt_client_reconnect(mqtt_client);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT Subscribed, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        // WatchTower 명령 수신
        ESP_LOGI(TAG_MQTT, "MQTT Data received");
        ESP_LOGI(TAG_MQTT, "TOPIC: %.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG_MQTT, "DATA: %.*s", event->data_len, event->data);

        // 진동모터 트리거 명령 처리
        if (strstr(event->data, "\"command\":\"vibration_trigger\"") != NULL) {
            ESP_LOGI(TAG_MQTT, "Received VIBRATION TRIGGER command");
            if (vibration_motor_trigger()) {
                mqtt_publish_status("vibration_triggered");
            } else {
                ESP_LOGW(TAG_MQTT, "Failed to trigger vibration motor");
            }
        }
        // 진동모터 ON/OFF 명령 처리
        else if (strstr(event->data, "\"vibration\":\"ON\"") != NULL) {
            ESP_LOGI(TAG_MQTT, "Received VIBRATION ON command");

            // JSON에서 time 값 추출
            float vibration_time = 1.0f; // 기본값 1초
            char *time_start = strstr(event->data, "\"time\":");
            if (time_start != NULL) {
                time_start += 7; // "\"time\":" 길이만큼 이동
                char *time_end = strchr(time_start, ',');
                if (time_end == NULL) {
                    time_end = strchr(time_start, '}');
                }
                if (time_end != NULL) {
                    char time_str[32];
                    size_t time_len = time_end - time_start;
                    if (time_len < sizeof(time_str)) {
                        strncpy(time_str, time_start, time_len);
                        time_str[time_len] = '\0';
                        vibration_time = atof(time_str);
                        ESP_LOGI(TAG_MQTT, "Parsed vibration time: %.2f seconds", vibration_time);
                    }
                }
            }

            if (vibration_motor_trigger_duration(vibration_time * 1000.0f)) {
                ESP_LOGI(TAG_MQTT, "Vibration motor triggered for %.2f seconds", vibration_time);
                mqtt_publish_status("vibration_on");
            } else {
                ESP_LOGW(TAG_MQTT, "Failed to trigger vibration motor");
            }
        }
        else if (strstr(event->data, "\"vibration\":\"OFF\"") != NULL) {
            ESP_LOGI(TAG_MQTT, "Received VIBRATION OFF command");
            vibration_motor_stop();
            ESP_LOGI(TAG_MQTT, "Vibration motor stopped");
            mqtt_publish_status("vibration_off");
        }
        break;

    case MQTT_EVENT_ERROR:
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
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .session.keepalive = 60,
        .session.disable_clean_session = false,
        .network.reconnect_timeout_ms = 10000,
        .network.timeout_ms = 10000,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
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
 * @brief Exerciser 상태 발행
 */
void mqtt_publish_status(const char *status)
{
    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG_MQTT, "MQTT not connected, skipping status publish");
        return;
    }

    int64_t timestamp = esp_timer_get_time() / 1000;
    snprintf(status_payload_buffer, sizeof(status_payload_buffer),
             "{\"status\":\"%s\",\"timestamp\":%lld}",
             status, (long long)timestamp);

    int msg_id = esp_mqtt_client_publish(mqtt_client,
                                          MQTT_TOPIC_STATUS,
                                          status_payload_buffer,
                                          0, 1, 0);

    if (msg_id != -1) {
        ESP_LOGI(TAG_MQTT, "Published exerciser status: %s", status);
    } else {
        ESP_LOGE(TAG_MQTT, "Failed to publish exerciser status");
    }
}
