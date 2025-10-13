/* 센서 태스크 구현 */

#include "sensor_task.h"
#include "mqtt_handler.h"
#include "mpu6050.h"
#include "config.h"

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 센서 데이터 전송 주기 (동적 변경 가능)
static uint32_t publish_interval_ms = DEFAULT_PUBLISH_INTERVAL_MS;

// MPU6050 초기화 상태
static bool mpu6050_initialized = false;

/**
 * @brief 센서 데이터 읽기 (MPU6050)
 */
bool sensor_read_data(mpu6050_data_t *data)
{
    if (!mpu6050_initialized) {
        ESP_LOGE(TAG_SENSOR, "MPU6050 not initialized");
        return false;
    }

    // MPU6050 센서 데이터 읽기
    esp_err_t ret = mpu6050_read_data(data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "Failed to read MPU6050 data");
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
        vTaskDelete(NULL);  //NULL이면 자기자신 task 삭제
        return;
    }
    mpu6050_initialized = true; //초기화 상태임을 알려주는 flag
    ESP_LOGI(TAG_SENSOR, "MPU6050 initialized successfully");

    while (1) {
        mpu6050_data_t sensor_data;

        // 센서 데이터 읽기
        if (sensor_read_data(&sensor_data)) {
            // MQTT로 발행
            mqtt_publish_mpu6050_data(&sensor_data);
        } else {
            ESP_LOGE(TAG_SENSOR, "Failed to read sensor data");
        }

        // 동적 전송 주기로 대기
        vTaskDelay(pdMS_TO_TICKS(publish_interval_ms));
    }
}

/**
 * @brief 센서 태스크 시작
 */
void sensor_task_start(void)
{
    xTaskCreate(sensor_task, "sensor_task", 8192, NULL, 5, NULL);
    ESP_LOGI(TAG_SENSOR, "Sensor task created");
}
