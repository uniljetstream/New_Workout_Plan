/* 에어마우스 기능 구현
 * MPU6050 센서를 이용한 에어마우스 구현
 */

#include "airmouse.h"
#include "config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include <string.h>

static const char *TAG = "AIRMOUSE";

// 전역 변수들
static airmouse_mode_t current_mode = AIRMOUSE_MODE_SENSOR;
static calibration_data_t calibration_data = {0};
static filter_config_t filter_config = {
    .alpha = 0.8f,              // 저역통과 필터 계수
    .threshold = 0.1f,          // 움직임 임계값
    .smoothing_samples = 5       // 스무딩 샘플 수
};
static sensitivity_config_t sensitivity_config = {
    .sensitivity_x = 1.0f,      // X축 감도
    .sensitivity_y = 1.0f,      // Y축 감도
    .sensitivity_z = 1.0f,      // Z축 감도
    .max_speed = 100.0f         // 최대 마우스 속도
};
static gesture_config_t gesture_config = {
    .scroll_threshold = 1.5f   // 스크롤 감지 임계값
};

// 필터링을 위한 이전 값들
static float prev_accel_x = 0.0f;
static float prev_accel_y = 0.0f;
static float prev_accel_z = 0.0f;
static float prev_gyro_x = 0.0f;
static float prev_gyro_y = 0.0f;
static float prev_gyro_z = 0.0f;

// 스무딩을 위한 버퍼 (메모리 재사용)
#define SMOOTHING_BUFFER_SIZE 10
static float smoothing_buffer_x[SMOOTHING_BUFFER_SIZE] = {0};
static float smoothing_buffer_y[SMOOTHING_BUFFER_SIZE] = {0};
static float smoothing_buffer_z[SMOOTHING_BUFFER_SIZE] = {0};
static uint8_t smoothing_index = 0;

// 마우스 데이터 캐시 (메모리 재사용)
static mouse_data_t mouse_data_cache;

/**
 * @brief 저역통과 필터 적용
 */
static float apply_low_pass_filter(float current, float previous, float alpha)
{
    return alpha * current + (1.0f - alpha) * previous;
}

/**
 * @brief 스무딩 필터 적용
 */
static float apply_smoothing_filter(float new_value, float *buffer, uint8_t *index)
{
    buffer[*index] = new_value;
    *index = (*index + 1) % SMOOTHING_BUFFER_SIZE;
    
    float sum = 0.0f;
    for (int i = 0; i < SMOOTHING_BUFFER_SIZE; i++) {
        sum += buffer[i];
    }
    return sum / SMOOTHING_BUFFER_SIZE;
}

/**
 * @brief 제스처 인식 (스크롤만)
 */
static void detect_gestures(const mpu6050_data_t *sensor_data, mouse_data_t *mouse_data)
{
    // 스크롤 감지 (Y축 움직임)
    float y_movement = fabs(sensor_data->accel_y - prev_accel_y);
    if (y_movement > gesture_config.scroll_threshold) {
        if (sensor_data->accel_y > prev_accel_y) {
            mouse_data->scroll_delta = 1;  // 위로 스크롤
        } else {
            mouse_data->scroll_delta = -1; // 아래로 스크롤
        }
    }
}

/**
 * @brief 에어마우스 초기화
 */
bool airmouse_init(void)
{
    ESP_LOGI(TAG, "Initializing AirMouse...");
    
    // 기본값으로 초기화
    current_mode = AIRMOUSE_MODE_SENSOR;
    memset(&calibration_data, 0, sizeof(calibration_data));
    
    // 스무딩 버퍼 초기화
    memset(smoothing_buffer_x, 0, sizeof(smoothing_buffer_x));
    memset(smoothing_buffer_y, 0, sizeof(smoothing_buffer_y));
    memset(smoothing_buffer_z, 0, sizeof(smoothing_buffer_z));
    smoothing_index = 0;
    
    ESP_LOGI(TAG, "AirMouse initialized successfully");
    return true;
}

/**
 * @brief 에어마우스 모드 설정
 */
void airmouse_set_mode(airmouse_mode_t mode)
{
    current_mode = mode;
    ESP_LOGI(TAG, "AirMouse mode set to: %s", 
             mode == AIRMOUSE_MODE_SENSOR ? "SENSOR" : "MOUSE");
}

/**
 * @brief 현재 에어마우스 모드 조회
 */
airmouse_mode_t airmouse_get_mode(void)
{
    return current_mode;
}

/**
 * @brief 센서 캘리브레이션 시작
 */
bool airmouse_start_calibration(void)
{
    ESP_LOGI(TAG, "Starting calibration...");
    
    // 캘리브레이션 데이터 초기화
    memset(&calibration_data, 0, sizeof(calibration_data));
    calibration_data.calibrated = false;
    
    // 3초간 데이터 수집하여 평균값 계산
    mpu6050_data_t sensor_data;
    float sum_accel_x = 0, sum_accel_y = 0, sum_accel_z = 0;
    float sum_gyro_x = 0, sum_gyro_y = 0, sum_gyro_z = 0;
    int sample_count = 0;
    
    uint32_t start_time = esp_timer_get_time() / 1000;
    uint32_t calibration_duration = 3000; // 3초
    
    while ((esp_timer_get_time() / 1000) - start_time < calibration_duration) {
        if (mpu6050_read_data(&sensor_data) == ESP_OK) {
            sum_accel_x += sensor_data.accel_x;
            sum_accel_y += sensor_data.accel_y;
            sum_accel_z += sensor_data.accel_z;
            sum_gyro_x += sensor_data.gyro_x;
            sum_gyro_y += sensor_data.gyro_y;
            sum_gyro_z += sensor_data.gyro_z;
            sample_count++;
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms 대기
    }
    
    if (sample_count > 0) {
        calibration_data.accel_x_offset = sum_accel_x / sample_count;
        calibration_data.accel_y_offset = sum_accel_y / sample_count;
        calibration_data.accel_z_offset = sum_accel_z / sample_count;
        calibration_data.gyro_x_offset = sum_gyro_x / sample_count;
        calibration_data.gyro_y_offset = sum_gyro_y / sample_count;
        calibration_data.gyro_z_offset = sum_gyro_z / sample_count;
        calibration_data.calibrated = true;
        
        ESP_LOGI(TAG, "Calibration completed with %d samples", sample_count);
        ESP_LOGI(TAG, "Offsets - Accel: X=%.3f Y=%.3f Z=%.3f", 
                 calibration_data.accel_x_offset, calibration_data.accel_y_offset, calibration_data.accel_z_offset);
        ESP_LOGI(TAG, "Offsets - Gyro: X=%.3f Y=%.3f Z=%.3f", 
                 calibration_data.gyro_x_offset, calibration_data.gyro_y_offset, calibration_data.gyro_z_offset);
        return true;
    }
    
    ESP_LOGE(TAG, "Calibration failed - no samples collected");
    return false;
}

/**
 * @brief 캘리브레이션 상태 확인
 */
bool airmouse_is_calibrated(void)
{
    return calibration_data.calibrated;
}

/**
 * @brief 센서 데이터를 마우스 데이터로 변환
 */
bool airmouse_convert_sensor_to_mouse(const mpu6050_data_t *sensor_data, mouse_data_t *mouse_data)
{
    if (!sensor_data || !mouse_data) {
        return false;
    }
    
    // 마우스 데이터 초기화 (고정 버퍼 재사용)
    memset(&mouse_data_cache, 0, sizeof(mouse_data_t));
    
    // 캘리브레이션 적용
    float accel_x = sensor_data->accel_x - calibration_data.accel_x_offset;
    float accel_y = sensor_data->accel_y - calibration_data.accel_y_offset;
    float accel_z = sensor_data->accel_z - calibration_data.accel_z_offset;
    float gyro_x = sensor_data->gyro_x - calibration_data.gyro_x_offset;
    float gyro_y = sensor_data->gyro_y - calibration_data.gyro_y_offset;
    float gyro_z = sensor_data->gyro_z - calibration_data.gyro_z_offset;
    
    // 저역통과 필터 적용
    accel_x = apply_low_pass_filter(accel_x, prev_accel_x, filter_config.alpha);
    accel_y = apply_low_pass_filter(accel_y, prev_accel_y, filter_config.alpha);
    accel_z = apply_low_pass_filter(accel_z, prev_accel_z, filter_config.alpha);
    gyro_x = apply_low_pass_filter(gyro_x, prev_gyro_x, filter_config.alpha);
    gyro_y = apply_low_pass_filter(gyro_y, prev_gyro_y, filter_config.alpha);
    gyro_z = apply_low_pass_filter(gyro_z, prev_gyro_z, filter_config.alpha);
    
    // 스무딩 필터 적용
    accel_x = apply_smoothing_filter(accel_x, smoothing_buffer_x, &smoothing_index);
    accel_y = apply_smoothing_filter(accel_y, smoothing_buffer_y, &smoothing_index);
    accel_z = apply_smoothing_filter(accel_z, smoothing_buffer_z, &smoothing_index);
    
    // 임계값 적용 (작은 움직임 무시)
    if (fabs(accel_x) < filter_config.threshold) accel_x = 0;
    if (fabs(accel_y) < filter_config.threshold) accel_y = 0;
    if (fabs(accel_z) < filter_config.threshold) accel_z = 0;
    
    // 감도 적용 및 마우스 좌표 변환 (고정 버퍼 사용)
    mouse_data_cache.mouse_x = accel_x * sensitivity_config.sensitivity_x * 50.0f; // 스케일링
    mouse_data_cache.mouse_y = accel_y * sensitivity_config.sensitivity_y * 50.0f;
    
    // 최대 속도 제한
    if (fabs(mouse_data_cache.mouse_x) > sensitivity_config.max_speed) {
        mouse_data_cache.mouse_x = (mouse_data_cache.mouse_x > 0) ? sensitivity_config.max_speed : -sensitivity_config.max_speed;
    }
    if (fabs(mouse_data_cache.mouse_y) > sensitivity_config.max_speed) {
        mouse_data_cache.mouse_y = (mouse_data_cache.mouse_y > 0) ? sensitivity_config.max_speed : -sensitivity_config.max_speed;
    }
    
    // 제스처 인식
    detect_gestures(sensor_data, &mouse_data_cache);
    
    // 이전 값 업데이트
    prev_accel_x = accel_x;
    prev_accel_y = accel_y;
    prev_accel_z = accel_z;
    prev_gyro_x = gyro_x;
    prev_gyro_y = gyro_y;
    prev_gyro_z = gyro_z;
    
    // 결과를 출력 매개변수에 복사
    *mouse_data = mouse_data_cache;
    
    return true;
}

/**
 * @brief 필터링 설정
 */
void airmouse_set_filter_config(const filter_config_t *config)
{
    if (config) {
        filter_config = *config;
        ESP_LOGI(TAG, "Filter config updated - alpha: %.2f, threshold: %.2f", 
                 config->alpha, config->threshold);
    }
}

/**
 * @brief 감도 설정
 */
void airmouse_set_sensitivity_config(const sensitivity_config_t *config)
{
    if (config) {
        sensitivity_config = *config;
        ESP_LOGI(TAG, "Sensitivity config updated - X: %.2f, Y: %.2f, max_speed: %.2f", 
                 config->sensitivity_x, config->sensitivity_y, config->max_speed);
    }
}

/**
 * @brief 제스처 인식 설정
 */
void airmouse_set_gesture_config(const gesture_config_t *config)
{
    if (config) {
        gesture_config = *config;
        ESP_LOGI(TAG, "Gesture config updated - scroll_threshold: %.2f", 
                 config->scroll_threshold);
    }
}

/**
 * @brief 현재 필터링 설정 조회
 */
void airmouse_get_filter_config(filter_config_t *config)
{
    if (config) {
        *config = filter_config;
    }
}

/**
 * @brief 현재 감도 설정 조회
 */
void airmouse_get_sensitivity_config(sensitivity_config_t *config)
{
    if (config) {
        *config = sensitivity_config;
    }
}

/**
 * @brief 현재 제스처 설정 조회
 */
void airmouse_get_gesture_config(gesture_config_t *config)
{
    if (config) {
        *config = gesture_config;
    }
}

/**
 * @brief 에어마우스 종료 및 리소스 해제
 */
void airmouse_deinit(void)
{
    ESP_LOGI(TAG, "AirMouse deinitialized");
    current_mode = AIRMOUSE_MODE_SENSOR;
    memset(&calibration_data, 0, sizeof(calibration_data));
}
