/* Wi-Fi 핸들러 구현 */

#include "wifi_handler.h"
#include "config.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

// Wi-Fi 연결 상태 관리
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;

/**
 * @brief Wi-Fi 이벤트 핸들러
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)   //wifi 드라이버가 시작되면 
    {
        esp_wifi_connect(); //공유기 연결 시도
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)   //wifi 연결이 끊겼으면
    {
        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG_WIFI, "Wi-Fi disconnected. Reason: %d", disconnected->reason);
        
        if (s_retry_num < WIFI_MAX_RETRY)   
        {                       // 재시도 횟수가 남았으면
            vTaskDelay(pdMS_TO_TICKS(2000));  // 2초 대기 후 재연결
            esp_wifi_connect(); // 다시 연결 시도
            s_retry_num++;      // 카운터 증가
            ESP_LOGI(TAG_WIFI, "Retry to connect to AP (attempt %d/%d)", s_retry_num,
                     WIFI_MAX_RETRY); // 로그: "3/5번째 재시도 중"
        }
        else    //재시도 횟수가 초과 일시
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);  //이벤트 그룹비트에서 실패 붙이기
            ESP_LOGE(TAG_WIFI, "Failed to connect to Wi-Fi after %d attempts", WIFI_MAX_RETRY);
            ESP_LOGE(TAG_WIFI, "Please check SSID: %s and password", WIFI_SSID);
            ESP_LOGE(TAG_WIFI, "Last disconnect reason: %d", disconnected->reason);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) //공유기로부터 ip주소를 받으면(==wifi 연결이 완료되면)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data; //ip 정보 가져오기
        ESP_LOGI(TAG_WIFI, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG_WIFI, "IP assignment completed successfully");
        s_retry_num = 0;    //재시도 카운터 리셋
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); //이벤트 루프에 성공비트로 표
    }
}

/**
 * @brief Wi-Fi 초기화 및 연결
 */
bool wifi_init_and_connect(void)
{
    /*
    동기화 매커니즘, 여러 이벤트를 비트 단위로 관리함.
    24비트의 이벤트 플래그를 제공하며 여러 태스크가 동시에 접근 가능한. thread-safe
    #define WIFI_CONNECTED_BIT BIT0
    #define WIFI_FAIL_BIT BIT1
    */
    s_wifi_event_group = xEventGroupCreate();

    // tcp/ip 스택을 초기화
    ESP_ERROR_CHECK(esp_netif_init());
    // 기본 이벤트 루프를 생성
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /*
    wifi 스테이션 인터페이스를 생성함.
    netif 객체를 생성하고
    dhcp 클라이트를 활성화함.
    */
    esp_netif_create_default_wifi_sta();

    // 기본 wifi 설정을 가져옴
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // wifi 드라이버 초기화
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 모든 wifi 이벤트 핸들러의 참조 변수, 등록한 핸들러의 제거 할 때 식별하기위한 ID처럼 사용
    esp_event_handler_instance_t instance_any_id;
    // ip 획득 이벤트 핸들러 참조 변수, 등록한 핸들러의 제거 할 때 식별하기위한 ID처럼 사용
    esp_event_handler_instance_t instance_got_ip;
    // 모든 wifi 이벤트(WIFI_EVENT, ESP_EVENT_ANY_ID)가 발생하면 wifi_event_handler를 호출해라. NULL은 전달할 매개변수
    // 없을 나타냄.
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    // ip를 받았을 때만 이벤트(IP_EVENT, IP_EVENT_STA_GOT_IP)가 발생하면 wifi_event_handler를 호출해라. NULL은 전달할
    // 매개변수가 없음을 나타냄.
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
                                                        &instance_got_ip));
    //wifi 설정 구조체를 초기화함.
    wifi_config_t wifi_config = {0};

    // SSID와 비밀번호 설정(config.h에 정의 한 값)
    memcpy(wifi_config.sta.ssid, WIFI_SSID, strlen(WIFI_SSID));
    memcpy(wifi_config.sta.password, WIFI_PASSWORD, strlen(WIFI_PASSWORD));

    // Wi-Fi 보안 설정
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;    //WPA2 사용
    wifi_config.sta.pmf_cfg.capable = true; //PMF 지원 가능
    wifi_config.sta.pmf_cfg.required = false;   //PMF 필수 아님

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));  //스테이션 모드(공유기에 연결하는 클라이언트)로 설정
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));    //설정을 적용
    ESP_ERROR_CHECK(esp_wifi_start());  //wifi 시작. WIFI_EVENT_STA_START  이벤트가 발생되서 wifi_event_handler 가 자동 호출됨.

    ESP_LOGI(TAG_WIFI, "Connecting to Wi-Fi SSID: %s", WIFI_SSID);
    ESP_LOGI(TAG_WIFI, "Wi-Fi Password: %s", WIFI_PASSWORD);
    ESP_LOGI(TAG_WIFI, "Max retry count: %d", WIFI_MAX_RETRY);
    ESP_LOGI(TAG_WIFI, "Wi-Fi mode: Station mode");
    ESP_LOGI(TAG_WIFI, "Wi-Fi scan enabled: %s", CONFIG_ESP32_WIFI_ENABLE_WPA3_SAE ? "WPA3" : "WPA2");

    //wifi 연결을 시도함. wifi_event_handler가 WIFI_CONNECTED_BIT, WIFI_FAIL_BIT를 설정할 때까지 대기.
    /*
    EventBits_t bits = xEventGroupWaitBits(
    s_wifi_event_group,                  // 어떤 게시판?
    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,  // 어떤 스티커 기다림? (성공 or 실패)
    pdFALSE,                             // 대기 후 비트 지우지 마
    pdFALSE,                             // OR 조건 (둘 중 하나만 설정되면 OK)
    portMAX_DELAY);                      // 무한정 대기
    */
    EventBits_t bits =
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(120000));

    if (bits & WIFI_CONNECTED_BIT)  //연결 성공 비트가 설정 되면
    {
        ESP_LOGI(TAG_WIFI, "Wi-Fi connected successfully");
        
        // IP 할당까지 추가 대기 (최대 10초)
        ESP_LOGI(TAG_WIFI, "Waiting for IP assignment...");
        vTaskDelay(pdMS_TO_TICKS(10000));
        
        return true;    //성공반환
    }
    else if (bits & WIFI_FAIL_BIT)  //실패 비트가 설정되었으면
    {
        ESP_LOGE(TAG_WIFI, "Wi-Fi connection failed");
        return false;       //실패 반환
    }

    return false;   //만약을 위한 기본 설정
}
