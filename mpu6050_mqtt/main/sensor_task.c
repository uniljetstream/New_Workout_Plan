/* 센서 태스크 구현 */

#include "sensor_task.h"
#include "mqtt_handler.h"
#include "mpu6050.h"
#include "config.h"
#include "airmouse.h"
#include "button_handler.h"  // 버튼 핸들러 추가
#include "vibration_motor.h" // 진동모터 헤더 추가

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

// 센서 데이터 캐시 (메모리 재사용)
static mpu6050_data_t sensor_data_cache;
static mouse_data_t mouse_data_cache;

// 진동모터 제어를 위한 상태 변수
static bool vibration_enabled = true;  // 진동모터 활성화 상태

/**
 * @brief 자이로센서 임계값 감지 (로그만 출력)
 * 자이로센서 값이 임계값을 넘으면 로그만 출력 (진동은 MQTT 명령으로만 제어)
 */
static void check_gyro_threshold_and_vibrate(const mpu6050_data_t *sensor_data)
{
    if (!vibration_enabled) {
        return;
    }

    // 자이로센서 값의 절댓값 계산 (X, Y, Z 축 중 최대값) - 로그만 출력
    float gyro_x_abs = (sensor_data->gyro_x < 0) ? -sensor_data->gyro_x : sensor_data->gyro_x;
    float gyro_y_abs = (sensor_data->gyro_y < 0) ? -sensor_data->gyro_y : sensor_data->gyro_y;
    float gyro_z_abs = (sensor_data->gyro_z < 0) ? -sensor_data->gyro_z : sensor_data->gyro_z;
    
    float max_gyro = gyro_x_abs;
    if (gyro_y_abs > max_gyro) max_gyro = gyro_y_abs;
    if (gyro_z_abs > max_gyro) max_gyro = gyro_z_abs;

    // 임계값 초과 시 로그만 출력 (진동 제거 - MQTT 명령으로만 제어)
    if (max_gyro > GYRO_THRESHOLD_DEG_S) {
        ESP_LOGD(TAG_SENSOR, "Gyro threshold exceeded! Max gyro: %.1f deg/s (threshold: %.1f)", 
                 max_gyro, GYRO_THRESHOLD_DEG_S);
        ESP_LOGD(TAG_SENSOR, "Gyro values - X: %.1f, Y: %.1f, Z: %.1f deg/s", 
                 sensor_data->gyro_x, sensor_data->gyro_y, sensor_data->gyro_z);
    }
}

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
    const int max_retries = 5;  // 재시도 횟수 증가
    
    while (ret != ESP_OK && retry_count < max_retries) {
        ret = mpu6050_read_data(data);
        if (ret != ESP_OK) {
            retry_count++;
            ESP_LOGW(TAG_SENSOR, "MPU6050 read failed (attempt %d/%d), retrying...", retry_count, max_retries);
            vTaskDelay(pdMS_TO_TICKS(20)); // 20ms 대기 후 재시도 (더 긴 대기)
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

    // 진동모터 초기화
    if (!vibration_motor_init()) {
        ESP_LOGE(TAG_SENSOR, "Vibration motor initialization failed");
        // 진동모터 실패해도 센서 태스크는 계속 실행
    } else {
        ESP_LOGI(TAG_SENSOR, "Vibration motor initialized successfully");
        ESP_LOGI(TAG_SENSOR, "Gyro threshold: %.1f deg/s", GYRO_THRESHOLD_DEG_S);
    }

    while (sensor_task_running) {
        // Watchdog 피드 (태스크가 살아있음을 알림)
        vTaskDelay(pdMS_TO_TICKS(1));
        
        // 메모리 사용량 모니터링 (10초마다)
        static uint32_t last_memory_check = 0;
        uint32_t current_time = esp_timer_get_time() / 1000;
        if (current_time - last_memory_check > 10000) {
            ESP_LOGI(TAG_SENSOR, "Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());
            last_memory_check = current_time;
        }

        // 센서 데이터 읽기 (재시도 로직 포함, 고정 버퍼 사용)
        bool data_read_success = false;
        int retry_count = 0;
        const int max_retries = 3;
        
        // 센서 읽기 시작 로그
        ESP_LOGD(TAG_SENSOR, "Starting sensor data read...");
        
        while (!data_read_success && retry_count < max_retries) {
            if (sensor_read_data(&sensor_data_cache)) {
                data_read_success = true;
                ESP_LOGD(TAG_SENSOR, "Sensor data read successful");
                
                // 현재 모드에 따라 다른 데이터 전송
                airmouse_mode_t current_mode = airmouse_get_mode();
                
                // 자이로센서 임계값 감지 및 진동모터 트리거 (모든 모드에서 동작)
                check_gyro_threshold_and_vibrate(&sensor_data_cache);

                if (current_mode == AIRMOUSE_MODE_MOUSE) {
                    // 에어마우스 모드: 센서 데이터를 마우스 데이터로 변환하여 전송 (고정 버퍼 사용)
                    ESP_LOGD(TAG_SENSOR, "Converting to mouse data...");
                    if (airmouse_convert_sensor_to_mouse(&sensor_data_cache, &mouse_data_cache)) {
                        // 버튼 상태 읽기
                        bool button_pressed = button_is_pressed();

                        ESP_LOGD(TAG_SENSOR, "Publishing airmouse data...");
                        mqtt_publish_airmouse_data(&mouse_data_cache, button_pressed);
                        ESP_LOGD(TAG_SENSOR, "AirMouse data sent: X=%.2f Y=%.2f Scroll=%d Button=%d",
                                 mouse_data_cache.mouse_x, mouse_data_cache.mouse_y, mouse_data_cache.scroll_delta, button_pressed);
                    } else {
                        ESP_LOGE(TAG_SENSOR, "Failed to convert sensor data to mouse data");
                    }
                } else {
                    // 센서 모드: 기존 방식으로 센서 데이터 전송 (고정 버퍼 사용)
                    ESP_LOGD(TAG_SENSOR, "Publishing sensor data...");
                    mqtt_publish_mpu6050_data(&sensor_data_cache);
                    ESP_LOGD(TAG_SENSOR, "Sensor data sent: Accel X=%.3f Y=%.3f Z=%.3f", 
                             sensor_data_cache.accel_x, sensor_data_cache.accel_y, sensor_data_cache.accel_z);
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
    xTaskCreate(sensor_task, "sensor_task", 20480, NULL, 4, &sensor_task_handle);
    ESP_LOGI(TAG_SENSOR, "Sensor task created with 20KB stack, priority 4");
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

/**
 * @brief 진동모터 활성화/비활성화
 */
void sensor_set_vibration_enabled(bool enabled)
{
    vibration_enabled = enabled;
    ESP_LOGI(TAG_SENSOR, "Vibration motor %s", enabled ? "enabled" : "disabled");
}

/**
 * @brief 진동모터 상태 조회
 */
bool sensor_is_vibration_enabled(void)
{
    return vibration_enabled;
}

/**
 * @brief 수동으로 진동모터 트리거
 */
bool sensor_trigger_vibration(void)
{
    if (!vibration_enabled) {
        ESP_LOGW(TAG_SENSOR, "Vibration motor is disabled");
        return false;
    }

    ESP_LOGI(TAG_SENSOR, "Manual vibration trigger");
    return vibration_motor_trigger();
}
