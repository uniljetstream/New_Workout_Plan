/* MPU6050 센서 드라이버 헤더 */

#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

// MPU6050 측정 데이터 구조체
typedef struct {
    float accel_x;    // 가속도 X축 (g)
    float accel_y;    // 가속도 Y축 (g)
    float accel_z;    // 가속도 Z축 (g)
    float gyro_x;     // 자이로 X축 (°/s)
    float gyro_y;     // 자이로 Y축 (°/s)
    float gyro_z;     // 자이로 Z축 (°/s)
    float temperature;// 온도 (°C)
} mpu6050_data_t;

/**
 * @brief MPU6050 초기화
 *
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
esp_err_t mpu6050_init_sensor(void);

/**
 * @brief MPU6050 센서 데이터 읽기
 *
 * @param data 센서 데이터를 저장할 구조체 포인터
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
esp_err_t mpu6050_read_data(mpu6050_data_t *data);

/**
 * @brief MPU6050 종료 및 리소스 해제
 *
 * @return esp_err_t ESP_OK 성공, 그 외 에러 코드
 */
esp_err_t mpu6050_deinit(void);

#endif // MPU6050_H
