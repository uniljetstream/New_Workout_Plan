/**
 * 워치 MQTT 모듈 구현
 *
 * Wi-Fi 연결 상태를 감시하여 자동으로 MQTT 브로커에 접속하고
 * 심박수 데이터를 JSON 형태로 발행합니다.
 */

#include "watch_mqtt_client.h"

#include "mqtt_client.h"

#include "sdkconfig.h"

#if CONFIG_WORKOUT_MQTT_ENABLED

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sensor.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>

static const char *TAG = "WATCH_MQTT";

static esp_mqtt_client_handle_t s_client = NULL;
static SemaphoreHandle_t s_publish_mutex = NULL;
static bool s_client_started = false;
static bool s_is_connected = false;
static bool s_measurement_enabled = false;
static char s_current_mode[32] = "";

static esp_err_t ensure_publish_mutex(void)
{
    if (s_publish_mutex == NULL)
    {
        s_publish_mutex = xSemaphoreCreateMutex();
        if (s_publish_mutex == NULL)
        {
            ESP_LOGE(TAG, "Publish 뮤텍스 생성 실패");
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}

static long long get_timestamp_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
}

static esp_err_t mqtt_publish_locked(const char *topic, const char *payload, int len, int qos)
{
    if (!topic || !payload || len <= 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_is_connected || !s_client)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (ensure_publish_mutex() != ESP_OK)
    {
        return ESP_FAIL;
    }

    if (xSemaphoreTake(s_publish_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        ESP_LOGW(TAG, "MQTT publish 뮤텍스 획득 실패");
        return ESP_ERR_TIMEOUT;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, len, qos, 0);
    xSemaphoreGive(s_publish_mutex);

    if (msg_id < 0)
    {
        ESP_LOGE(TAG, "MQTT 발행 실패 (topic=%s, msg_id=%d)", topic, msg_id);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t publish_status(const char *status, const char *mode, const char *info)
{
    if (!status || status[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    long long timestamp_ms = get_timestamp_ms();
    char payload[256];

    int len = snprintf(payload, sizeof(payload),
                       "{\"device_id\":\"%s\",\"status\":\"%s\",\"timestamp\":%lld",
                       CONFIG_WORKOUT_MQTT_CLIENT_ID,
                       status,
                       timestamp_ms);

    if (len < 0 || len >= (int)sizeof(payload))
    {
        ESP_LOGE(TAG, "워치 상태 페이로드 생성 실패(1)");
        return ESP_FAIL;
    }

    if (mode && mode[0] != '\0')
    {
        int written = snprintf(payload + len, sizeof(payload) - len, ",\"mode\":\"%s\"", mode);
        if (written < 0 || written >= (int)(sizeof(payload) - len))
        {
            ESP_LOGE(TAG, "워치 상태 페이로드 생성 실패(모드)");
            return ESP_FAIL;
        }
        len += written;
    }

    if (info && info[0] != '\0')
    {
        int written = snprintf(payload + len, sizeof(payload) - len, ",\"info\":\"%s\"", info);
        if (written < 0 || written >= (int)(sizeof(payload) - len))
        {
            ESP_LOGE(TAG, "워치 상태 페이로드 생성 실패(info)");
            return ESP_FAIL;
        }
        len += written;
    }

    int closing = snprintf(payload + len, sizeof(payload) - len, "}");
    if (closing < 0 || closing >= (int)(sizeof(payload) - len))
    {
        ESP_LOGE(TAG, "워치 상태 페이로드 생성 실패(닫기)");
        return ESP_FAIL;
    }
    len += closing;

    esp_err_t ret = mqtt_publish_locked(CONFIG_WORKOUT_MQTT_TOPIC_STATUS, payload, len, CONFIG_WORKOUT_MQTT_QOS);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "워치 상태 발행: %s", payload);
    }
    return ret;
}

static void handle_watch_command(const char *payload)
{
    if (!payload)
    {
        return;
    }

    ESP_LOGI(TAG, "Watch 명령 수신: %s", payload);
    cJSON *root = cJSON_Parse(payload);
    if (!root)
    {
        ESP_LOGW(TAG, "Watch 명령 JSON 파싱 실패");
        publish_status("error", s_current_mode[0] ? s_current_mode : NULL, "invalid_json");
        return;
    }

    const cJSON *command_item = cJSON_GetObjectItemCaseSensitive(root, "command");
    const cJSON *mode_item = cJSON_GetObjectItemCaseSensitive(root, "mode");

    const char *command = (cJSON_IsString(command_item) && command_item->valuestring) ? command_item->valuestring : NULL;
    const char *mode = (cJSON_IsString(mode_item) && mode_item->valuestring && mode_item->valuestring[0]) ? mode_item->valuestring : NULL;

    if (mode)
    {
        strncpy(s_current_mode, mode, sizeof(s_current_mode) - 1);
        s_current_mode[sizeof(s_current_mode) - 1] = '\0';
    }

    if (!command)
    {
        ESP_LOGW(TAG, "Watch 명령에 command 필드가 없습니다");
        publish_status("error", s_current_mode[0] ? s_current_mode : NULL, "missing_command");
        cJSON_Delete(root);
        return;
    }

    if (strcmp(command, "start") == 0)
    {
        if (!s_measurement_enabled)
        {
            esp_err_t sensor_ret = heart_rate_sensor_start();
            if (sensor_ret == ESP_OK)
            {
                s_measurement_enabled = true;
                publish_status("running", s_current_mode[0] ? s_current_mode : NULL, "start_command");
            }
            else
            {
                ESP_LOGE(TAG, "심박 센서 시작 실패: %s", esp_err_to_name(sensor_ret));
                publish_status("error", s_current_mode[0] ? s_current_mode : NULL, "sensor_start_failed");
            }
        }
        else
        {
            publish_status("running", s_current_mode[0] ? s_current_mode : NULL, "already_running");
        }
    }
    else if (strcmp(command, "stop") == 0)
    {
        if (s_measurement_enabled)
        {
            heart_rate_sensor_stop();
            s_measurement_enabled = false;
        }
        publish_status("stopped", s_current_mode[0] ? s_current_mode : NULL, "stop_command");
        s_current_mode[0] = '\0';
        publish_status("ready", NULL, "await_start");
    }
    else if (strcmp(command, "status") == 0)
    {
        if (s_measurement_enabled)
        {
            publish_status("running",
                           s_current_mode[0] ? s_current_mode : NULL,
                           "status_request");
        }
        else
        {
            publish_status("ready", NULL, "status_request");
        }
    }
    else
    {
        ESP_LOGW(TAG, "알 수 없는 Watch 명령: %s", command);
        publish_status("error", s_current_mode[0] ? s_current_mode : NULL, "unknown_command");
    }

    cJSON_Delete(root);
}

static void mqtt_log_last_error(esp_mqtt_event_handle_t event)
{
    if (!event || !event->error_handle)
    {
        ESP_LOGI(TAG, "MQTT 에러 코드 없음");
        return;
    }

    const esp_mqtt_error_codes_t *err = event->error_handle;
    ESP_LOGE(TAG,
             "MQTT 에러 - type: 0x%x connect_code: 0x%x tls_err: 0x%x stack_err: 0x%x sock_errno: %d",
             err->error_type,
             err->connect_return_code,
             err->esp_tls_last_esp_err,
             err->esp_tls_stack_err,
             err->esp_transport_sock_errno);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        s_is_connected = true;
        ESP_LOGI(TAG, "MQTT 브로커 연결 완료");

        if (s_client)
        {
            int msg_id = esp_mqtt_client_subscribe(s_client, CONFIG_WORKOUT_MQTT_TOPIC_COMMAND, CONFIG_WORKOUT_MQTT_QOS);
            if (msg_id >= 0)
            {
                ESP_LOGI(TAG, "워치 명령 토픽 구독 성공: %s (msg_id=%d)", CONFIG_WORKOUT_MQTT_TOPIC_COMMAND, msg_id);
            }
            else
            {
                ESP_LOGE(TAG, "워치 명령 토픽 구독 실패: %d", msg_id);
            }
        }

        const char *mode = (s_current_mode[0] != '\0') ? s_current_mode : NULL;
        const char *info = s_measurement_enabled ? "mqtt_reconnected" : "mqtt_connected";
        const char *status = s_measurement_enabled ? "running" : "ready";
        publish_status(status, s_measurement_enabled ? mode : NULL, info);
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_is_connected = false;
        ESP_LOGW(TAG, "MQTT 브로커 연결 해제");
        break;
    case MQTT_EVENT_ERROR:
        mqtt_log_last_error(event);
        if (s_client && s_client_started)
        {
            ESP_LOGW(TAG, "MQTT 재연결 시도");
            esp_err_t ret = esp_mqtt_client_reconnect(s_client);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "MQTT 재연결 실패: %s", esp_err_to_name(ret));
            }
        }
        break;
    case MQTT_EVENT_DATA:
        if (event->topic && event->topic_len == strlen(CONFIG_WORKOUT_MQTT_TOPIC_COMMAND) &&
            strncmp(event->topic, CONFIG_WORKOUT_MQTT_TOPIC_COMMAND, event->topic_len) == 0)
        {
            char *payload = malloc(event->data_len + 1);
            if (!payload)
            {
                ESP_LOGE(TAG, "워치 명령 페이로드 메모리 부족");
                break;
            }
            memcpy(payload, event->data, event->data_len);
            payload[event->data_len] = '\0';
            handle_watch_command(payload);
            free(payload);
        }
        else
        {
            ESP_LOGI(TAG, "수신 토픽: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "수신 데이터: %.*s", event->data_len, event->data);
        }
        break;
    default:
        ESP_LOGD(TAG, "MQTT 이벤트: %" PRId32, event_id);
        break;
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (!s_client)
    {
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        if (!s_client_started)
        {
            esp_err_t ret = esp_mqtt_client_start(s_client);
            if (ret == ESP_OK)
            {
                s_client_started = true;
                ESP_LOGI(TAG, "MQTT 클라이언트 시작");
            }
            else
            {
                ESP_LOGE(TAG, "MQTT 클라이언트 시작 실패: %s", esp_err_to_name(ret));
            }
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_client_started)
        {
            esp_mqtt_client_stop(s_client);
            s_client_started = false;
            s_is_connected = false;
            ESP_LOGW(TAG, "Wi-Fi 해제 -> MQTT 정지");
        }
    }
}

esp_err_t watch_mqtt_client_init(void)
{
    if (s_client != NULL)
    {
        return ESP_OK;
    }

    esp_err_t mutex_ret = ensure_publish_mutex();
    if (mutex_ret != ESP_OK)
    {
        return mutex_ret;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_WORKOUT_MQTT_BROKER_URI,
        .credentials = {
            .username = CONFIG_WORKOUT_MQTT_USERNAME,
            .client_id = CONFIG_WORKOUT_MQTT_CLIENT_ID,
            .authentication.password = CONFIG_WORKOUT_MQTT_PASSWORD,
        },
        .session.keepalive = CONFIG_WORKOUT_MQTT_KEEPALIVE_SEC,
    };

    if (strlen(CONFIG_WORKOUT_MQTT_CERT_PEM) > 0)
    {
        mqtt_cfg.broker.verification.certificate = CONFIG_WORKOUT_MQTT_CERT_PEM;
    }

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_client)
    {
        ESP_LOGE(TAG, "MQTT 클라이언트 생성 실패");
        return ESP_FAIL;
    }

    esp_err_t ret = esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "MQTT 이벤트 핸들러 등록 실패: %s", esp_err_to_name(ret));
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        return ret;
    }

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "IP 이벤트 핸들러 등록 실패: %s", esp_err_to_name(ret));
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        return ret;
    }

    ret = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event_handler, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Wi-Fi 핸들러 등록 실패: %s", esp_err_to_name(ret));
        esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler);
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "MQTT 모듈 초기화 완료 (브로커: %s)", CONFIG_WORKOUT_MQTT_BROKER_URI);
    return ESP_OK;
}

void watch_mqtt_client_deinit(void)
{
    if (s_client)
    {
        if (s_client_started)
        {
            esp_mqtt_client_stop(s_client);
            s_client_started = false;
        }
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        s_is_connected = false;
    }

    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler);
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event_handler);

    if (s_publish_mutex)
    {
        vSemaphoreDelete(s_publish_mutex);
        s_publish_mutex = NULL;
    }

    s_measurement_enabled = false;
    s_current_mode[0] = '\0';
}

bool watch_mqtt_client_is_connected(void)
{
    return s_is_connected;
}

bool watch_mqtt_client_measurement_active(void)
{
    return s_measurement_enabled;
}

esp_err_t watch_mqtt_client_publish_heart_rate(uint16_t bpm)
{
    if (!CONFIG_WORKOUT_MQTT_ENABLED)
    {
        return ESP_OK;
    }

    if (!s_measurement_enabled)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_is_connected || !s_client)
    {
        return ESP_ERR_INVALID_STATE;
    }

    long long timestamp_ms = get_timestamp_ms();
    char payload[160];

    int len = snprintf(payload, sizeof(payload),
                       "{\"device_id\":\"%s\",\"heart_rate\":%u,\"timestamp\":%lld",
                       CONFIG_WORKOUT_MQTT_CLIENT_ID,
                       bpm,
                       timestamp_ms);

    if (len < 0 || len >= (int)sizeof(payload))
    {
        ESP_LOGE(TAG, "심박수 페이로드 생성 실패(1)");
        return ESP_FAIL;
    }

    if (s_current_mode[0] != '\0')
    {
        int written = snprintf(payload + len, sizeof(payload) - len, ",\"mode\":\"%s\"", s_current_mode);
        if (written < 0 || written >= (int)(sizeof(payload) - len))
        {
            ESP_LOGE(TAG, "심박수 페이로드 생성 실패(모드)");
            return ESP_FAIL;
        }
        len += written;
    }

    int closing = snprintf(payload + len, sizeof(payload) - len, "}");
    if (closing < 0 || closing >= (int)(sizeof(payload) - len))
    {
        ESP_LOGE(TAG, "심박수 페이로드 생성 실패(닫기)");
        return ESP_FAIL;
    }
    len += closing;

    esp_err_t result = mqtt_publish_locked(CONFIG_WORKOUT_MQTT_TOPIC_HEART_RATE, payload, len, CONFIG_WORKOUT_MQTT_QOS);
    if (result == ESP_OK)
    {
        ESP_LOGI(TAG, "심박수 발행 성공: %s", payload);
    }
    else if (result == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "심박수 발행 실패: MQTT 연결 상태 아님");
    }
    else
    {
        ESP_LOGE(TAG, "심박수 발행 실패: %s", esp_err_to_name(result));
    }
    return result;
}

#else // CONFIG_WORKOUT_MQTT_ENABLED

esp_err_t watch_mqtt_client_init(void)
{
    return ESP_OK;
}

void watch_mqtt_client_deinit(void)
{
}

bool watch_mqtt_client_is_connected(void)
{
    return false;
}

bool watch_mqtt_client_measurement_active(void)
{
    return false;
}

esp_err_t watch_mqtt_client_publish_heart_rate(uint16_t bpm)
{
    (void)bpm;
    return ESP_ERR_INVALID_STATE;
}

#endif // CONFIG_WORKOUT_MQTT_ENABLED
