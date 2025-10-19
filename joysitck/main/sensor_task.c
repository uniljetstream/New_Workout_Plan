/* 센서 태스크 구현 */

#include "sensor_task.h"
#include "mqtt_handler.h"
#include "mpu6050.h"
#include "config.h"
#include "airmouse.h"
#include "button_handler.h"  // 버튼 핸들러 추가
#include "vibration_motor.h"  // 진동모터 헤더 추가

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>  // sqrt 함수 사용을 위해

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

// 조이스틱 진동 관련 변수
static mpu6050_data_t previous_sensor_data = {0};  // 이전 센서 데이터
static int64_t last_vibration_time = 0;            // 마지막 진동 시간
static bool vibration_initialized = false;         // 진동 초기화 상태

/**
 * @brief 조이스틱 진동 체크 및 실행
 * 센서 값의 변화량을 계산하여 임계값을 넘으면 진동
 */
static void check_joystick_vibration(const mpu6050_data_t *current_data)
{
#if JOYSTICK_VIBRATION_ENABLED
    if (!vibration_initialized) {
        // 첫 번째 데이터는 기준점으로 설정
        previous_sensor_data = *current_data;
        vibration_initialized = true;
        return;
    }

    // 현재 시간
    int64_t current_time = esp_timer_get_time() / 1000;  // 밀리초 단위

    // 쿨다운 시간 체크
    if (current_time - last_vibration_time < VIBRATION_COOLDOWN_MS) {
        return;
    }

    // 가속도 변화량 계산 (벡터 크기)
    float accel_delta_x = current_data->accel_x - previous_sensor_data.accel_x;
    float accel_delta_y = current_data->accel_y - previous_sensor_data.accel_y;
    float accel_delta_z = current_data->accel_z - previous_sensor_data.accel_z;
    float accel_change_magnitude = sqrt(accel_delta_x * accel_delta_x + 
                                       accel_delta_y * accel_delta_y + 
                                       accel_delta_z * accel_delta_z);

    // 자이로 변화량 계산 (벡터 크기)
    float gyro_delta_x = current_data->gyro_x - previous_sensor_data.gyro_x;
    float gyro_delta_y = current_data->gyro_y - previous_sensor_data.gyro_y;
    float gyro_delta_z = current_data->gyro_z - previous_sensor_data.gyro_z;
    float gyro_change_magnitude = sqrt(gyro_delta_x * gyro_delta_x + 
                                      gyro_delta_y * gyro_delta_y + 
                                      gyro_delta_z * gyro_delta_z);

    // 진동 조건 체크
    bool should_vibrate = false;
    
    if (accel_change_magnitude > VIBRATION_THRESHOLD_ACCEL) {
        ESP_LOGI(TAG_SENSOR, "Accel change triggered vibration: %.3f > %.3f", 
                 accel_change_magnitude, VIBRATION_THRESHOLD_ACCEL);
        should_vibrate = true;
    }
    
    if (gyro_change_magnitude > VIBRATION_THRESHOLD_GYRO) {
        ESP_LOGI(TAG_SENSOR, "Gyro change triggered vibration: %.2f > %.2f", 
                 gyro_change_magnitude, VIBRATION_THRESHOLD_GYRO);
        should_vibrate = true;
    }

    // 진동 실행 (센서 값에 따른 강도 계산)
    if (should_vibrate) {
        // 가속도와 자이로 값의 크기 계산
        float accel_magnitude = sqrt(accel_delta_x * accel_delta_x + 
                                   accel_delta_y * accel_delta_y + 
                                   accel_delta_z * accel_delta_z);
        float gyro_magnitude = sqrt(gyro_delta_x * gyro_delta_x + 
                                  gyro_delta_y * gyro_delta_y + 
                                  gyro_delta_z * gyro_delta_z);
        
        // 진동 강도 계산 (0-100 범위)
        uint8_t vibration_intensity = 0;
        if (accel_magnitude > VIBRATION_THRESHOLD_ACCEL) {
            vibration_intensity = (uint8_t)((accel_magnitude / VIBRATION_THRESHOLD_ACCEL) * 50);
        }
        if (gyro_magnitude > VIBRATION_THRESHOLD_GYRO) {
            uint8_t gyro_intensity = (uint8_t)((gyro_magnitude / VIBRATION_THRESHOLD_GYRO) * 50);
            if (gyro_intensity > vibration_intensity) {
                vibration_intensity = gyro_intensity;
            }
        }
        
        // 최대 강도 제한
        if (vibration_intensity > 100) vibration_intensity = 100;
        if (vibration_intensity < 20) vibration_intensity = 20;  // 최소 진동 강도
        
        ESP_LOGI(TAG_SENSOR, "Triggering joystick vibration! Intensity: %d%% (Accel: %.2f, Gyro: %.2f)", 
                 vibration_intensity, accel_magnitude, gyro_magnitude);
        
        vibration_motor_start(vibration_intensity, 500);  // 0.5초 진동
        last_vibration_time = current_time;
    }

    // 현재 데이터를 이전 데이터로 저장
    previous_sensor_data = *current_data;
#endif
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

    // MPU6050 초기화 (이미 초기화되었는지 확인)
    if (!mpu6050_initialized) {
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
    } else {
        ESP_LOGI(TAG_SENSOR, "MPU6050 already initialized, reusing existing connection");
    }

    while (sensor_task_running) {
        // Watchdog 피드 (태스크가 살아있음을 알림)
        vTaskDelay(pdMS_TO_TICKS(1));
        
        // 메모리 사용량 모니터링 - 비활성화 (힙 메모리 해제로 인한 끊김 방지)
        // static uint32_t last_memory_check = 0;
        // uint32_t current_time = esp_timer_get_time() / 1000;
        // if (current_time - last_memory_check > 30000) {
        //     uint32_t free_heap = esp_get_free_heap_size();
        //     if (free_heap > 50000) {
        //         ESP_LOGI(TAG_SENSOR, "Free heap: %" PRIu32 " bytes", free_heap);
        //     } else {
        //         ESP_LOGW(TAG_SENSOR, "Low memory warning: %" PRIu32 " bytes", free_heap);
        //     }
        //     last_memory_check = current_time;
        // }

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
                
                // 조이스틱 진동 체크 (센서 데이터 읽기 성공 후) - 비활성화됨
                // check_joystick_vibration(&sensor_data_cache);
                
                // 현재 모드에 따라 다른 데이터 전송
                airmouse_mode_t current_mode = airmouse_get_mode();
                
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
        ESP_LOGI(TAG_SENSOR, "Sensor task already running, restarting...");
        sensor_task_stop();  // 기존 태스크 중지
        vTaskDelay(pdMS_TO_TICKS(100));  // 잠시 대기
    }

    sensor_task_running = true;
    xTaskCreate(sensor_task, "sensor_task", 30720, NULL, 4, &sensor_task_handle);  // 스택 크기 증가: 20480 -> 30720
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
