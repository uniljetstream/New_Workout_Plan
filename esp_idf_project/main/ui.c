/**
 * UI 모듈 구현
 *
 * LVGL UI 초기화 및 이벤트 처리를 담당합니다.
 */

#include "ui.h"
#include "demos/lv_demos.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware.h"
#include "wifi.h"
#include "sensor.h"
#include "watch_mqtt_client.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "UI";

// 하드웨어 객체 (외부에서 초기화됨)
extern st7789_t lcd;
extern cst816s_t touch;

// LVGL 변수
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

// 시계 UI 요소
static lv_obj_t *label_time = NULL;
static lv_obj_t *label_date = NULL;

// 화면 객체
static lv_obj_t *screen_main = NULL;
static lv_obj_t *screen_wifi = NULL;
static lv_obj_t *screen_password = NULL;
static lv_obj_t *screen_sensor = NULL;

// WiFi 관련 변수
static wifi_scan_result_t wifi_scan_results[MAX_WIFI_SCAN_RESULTS];
static uint16_t wifi_scan_count = 0;
static char selected_ssid[33] = {0};
static lv_obj_t *password_textarea = NULL;

// WiFi 상태 플래그
static volatile bool wifi_status_changed = false;
static volatile bool wifi_last_status = false;
static volatile bool wifi_color_changed = false;  // 색상 변경 상태 플래그 추가

// 심박수 표시용 변수
static lv_obj_t *label_heart_rate_sensor = NULL;
static volatile bool heart_rate_updated = false;
static volatile uint16_t heart_rate_latest_bpm = 0;

// ============================================================================
// 내부 함수 (Private Functions)
// ============================================================================

/**
 * 시계 업데이트 타이머 콜백
 */
static void update_clock_cb(lv_timer_t *timer)
{
    time_t now;
    struct tm timeinfo;
    char time_str[32];
    char date_str[64];
    const char *weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    time(&now);
    localtime_r(&now, &timeinfo);

    // 시간 포맷: HH:MM:ss
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    lv_label_set_text(label_time, time_str);

    // 날짜 포맷: yyyy-mm-dd 요일
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d %s",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             weekdays[timeinfo.tm_wday]);
    lv_label_set_text(label_date, date_str);
}

/**
 * LVGL 타이머 콜백 함수
 * 2ms마다 호출되어 LVGL 시간을 증가시킵니다.
 */
static void lvgl_tick_timer_cb(void *arg)
{
    lv_tick_inc(2);
}

/**
 * 디스플레이 출력 콜백 함수
 * LVGL이 그린 픽셀 데이터를 LCD로 전송합니다.
 */
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    st7789_set_window(&lcd, area->x1, area->y1, area->x2, area->y2);
    st7789_write_data(&lcd, (uint8_t *)color_p, w * h * 2);

    lv_disp_flush_ready(disp_drv);
}

/**
 * 터치 입력 읽기 콜백 함수
 * 터치 센서에서 좌표를 읽어 LVGL에 전달합니다.
 */
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    if (cst816s_read(&touch))
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch.data.x;
        data->point.y = touch.data.y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

/**
 * 배경색 초기화 타이머 콜백
 */
static void reset_bg_color_cb(lv_timer_t *timer)
{
    // 배경색을 원래대로 (기본 흰색)
    lv_obj_set_style_bg_color(screen_main, lv_color_white(), 0);
    wifi_color_changed = false;  // 색상 변경 상태 초기화
    lv_timer_del(timer);  // 타이머 삭제
}

/**
 * LVGL 태스크 함수
 * 10ms마다 LVGL 이벤트를 처리합니다. (5ms에서 10ms로 변경하여 안정성 향상)
 */
static void lvgl_task(void *pvParameter)
{
    static uint32_t task_counter = 0;
    
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));  // 5ms에서 10ms로 변경
        task_counter++;

        // 메모리 모니터링 (100번마다)
        if (task_counter % 100 == 0) {
            ESP_LOGI(TAG, "LVGL 태스크 실행 중 - 사용 가능한 힙: %d bytes", esp_get_free_heap_size());
        }

        // WiFi 상태 변경 확인 (백업 처리)
        if (wifi_status_changed && !wifi_color_changed)
        {
            ESP_LOGI(TAG, "🔍 LVGL 태스크에서 WiFi 상태 변경 감지됨!");
            wifi_status_changed = false;
            
            // 현재 화면이 메인 화면일 때만 배경색 변경
            if (lv_scr_act() == screen_main)
            {
                ESP_LOGI(TAG, "🔍 LVGL 태스크에서 메인 화면에서 WiFi 상태 처리: %s", wifi_last_status ? "연결됨" : "연결 안됨");
                
                // 배경색 변경 (연결 성공: 초록, 실패: 빨강)
                if (wifi_last_status)
                {
                    ESP_LOGI(TAG, "✅ LVGL 태스크에서 배경색을 초록색으로 변경");
                    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x0000FF), 0);  // 초록색
                    wifi_color_changed = true;  // 색상 변경 상태 설정
                    // 성공 시에도 2초 후 배경색을 원래대로 복구
                    lv_timer_create(reset_bg_color_cb, 2000, NULL);
                }
                else
                {
                    ESP_LOGI(TAG, "❌ LVGL 태스크에서 배경색을 빨간색으로 변경");
                    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x00FF00), 0);  // 빨간색
                    wifi_color_changed = true;  // 색상 변경 상태 설정
                    // 실패 시에도 2초 후 배경색을 원래대로 복구
                    lv_timer_create(reset_bg_color_cb, 2000, NULL);
                }
            }
            else
            {
                ESP_LOGI(TAG, "🔍 LVGL 태스크에서 현재 화면이 메인 화면이 아님: %p", lv_scr_act());
            }
        }

        // 심박수 업데이트 확인
        if (heart_rate_updated)
        {
            heart_rate_updated = false;
            uint16_t bpm = heart_rate_latest_bpm;
            if (label_heart_rate_sensor != NULL)
            {
                lv_label_set_text_fmt(label_heart_rate_sensor, "%u bpm", bpm);
            }
        }

        // LVGL 타이머 핸들러 실행 (안전하게)
        lv_timer_handler();
    }
}

/**
 * WiFi 연결 상태 콜백 (WiFi 이벤트 스레드에서 호출됨)
 */
static void wifi_status_callback(bool connected, const char *message)
{
    ESP_LOGI(TAG, "🔔 WiFi 상태 콜백 호출됨!");
    ESP_LOGI(TAG, "🔔 연결 상태: %s", connected ? "연결됨" : "연결 안됨");
    ESP_LOGI(TAG, "🔔 메시지: %s", message);

    // 플래그 설정 (LVGL 태스크에서 처리하도록)
    wifi_last_status = connected;
    wifi_status_changed = true;
    
    ESP_LOGI(TAG, "🔍 설정된 wifi_last_status: %s", wifi_last_status ? "true" : "false");
    ESP_LOGI(TAG, "🔍 설정된 wifi_status_changed: %s", wifi_status_changed ? "true" : "false");
    
    // 직접 화면 색상 변경하지 않음 - LVGL 태스크에서만 처리
}

/**
 * 비밀번호 Connect 버튼 이벤트 핸들러
 */
static void password_connect_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        const char *password = lv_textarea_get_text(password_textarea);
        ESP_LOGI(TAG, "=== WiFi 연결 시도 시작 ===");
        ESP_LOGI(TAG, "선택된 SSID: %s", selected_ssid);
        ESP_LOGI(TAG, "입력된 비밀번호 길이: %d", strlen(password));
        ESP_LOGI(TAG, "비밀번호 (처음 10자): %.10s", password);

        // WiFi 연결
        esp_err_t result = wifi_connect(selected_ssid, password, wifi_status_callback);
        ESP_LOGI(TAG, "WiFi 연결 함수 결과: %s", esp_err_to_name(result));
        ESP_LOGI(TAG, "=== WiFi 연결 시도 완료 ===");

        // 메인 화면으로 돌아가기
        lv_scr_load(screen_main);
    }
}

/**
 * 비밀번호 Cancel 버튼 이벤트 핸들러
 */
static void password_cancel_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        ESP_LOGI(TAG, "비밀번호 입력 취소");
        lv_scr_load(screen_wifi);
    }
}

/**
 * 비밀번호 입력 화면 표시
 */
static void show_password_screen(void)
{
    ESP_LOGI(TAG, "비밀번호 입력 화면 표시");

    // 비밀번호 화면 생성
    if (screen_password != NULL)
    {
        lv_obj_del(screen_password);
    }

    screen_password = lv_obj_create(NULL);

    // 제목
    lv_obj_t *title = lv_label_create(screen_password);
    lv_label_set_text_fmt(title, "Password for:\n%s", selected_ssid);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    // 비밀번호 입력 텍스트 영역
    password_textarea = lv_textarea_create(screen_password);
    lv_obj_set_size(password_textarea, 200, 40);
    lv_obj_align(password_textarea, LV_ALIGN_TOP_MID, 0, 50);
    lv_textarea_set_password_mode(password_textarea, true);
    lv_textarea_set_one_line(password_textarea, true);
    lv_textarea_set_text(password_textarea, "");

    // 키보드
    lv_obj_t *kb = lv_keyboard_create(screen_password);
    lv_keyboard_set_textarea(kb, password_textarea);
    lv_obj_set_size(kb, 240, 140);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Connect 버튼
    lv_obj_t *btn_connect = lv_btn_create(screen_password);
    lv_obj_set_size(btn_connect, 90, 35);
    lv_obj_align(btn_connect, LV_ALIGN_BOTTOM_LEFT, 10, -150);
    lv_obj_add_event_cb(btn_connect, password_connect_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_connect = lv_label_create(btn_connect);
    lv_label_set_text(label_connect, "Connect");
    lv_obj_center(label_connect);

    // Cancel 버튼
    lv_obj_t *btn_cancel = lv_btn_create(screen_password);
    lv_obj_set_size(btn_cancel, 90, 35);
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_RIGHT, -10, -150);
    lv_obj_add_event_cb(btn_cancel, password_cancel_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(label_cancel, "Cancel");
    lv_obj_center(label_cancel);

    // 비밀번호 화면으로 전환
    lv_scr_load(screen_password);
}

/**
 * 뒤로가기 버튼 이벤트 핸들러
 */
static void back_button_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        ESP_LOGI(TAG, "뒤로가기 버튼 클릭");
        lv_scr_load(screen_main);
    }
}

/**
 * WiFi 목록 항목 클릭 이벤트 핸들러
 */
static void wifi_list_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        uint32_t index = (uint32_t)(uintptr_t)lv_event_get_user_data(e);

        if (index < wifi_scan_count)
        {
            strncpy(selected_ssid, wifi_scan_results[index].ssid, sizeof(selected_ssid) - 1);
            ESP_LOGI(TAG, "WiFi 선택됨: %s (Auth: %d)", selected_ssid, wifi_scan_results[index].authmode);

            // Open 네트워크면 바로 연결
            if (wifi_scan_results[index].authmode == WIFI_AUTH_OPEN)
            {
                ESP_LOGI(TAG, "Open 네트워크 - 바로 연결");
                wifi_connect(selected_ssid, NULL, wifi_status_callback);
            }
            else
            {
                // 비밀번호 입력 필요 - 키보드 화면 표시
                ESP_LOGI(TAG, "보안 네트워크 - 비밀번호 필요");
                show_password_screen();
            }
        }
    }
}

/**
 * WiFi 화면으로 전환
 */
static void show_wifi_screen(void)
{
    ESP_LOGI(TAG, "WiFi 화면 표시");

    // WiFi 스캔
    wifi_scan_count = wifi_scan(wifi_scan_results, MAX_WIFI_SCAN_RESULTS);

    // WiFi 화면 생성
    if (screen_wifi != NULL)
    {
        lv_obj_del(screen_wifi);
    }

    screen_wifi = lv_obj_create(NULL);

    // 제목 라벨
    lv_obj_t *title = lv_label_create(screen_wifi);
    lv_label_set_text(title, "WiFi Networks");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // WiFi 목록 컨테이너
    lv_obj_t *list = lv_obj_create(screen_wifi);
    lv_obj_set_size(list, 220, 200);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    // WiFi 목록 항목 생성
    for (int i = 0; i < wifi_scan_count; i++)
    {
        lv_obj_t *btn = lv_btn_create(list);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_height(btn, 40);
        lv_obj_add_event_cb(btn, wifi_list_event_handler, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        char label_text[64];
        const char *auth_str = wifi_scan_results[i].authmode == WIFI_AUTH_OPEN ? "Open" : "Secure";
        snprintf(label_text, sizeof(label_text), "%s (%s)",
                 wifi_scan_results[i].ssid, auth_str);
        lv_label_set_text(label, label_text);
        lv_obj_center(label);
    }

    // 뒤로가기 버튼
    lv_obj_t *btn_back = lv_btn_create(screen_wifi);
    lv_obj_set_size(btn_back, 100, 40);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn_back, back_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Back");
    lv_obj_center(label_back);

    // WiFi 화면으로 전환
    lv_scr_load(screen_wifi);
}

/**
 * WiFi Connect 버튼 클릭 이벤트 핸들러
 */
static void left_button_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        ESP_LOGI(TAG, "WiFi Connect 버튼 클릭!");
        show_wifi_screen();
    }
}

/**
 * 심박 센서 화면 뒤로가기 버튼 이벤트 핸들러
 */
static void sensor_back_button_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        ESP_LOGI(TAG, "심박 센서 화면 - 뒤로가기");
        label_heart_rate_sensor = NULL;
        lv_scr_load(screen_main);
    }
}

/**
 * 심박 센서 화면 표시
 */
static void show_sensor_screen(void)
{
    ESP_LOGI(TAG, "심박 센서 화면 표시");

    label_heart_rate_sensor = NULL;

    if (screen_sensor != NULL)
    {
        lv_obj_del(screen_sensor);
    }

    screen_sensor = lv_obj_create(NULL);

    // 제목
    lv_obj_t *title = lv_label_create(screen_sensor);
    lv_label_set_text(title, "Heart Rate");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    // 심박수 라벨
    label_heart_rate_sensor = lv_label_create(screen_sensor);
    lv_label_set_text(label_heart_rate_sensor, "-- bpm");
    lv_obj_align(label_heart_rate_sensor, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_text_font(label_heart_rate_sensor, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_align(label_heart_rate_sensor, LV_TEXT_ALIGN_CENTER, 0);

    uint16_t bpm = heart_rate_sensor_get_bpm();
    if (bpm > 0)
    {
        lv_label_set_text_fmt(label_heart_rate_sensor, "%u bpm", bpm);
    }

    // 안내 문구
    lv_obj_t *hint = lv_label_create(screen_sensor);
    lv_label_set_text(hint, "Place finger and hold steady.");
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    // 뒤로가기 버튼
    lv_obj_t *btn_back = lv_btn_create(screen_sensor);
    lv_obj_set_size(btn_back, 120, 45);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn_back, sensor_back_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Back");
    lv_obj_center(label_back);

    lv_scr_load(screen_sensor);
}

/**
 * Sensor 버튼 클릭 이벤트 핸들러
 */
static void sensor_button_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        ESP_LOGI(TAG, "Sensor 버튼 클릭!");
        show_sensor_screen();
    }
}

/**
 * 심박 센서 데이터 콜백
 */
static void heart_rate_sensor_callback(uint16_t bpm, void *ctx)
{
    (void)ctx;
    heart_rate_latest_bpm = bpm;
    heart_rate_updated = true;

    if (bpm > 0)
    {
        esp_err_t ret = watch_mqtt_client_publish_heart_rate(bpm);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGW(TAG, "MQTT 심박수 전송 실패: %s", esp_err_to_name(ret));
        }
    }
}

static void create_default_ui(void)
{
    // 메인 화면 생성
    screen_main = lv_obj_create(NULL);

    // 시간 라벨 생성 (큰 글씨)
    label_time = lv_label_create(screen_main);
    lv_label_set_text(label_time, "00:00:00");
    lv_obj_align(label_time, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_align(label_time, LV_TEXT_ALIGN_CENTER, 0);

    // 날짜 라벨 생성
    label_date = lv_label_create(screen_main);
    lv_label_set_text(label_date, "0000-00-00 요일");
    lv_obj_align(label_date, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_text_font(label_date, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_align(label_date, LV_TEXT_ALIGN_CENTER, 0);

    // 시계 업데이트 타이머 생성 (1초마다)
    lv_timer_create(update_clock_cb, 1000, NULL);

    // 왼쪽 버튼(wifi 연결버튼) 생성
    lv_obj_t *btn_left = lv_btn_create(screen_main);
    lv_obj_set_size(btn_left, 100, 50);
    lv_obj_align(btn_left, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(btn_left, left_button_event_handler, LV_EVENT_CLICKED, NULL);

    // 왼쪽 버튼 라벨
    lv_obj_t *label_left = lv_label_create(btn_left);
    lv_label_set_text(label_left, "WiFi");
    lv_obj_center(label_left);

    // Sensor 버튼 생성
    lv_obj_t *btn_sensor = lv_btn_create(screen_main);
    lv_obj_set_size(btn_sensor, 100, 50);
    lv_obj_align(btn_sensor, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_event_cb(btn_sensor, sensor_button_event_handler, LV_EVENT_CLICKED, NULL);

    // Sensor 버튼 라벨
    lv_obj_t *label_sensor = lv_label_create(btn_sensor);
    lv_label_set_text(label_sensor, "Sensor");
    lv_obj_center(label_sensor);

    // 메인 화면 로드
    lv_scr_load(screen_main);
}


// ============================================================================
// 공개 함수 (Public Functions)
// ============================================================================

esp_err_t ui_init(void)
{
    ESP_LOGI(TAG, "LVGL 초기화 시작...");

    // LVGL 초기화
    lv_init();

    // 디스플레이 버퍼 할당 (화면의 1/4 크기, 더블 버퍼링)
    buf1 = heap_caps_malloc(240 * 280 / 4 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    buf2 = heap_caps_malloc(240 * 280 / 4 * sizeof(lv_color_t), MALLOC_CAP_DMA);

    if (!buf1 || !buf2)
    {
        ESP_LOGE(TAG, "디스플레이 버퍼 할당 실패");
        return ESP_ERR_NO_MEM;
    }

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 240 * 280 / 4);

    // 디스플레이 드라이버 등록
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 280;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // 터치 입력 드라이버 등록
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // LVGL 타이머 생성 (2ms 주기)
    const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &lvgl_tick_timer_cb, .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, 2000);

    // 기본 UI 생성
    create_default_ui();

    // 초기 심박수 반영 및 콜백 등록
    heart_rate_sensor_register_callback(heart_rate_sensor_callback, NULL);

    ESP_LOGI(TAG, "LVGL 초기화 완료");
    return ESP_OK;
}

esp_err_t ui_start_task(void)
{
    ESP_LOGI(TAG, "LVGL 태스크 시작 중...");

    // 스택 크기를 늘리고 우선순위를 낮춤 (리셋 방지)
    BaseType_t ret = xTaskCreate(lvgl_task, "lvgl_task", 16384, NULL, 3, NULL);
    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "LVGL 태스크 생성 실패");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LVGL 태스크 생성 완료 (스택: 16KB, 우선순위: 3)");
    return ESP_OK;
}
