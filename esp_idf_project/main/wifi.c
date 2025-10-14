/**
 * WiFi 모듈 구현
 *
 * WiFi 스캔, 연결 및 관리 기능을 담당합니다.
 */

#include "wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "WiFi";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define MAX_RETRY 5
#define WIFI_CONNECT_TIMEOUT_MS 15000  // 15초 타임아웃

// NVS 키 정의
#define NVS_NAMESPACE "wifi_config"
#define NVS_KEY_SSID "ssid"
#define NVS_KEY_PASSWORD "password"

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static wifi_status_cb_t s_status_callback = NULL;
static char s_connected_ssid[33] = {0};
static char s_connecting_password[64] = {0};  // 연결 시도 중인 비밀번호 임시 저장
static bool s_is_connected = false;

/**
 * WiFi 자격 증명을 NVS에 저장
 */
static esp_err_t save_wifi_credentials(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS 열기 실패: %s", esp_err_to_name(err));
        return err;
    }

    // SSID 저장
    err = nvs_set_str(nvs_handle, NVS_KEY_SSID, ssid);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "SSID 저장 실패: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // 비밀번호 저장
    if (password != NULL && strlen(password) > 0)
    {
        err = nvs_set_str(nvs_handle, NVS_KEY_PASSWORD, password);
    }
    else
    {
        // 비밀번호가 없으면 NVS에서 삭제
        nvs_erase_key(nvs_handle, NVS_KEY_PASSWORD);
        err = ESP_OK;
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "비밀번호 저장 실패: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // 변경사항 커밋
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS 커밋 실패: %s", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "WiFi 자격 증명 저장 완료: %s", ssid);
    }

    nvs_close(nvs_handle);
    return err;
}

/**
 * NVS에서 WiFi 자격 증명 로드
 */
static esp_err_t load_wifi_credentials(char *ssid, size_t ssid_len, char *password, size_t password_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGI(TAG, "저장된 WiFi 자격 증명 없음");
        }
        else
        {
            ESP_LOGE(TAG, "NVS 열기 실패: %s", esp_err_to_name(err));
        }
        return err;
    }

    // SSID 로드
    size_t required_size = ssid_len;
    err = nvs_get_str(nvs_handle, NVS_KEY_SSID, ssid, &required_size);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGI(TAG, "저장된 SSID 없음");
        }
        else
        {
            ESP_LOGE(TAG, "SSID 로드 실패: %s", esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
        return err;
    }

    // 비밀번호 로드 (선택사항)
    required_size = password_len;
    err = nvs_get_str(nvs_handle, NVS_KEY_PASSWORD, password, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // 비밀번호가 없는 경우 (open network)
        password[0] = '\0';
        err = ESP_OK;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "비밀번호 로드 실패: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "WiFi 자격 증명 로드 완료: %s", ssid);
    nvs_close(nvs_handle);
    return ESP_OK;
}

/**
 * NTP 시간 동기화 알림 콜백
 */
static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "시간 동기화 완료!");
}

/**
 * NTP 시간 동기화 초기화
 */
static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "SNTP 초기화 중...");

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    // 타임존 설정 (한국 시간: UTC+9)
    setenv("TZ", "KST-9", 1);
    tzset();
}

/**
 * WiFi 연결 타임아웃 처리 태스크
 */
static void wifi_connect_timeout_task(void *pvParameters)
{
    // 타임아웃까지 대기
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdTRUE,
                                          pdFALSE,
                                          pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));
    
    // 타임아웃 발생 (연결 성공/실패 이벤트가 없음)
    if ((bits & (WIFI_CONNECTED_BIT | WIFI_FAIL_BIT)) == 0) {
        ESP_LOGW(TAG, "WiFi 연결 타임아웃 (%dms)", WIFI_CONNECT_TIMEOUT_MS);
        
        // 연결 시도 중단
        esp_wifi_disconnect();
        s_is_connected = false;
        s_retry_num = 0;
        
        // 실패 콜백 호출
        if (s_status_callback) {
            ESP_LOGI(TAG, "🔔 타임아웃으로 인한 연결 실패 콜백 호출");
            s_status_callback(false, "연결 타임아웃");
        }
    }
    
    // 태스크 종료
    vTaskDelete(NULL);
}

/**
 * WiFi 이벤트 핸들러
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // 시작 시 자동 연결하지 않음
        ESP_LOGI(TAG, "WiFi STA 시작됨");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi 연결 해제됨 (재시도 횟수: %d)", s_retry_num);
        
        // 연결 시도 중일 때만 재시도
        if (s_retry_num > 0 && s_retry_num < MAX_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "재연결 시도 중... (%d/%d)", s_retry_num, MAX_RETRY);
        }
        else if (s_retry_num >= MAX_RETRY)
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            s_is_connected = false;
            s_retry_num = 0;
            ESP_LOGE(TAG, "WiFi 연결 실패 - 최대 재시도 횟수(%d) 초과", MAX_RETRY);
            
            // 연결 실패 콜백 호출
            if (s_status_callback)
            {
                ESP_LOGI(TAG, "🔔 연결 실패 콜백 호출");
                s_status_callback(false, "연결 실패 - 최대 재시도 횟수 초과");
            }
        }
        else
        {
            ESP_LOGI(TAG, "WiFi 연결 해제됨 (재시도하지 않음)");
            
            // 연결 시도 중이었다면 실패로 처리
            if (s_status_callback && s_retry_num == 0) {
                ESP_LOGI(TAG, "🔔 연결 해제로 인한 실패 콜백 호출");
                s_status_callback(false, "연결 해제됨");
            }
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "🎉 WiFi 연결 성공! IP 주소: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "연결된 SSID: %s", s_connected_ssid);
        
        s_retry_num = 0;
        s_is_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // WiFi 자격 증명 저장
        save_wifi_credentials(s_connected_ssid, s_connecting_password);

        // NTP 시간 동기화 시작
        initialize_sntp();

        // 연결 성공 콜백 호출
        if (s_status_callback)
        {
            ESP_LOGI(TAG, "🔔 연결 성공 콜백 호출");
            s_status_callback(true, "연결 성공");
        }
    }
}

esp_err_t wifi_init(void)
{
    ESP_LOGI(TAG, "WiFi 초기화 시작...");

    // NVS 초기화
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 이벤트 그룹 생성
    s_wifi_event_group = xEventGroupCreate();

    // 네트워크 인터페이스 초기화
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // WiFi 초기화
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 이벤트 핸들러 등록
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &wifi_event_handler,
                                                         NULL,
                                                         NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                         IP_EVENT_STA_GOT_IP,
                                                         &wifi_event_handler,
                                                         NULL,
                                                         NULL));

    // WiFi 모드 설정
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi 초기화 완료");
    return ESP_OK;
}

uint16_t wifi_scan(wifi_scan_result_t *results, uint16_t max_results)
{
    ESP_LOGI(TAG, "WiFi 스캔 시작...");

    // 스캔 시작
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 150,
            },
        },
    };

    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi 스캔 시작 실패: %s", esp_err_to_name(ret));
        return 0;
    }

    // 스캔 결과 가져오기
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count == 0)
    {
        ESP_LOGI(TAG, "스캔된 AP 없음");
        return 0;
    }

    wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (ap_info == NULL)
    {
        ESP_LOGE(TAG, "메모리 할당 실패");
        return 0;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));

    // 결과 복사
    uint16_t count = ap_count < max_results ? ap_count : max_results;
    for (int i = 0; i < count; i++)
    {
        strncpy(results[i].ssid, (char *)ap_info[i].ssid, 32);
        results[i].ssid[32] = '\0';
        results[i].rssi = ap_info[i].rssi;
        results[i].authmode = ap_info[i].authmode;
        results[i].channel = ap_info[i].primary;

        ESP_LOGI(TAG, "AP[%d]: %s (RSSI: %d, Auth: %d)",
                 i, results[i].ssid, results[i].rssi, results[i].authmode);
    }

    free(ap_info);
    ESP_LOGI(TAG, "스캔 완료: %d개 AP 발견", count);

    return count;
}

esp_err_t wifi_connect(const char *ssid, const char *password, wifi_status_cb_t callback)
{
    ESP_LOGI(TAG, "WiFi 연결 시도: %s", ssid);
    ESP_LOGI(TAG, "비밀번호 길이: %d", password ? strlen(password) : 0);
    ESP_LOGI(TAG, "비밀번호 존재 여부: %s", password ? "있음" : "없음");

    // 이미 같은 SSID에 연결되어 있는지 확인
    if (s_is_connected && strcmp(s_connected_ssid, ssid) == 0) {
        ESP_LOGI(TAG, "이미 %s에 연결되어 있습니다. 재연결하지 않습니다.", ssid);
        if (callback) {
            callback(true, "이미 연결됨");
        }
        return ESP_OK;
    }

    s_status_callback = callback;
    s_retry_num = 1;  // 첫 번째 연결 시도 시작

    // 이벤트 비트 초기화
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    // WiFi 설정
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);

    if (password != NULL && strlen(password) > 0)
    {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
        ESP_LOGI(TAG, "비밀번호 설정됨 (길이: %d)", strlen(password));
    }
    else
    {
        ESP_LOGI(TAG, "Open 네트워크로 설정");
    }

    wifi_auth_mode_t auth_mode = (password != NULL && strlen(password) > 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    wifi_config.sta.threshold.authmode = auth_mode;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    ESP_LOGI(TAG, "인증 모드: %d", auth_mode);

    // SSID와 비밀번호 저장 (연결 성공 시 NVS에 저장하기 위해)
    strncpy(s_connected_ssid, ssid, sizeof(s_connected_ssid) - 1);
    if (password != NULL && strlen(password) > 0)
    {
        strncpy(s_connecting_password, password, sizeof(s_connecting_password) - 1);
    }
    else
    {
        s_connecting_password[0] = '\0';
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "WiFi 연결 명령 전송 완료, 비동기 연결 대기 중...");

    // 타임아웃 처리 태스크 생성
    xTaskCreate(wifi_connect_timeout_task, "wifi_timeout", 2048, NULL, 5, NULL);

    // 비동기 연결 - 이벤트 핸들러에서 콜백 호출
    return ESP_OK;
}

esp_err_t wifi_disconnect(void)
{
    ESP_LOGI(TAG, "WiFi 연결 해제");
    s_is_connected = false;
    memset(s_connected_ssid, 0, sizeof(s_connected_ssid));
    return esp_wifi_disconnect();
}

bool wifi_is_connected(void)
{
    return s_is_connected;
}

const char *wifi_get_connected_ssid(void)
{
    return s_connected_ssid;
}

esp_err_t wifi_auto_connect(wifi_status_cb_t callback)
{
    char ssid[33] = {0};
    char password[64] = {0};

    // NVS에서 저장된 자격 증명 로드
    esp_err_t err = load_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password));
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "자동 연결 실패: 저장된 자격 증명 없음");
        return ESP_ERR_NOT_FOUND;
    }

    // 로드된 자격 증명으로 연결 시도
    ESP_LOGI(TAG, "저장된 자격 증명으로 자동 연결 시도: %s", ssid);
    return wifi_connect(ssid, password, callback);
}

esp_err_t wifi_clear_saved_credentials(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS 열기 실패: %s", esp_err_to_name(err));
        return err;
    }

    // SSID와 비밀번호 삭제
    nvs_erase_key(nvs_handle, NVS_KEY_SSID);
    nvs_erase_key(nvs_handle, NVS_KEY_PASSWORD);

    // 변경사항 커밋
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS 커밋 실패: %s", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "저장된 WiFi 자격 증명 삭제 완료");
    }

    nvs_close(nvs_handle);
    return err;
}
