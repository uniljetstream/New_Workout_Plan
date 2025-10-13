/* 센서 태스크 구현 */

#include "sensor_task.h"
#include "mqtt_handler.h"
#include "mpu6050.h"
#include "config.h"
#include "airmouse.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 센서 데이터 전송 주기 (동적 변경 가능)
static uint32_t publish_interval_ms = DEFAULT_PUBLISH_INTERVAL_MS;

// MPU6050 초기화 상태
static bool mpu6050_initialized = false;

// 센서 태스크 동작 제어
static bool sensor_task_running = false;
static TaskHandle_t sensor_task_handle = NULL;

/**
 * @brief 센서 데이터 읽기 (MPU6050)
 */
bool sensor_read_data(mpu6050_data_t *data)
{
    if (!mpu6050_initialized) {
        ESP_LOGE(TAG_SENSOR, "MPU6050 not initialized");
        return false;
    }

    // MPU6050 센서 데이터 읽기 (재시도 로직 포함)
    esp_err_t ret = ESP_FAIL;
    int retry_count = 0;
    const int max_retries = 2;
    
    while (ret != ESP_OK && retry_count < max_retries) {
        ret = mpu6050_read_data(data);
        if (ret != ESP_OK) {
            retry_count++;
            ESP_LOGW(TAG_SENSOR, "MPU6050 read failed (attempt %d/%d), retrying...", retry_count, max_retries);
            vTaskDelay(pdMS_TO_TICKS(5)); // 5ms 대기 후 재시도
        }
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "Failed to read MPU6050 data after %d retries", max_retries);
        return false;
    }

    return true;
}

/**
 * @brief 전송 주기 설정
 */
void sensor_set_publish_interval(uint32_t interval_ms)
{
    if (interval_ms < 100) {
        ESP_LOGW(TAG_SENSOR, "Interval too short, setting to minimum 100ms");
        publish_interval_ms = 100;
    } else {
        publish_interval_ms = interval_ms;
        ESP_LOGI(TAG_SENSOR, "Publish interval changed to %lu ms", publish_interval_ms);
    }
}

/**
 * @brief 현재 전송 주기 조회
 */
uint32_t sensor_get_publish_interval(void)
{
    return publish_interval_ms;
}

/**
 * @brief 센서 태스크 (주기적으로 센서 값 읽고 발행)
 */
static void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG_SENSOR, "Sensor task started with interval: %lu ms", publish_interval_ms);

    // MPU6050 초기화
    esp_err_t ret = mpu6050_init_sensor();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "MPU6050 initialization failed, halting sensor task");
        sensor_task_running = false;
        sensor_task_handle = NULL;
        vTaskDelete(NULL);  //NULL이면 자기자신 task 삭제
        return;
    }
    mpu6050_initialized = true; //초기화 상태임을 알려주는 flag
    ESP_LOGI(TAG_SENSOR, "MPU6050 initialized successfully");

    while (sensor_task_running) {
        mpu6050_data_t sensor_data;

        // 메모리 사용량 모니터링 (10초마다)
        static uint32_t last_memory_check = 0;
        uint32_t current_time = esp_timer_get_time() / 1000;
        if (current_time - last_memory_check > 10000) {
            ESP_LOGI(TAG_SENSOR, "Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());
            last_memory_check = current_time;
        }

        // 센서 데이터 읽기 (재시도 로직 포함)
        bool data_read_success = false;
        int retry_count = 0;
        const int max_retries = 3;
        
        while (!data_read_success && retry_count < max_retries) {
            if (sensor_read_data(&sensor_data)) {
                data_read_success = true;
                
                // 현재 모드에 따라 다른 데이터 전송
                airmouse_mode_t current_mode = airmouse_get_mode();
                
                if (current_mode == AIRMOUSE_MODE_MOUSE) {
                    // 에어마우스 모드: 센서 데이터를 마우스 데이터로 변환하여 전송
                    mouse_data_t mouse_data;
                    if (airmouse_convert_sensor_to_mouse(&sensor_data, &mouse_data)) {
                        mqtt_publish_airmouse_data(&mouse_data);
                        ESP_LOGD(TAG_SENSOR, "AirMouse data sent: X=%.2f Y=%.2f Scroll=%d", 
                                 mouse_data.mouse_x, mouse_data.mouse_y, mouse_data.scroll_delta);
                    } else {
                        ESP_LOGE(TAG_SENSOR, "Failed to convert sensor data to mouse data");
                    }
                } else {
                    // 센서 모드: 기존 방식으로 센서 데이터 전송
                    mqtt_publish_mpu6050_data(&sensor_data);
                    ESP_LOGD(TAG_SENSOR, "Sensor data sent: Accel X=%.3f Y=%.3f Z=%.3f", 
                             sensor_data.accel_x, sensor_data.accel_y, sensor_data.accel_z);
                }
            } else {
                retry_count++;
                ESP_LOGW(TAG_SENSOR, "Failed to read sensor data (attempt %d/%d)", retry_count, max_retries);
                
                if (retry_count < max_retries) {
                    vTaskDelay(pdMS_TO_TICKS(10)); // 10ms 대기 후 재시도
                }
            }
        }
        
        if (!data_read_success) {
            ESP_LOGE(TAG_SENSOR, "Failed to read sensor data after %d retries", max_retries);
            
            // 센서 재초기화 시도
            ESP_LOGW(TAG_SENSOR, "Attempting sensor reinitialization...");
            mpu6050_deinit();
            vTaskDelay(pdMS_TO_TICKS(100)); // 100ms 대기
            if (mpu6050_init_sensor() == ESP_OK) {
                ESP_LOGI(TAG_SENSOR, "Sensor reinitialized successfully");
            } else {
                ESP_LOGE(TAG_SENSOR, "Sensor reinitialization failed");
            }
        }

        // 동적 전송 주기로 대기
        vTaskDelay(pdMS_TO_TICKS(publish_interval_ms));
    }

    ESP_LOGI(TAG_SENSOR, "Sensor task stopped");
    sensor_task_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * @brief 센서 태스크 시작
 */
void sensor_task_start(void)
{
    if (sensor_task_running) {
        ESP_LOGW(TAG_SENSOR, "Sensor task already running");
        return;
    }

    sensor_task_running = true;
    xTaskCreate(sensor_task, "sensor_task", 16384, NULL, 5, &sensor_task_handle);
    ESP_LOGI(TAG_SENSOR, "Sensor task created with 16KB stack");
}

/**
 * @brief 센서 태스크 중지
 */
void sensor_task_stop(void)
{
    if (!sensor_task_running) {
        ESP_LOGW(TAG_SENSOR, "Sensor task not running");
        return;
    }

    ESP_LOGI(TAG_SENSOR, "Stopping sensor task...");
    sensor_task_running = false;

    // 태스크가 종료될 때까지 대기
    int timeout = 50;  // 5초 타임아웃
    while (sensor_task_handle != NULL && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
    }

    if (sensor_task_handle == NULL) {
        ESP_LOGI(TAG_SENSOR, "Sensor task stopped successfully");
    } else {
        ESP_LOGW(TAG_SENSOR, "Sensor task stop timeout");
    }
}
