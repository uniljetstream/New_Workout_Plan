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
// #include "esp_mqtt_client.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>

static const char *TAG = "WATCH_MQTT";

static esp_mqtt_client_handle_t s_client = NULL;
static SemaphoreHandle_t s_publish_mutex = NULL;
static bool s_client_started = false;
static bool s_is_connected = false;
static watch_mqtt_command_callback_t s_command_callback = NULL;
static void *s_command_ctx = NULL;

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

static bool extract_json_string(const char *json, const char *key, char *out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0)
    {
        return false;
    }

    char pattern[32];
    int written = snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    if (written < 0 || (size_t)written >= sizeof(pattern))
    {
        return false;
    }

    const char *start = strstr(json, pattern);
    if (!start)
    {
        return false;
    }
    start += written;
    const char *end = strchr(start, '"');
    if (!end)
    {
        return false;
    }

    size_t len = (size_t)(end - start);
    if (len >= out_size)
    {
        len = out_size - 1;
    }

    memcpy(out, start, len);
    out[len] = '\0';
    return true;
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
            int msg_id = esp_mqtt_client_subscribe(s_client,
                                                   CONFIG_WORKOUT_MQTT_TOPIC_COMMAND,
                                                   CONFIG_WORKOUT_MQTT_QOS);
            if (msg_id >= 0)
            {
                ESP_LOGI(TAG, "워치 명령 토픽 구독 성공: %s (msg_id=%d)",
                         CONFIG_WORKOUT_MQTT_TOPIC_COMMAND, msg_id);
            }
            else
            {
                ESP_LOGE(TAG, "워치 명령 토픽 구독 실패: %d", msg_id);
            }
            esp_err_t status_ret = watch_mqtt_client_publish_status("ready");
            if (status_ret != ESP_OK)
            {
                ESP_LOGW(TAG, "워치 초기 상태 전송 실패: %s", esp_err_to_name(status_ret));
            }
        }
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
    {
        size_t topic_len = (size_t)event->topic_len;
        const char *command_topic = CONFIG_WORKOUT_MQTT_TOPIC_COMMAND;
        size_t command_topic_len = strlen(command_topic);

        if (topic_len == command_topic_len &&
            strncmp(event->topic, command_topic, topic_len) == 0)
        {
            char payload[256];
            size_t copy_len = (size_t)event->data_len;
            if (copy_len >= sizeof(payload))
            {
                copy_len = sizeof(payload) - 1;
            }
            memcpy(payload, event->data, copy_len);
            payload[copy_len] = '\0';

            watch_mqtt_command_t command = WATCH_MQTT_COMMAND_UNKNOWN;
            if (strstr(payload, "\"command\":\"start\""))
            {
                command = WATCH_MQTT_COMMAND_START;
            }
            else if (strstr(payload, "\"command\":\"stop\""))
            {
                command = WATCH_MQTT_COMMAND_STOP;
            }

            if (command != WATCH_MQTT_COMMAND_UNKNOWN)
            {
                char mode[32];
                const char *mode_ptr = NULL;
                if (extract_json_string(payload, "mode", mode, sizeof(mode)))
                {
                    mode_ptr = mode;
                }

                ESP_LOGI(TAG, "워치 명령 수신: %s%s%s",
                         (command == WATCH_MQTT_COMMAND_START) ? "start" :
                         (command == WATCH_MQTT_COMMAND_STOP) ? "stop" : "unknown",
                         mode_ptr ? " mode=" : "",
                         mode_ptr ? mode_ptr : "");

                if (s_command_callback)
                {
                    s_command_callback(command, mode_ptr, s_command_ctx);
                }
            }
            else
            {
                ESP_LOGW(TAG, "알 수 없는 명령 페이로드: %s", payload);
            }
        }
        else
        {
            ESP_LOGD(TAG, "수신 토픽: %.*s", event->topic_len, event->topic);
        }
        break;
    }
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
}

bool watch_mqtt_client_is_connected(void)
{
    return s_is_connected;
}

void watch_mqtt_client_register_command_callback(watch_mqtt_command_callback_t callback, void *ctx)
{
    s_command_callback = callback;
    s_command_ctx = ctx;
}

esp_err_t watch_mqtt_client_publish_status(const char *status)
{
    if (!CONFIG_WORKOUT_MQTT_ENABLED)
    {
        return ESP_OK;
    }

    if (!status)
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
        ESP_LOGW(TAG, "Status publish 뮤텍스 획득 실패");
        return ESP_ERR_TIMEOUT;
    }

    char payload[128];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long timestamp_ms = (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;

    int len = snprintf(payload, sizeof(payload),
                       "{\"status\":\"%s\",\"timestamp\":%lld,\"device_id\":\"%s\"}",
                       status,
                       timestamp_ms,
                       CONFIG_WORKOUT_MQTT_CLIENT_ID);

    esp_err_t result = ESP_FAIL;
    if (len > 0 && len < (int)sizeof(payload))
    {
        int msg_id = esp_mqtt_client_publish(s_client,
                                             CONFIG_WORKOUT_MQTT_TOPIC_STATUS,
                                             payload,
                                             len,
                                             CONFIG_WORKOUT_MQTT_QOS,
                                             0);
        if (msg_id >= 0)
        {
            ESP_LOGI(TAG, "워치 상태 발행 성공: %s (msg_id=%d)", payload, msg_id);
            result = ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "워치 상태 발행 실패 (msg_id=%d)", msg_id);
        }
    }
    else
    {
        ESP_LOGE(TAG, "워치 상태 페이로드 생성 실패");
    }

    xSemaphoreGive(s_publish_mutex);
    return result;
}

esp_err_t watch_mqtt_client_publish_heart_rate(uint16_t bpm)
{
    if (!CONFIG_WORKOUT_MQTT_ENABLED)
    {
        return ESP_OK;
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
        ESP_LOGW(TAG, "Publish 뮤텍스 획득 실패");
        return ESP_ERR_TIMEOUT;
    }

    char payload[128];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long timestamp_ms = (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;

    int len = snprintf(payload, sizeof(payload),
                       "{\"heart_rate\":%u,\"timestamp\":%lld,\"device_id\":\"%s\"}",
                       bpm,
                       timestamp_ms,
                       CONFIG_WORKOUT_MQTT_CLIENT_ID);

    esp_err_t result = ESP_FAIL;
    if (len > 0 && len < sizeof(payload))
    {
        int msg_id = esp_mqtt_client_publish(s_client,
                                             CONFIG_WORKOUT_MQTT_TOPIC_HEART_RATE,
                                             payload,
                                             len,
                                             CONFIG_WORKOUT_MQTT_QOS,
                                             0);
        if (msg_id >= 0)
        {
            ESP_LOGI(TAG, "심박수 발행 성공: %s (msg_id=%d)", payload, msg_id);
            result = ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "심박수 발행 실패 (msg_id=%d)", msg_id);
            result = ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "페이로드 생성 실패");
    }

    xSemaphoreGive(s_publish_mutex);
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

void watch_mqtt_client_register_command_callback(watch_mqtt_command_callback_t callback, void *ctx)
{
    (void)callback;
    (void)ctx;
}

esp_err_t watch_mqtt_client_publish_status(const char *status)
{
    (void)status;
    return ESP_ERR_INVALID_STATE;
}

esp_err_t watch_mqtt_client_publish_heart_rate(uint16_t bpm)
{
    (void)bpm;
    return ESP_ERR_INVALID_STATE;
}

#endif // CONFIG_WORKOUT_MQTT_ENABLED
