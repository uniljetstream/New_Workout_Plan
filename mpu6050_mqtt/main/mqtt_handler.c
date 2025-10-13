/* MQTT 핸들러 구현 */

#include "mqtt_handler.h"
#include "sensor_task.h"
#include "config.h"

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

        // 명령 토픽(esp32/command) 구독
        // @arg 클라이언ㅌ, 토믹, Qos size
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_COMMAND, 1);
        ESP_LOGI(TAG_MQTT, "Subscribed to topic: %s (msg_id=%d)", MQTT_TOPIC_COMMAND, msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:   //연결 실패 이벤트가 발생하면
        ESP_LOGI(TAG_MQTT, "MQTT Disconnected");    
        mqtt_connected = false; //mqtt_connected 를 false로 바꿈
        break;

    case MQTT_EVENT_SUBSCRIBED: //int msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_COMMAND, 1); 에서 발생한 구독이 성공하면 발생하는 이벤트
        ESP_LOGI(TAG_MQTT, "MQTT Subscribed, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:   //데이터 수신 이벤트가 발생하면
        // 명령 수신
        ESP_LOGI(TAG_MQTT, "MQTT Data received");
        printf("TOPIC: %.*s\n", event->topic_len, event->topic);
        printf("DATA: %.*s\n", event->data_len, event->data);

        // 전송 주기 변경 명령 처리
        if (strncmp(event->data, "INTERVAL:", 9) == 0) {    //받은 명령 첫 9글자가 "INTERVAL:"  와 같으면
            //event->data의 포인터를 9칸 이동. INTERVAL: 이후를 저장
            uint32_t new_interval = atoi(event->data + 9); 
            //주기 설정를 설장함.
            sensor_set_publish_interval(new_interval);

            // 응답 메시지 생성 
            char response[64];
            snprintf(response, sizeof(response),
                    "{\"status\":\"ok\",\"interval\":%lu}",
                    sensor_get_publish_interval());
            //응답 메시지를 발행
            //@arg : 클라이언트 핸들, 응답을 보낼 토픽, 발행할 메시지, 메시지 길이(0이면 자동계산), QoS 레벨(1:최소 한 번 전송 보장), retain 플래그(0 유지 안함)
            esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_RESPONSE, response, 0, 1, 0);
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
 * @brief MPU6050 센서 데이터 발행
 */
void mqtt_publish_mpu6050_data(const mpu6050_data_t *data)
{
    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG_MQTT, "MQTT not connected, skipping publish");
        return;
    }

    // JSON 형식으로 MPU6050 데이터 생성
    char payload[256];
    int64_t timestamp = esp_timer_get_time() / 1000000;
    snprintf(payload, sizeof(payload),
             "{\"sensor\":\"MPU6050\","
             "\"accel\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
             "\"gyro\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},"
             "\"temp\":%.2f,"
             "\"timestamp\":%lld}",
             data->accel_x, data->accel_y, data->accel_z,
             data->gyro_x, data->gyro_y, data->gyro_z,
             data->temperature,
             (long long)timestamp);

    // MQTT 발행
    int msg_id = esp_mqtt_client_publish(mqtt_client,
                                          MQTT_TOPIC_SENSOR_DATA,
                                          payload,
                                          0,    // 길이 (0 = 자동)
                                          1,    // QoS 1
                                          0);   // retain 플래그

    if (msg_id != -1) {
        ESP_LOGI(TAG_MQTT, "Published MPU6050 data (msg_id=%d)", msg_id);
        ESP_LOGI(TAG_MQTT, "Accel(g): X=%.3f Y=%.3f Z=%.3f | Gyro(°/s): X=%.2f Y=%.2f Z=%.2f | Temp: %.2f°C",
                 data->accel_x, data->accel_y, data->accel_z,
                 data->gyro_x, data->gyro_y, data->gyro_z,
                 data->temperature);
    } else {
        ESP_LOGE(TAG_MQTT, "Failed to publish MPU6050 data");
    }
}
