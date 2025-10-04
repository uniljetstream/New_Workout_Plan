/* MPU6050 센서 드라이버 구현 */

#include "mpu6050.h"
#include "config.h"

#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// MPU6050 레지스터 주소
#define MPU6050_SENSOR_ADDR 0x68
#define MPU6050_WHO_AM_I_REG_ADDR 0x75
#define MPU6050_PWR_MGMT_1_REG_ADDR 0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_ACCEL_CONFIG_REG 0x1C
#define MPU6050_GYRO_CONFIG_REG 0x1B
#define I2C_MASTER_TIMEOUT_MS 1000
#define CALIBRATION_SAMPLES 200

// 가속도계/자이로스코프 범위
#define ACCEL_RANGE_2G 0x00
#define GYRO_RANGE_250 0x00

// 보정 오프셋
typedef struct {
    int16_t accel_x_offset;
    int16_t accel_y_offset;
    int16_t accel_z_offset;
    int16_t gyro_x_offset;
    int16_t gyro_y_offset;
    int16_t gyro_z_offset;
} mpu6050_calibration_t;

// 전역 변수
static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;
static mpu6050_calibration_t calibration = {0};
static float accel_sensitivity = 16384.0;  // ±2g
static float gyro_sensitivity = 131.0;      // ±250°/s

/**
 * @brief MPU6050 레지스터 읽기
 */
static esp_err_t mpu6050_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS);
}

/**
 * @brief MPU6050 레지스터 쓰기
 */
static esp_err_t mpu6050_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);
}

/**
 * @brief 센서 값 읽기
 */
static esp_err_t mpu6050_read_sensor_raw(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                                          int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z, int16_t *temp)
{
    uint8_t data[14];
    esp_err_t ret = mpu6050_register_read(MPU6050_ACCEL_XOUT_H, data, 14);
    if (ret != ESP_OK) {
        return ret;
    }

    *accel_x = (int16_t)((data[0] << 8) | data[1]);
    *accel_y = (int16_t)((data[2] << 8) | data[3]);
    *accel_z = (int16_t)((data[4] << 8) | data[5]);
    *temp = (int16_t)((data[6] << 8) | data[7]);
    *gyro_x = (int16_t)((data[8] << 8) | data[9]);
    *gyro_y = (int16_t)((data[10] << 8) | data[11]);
    *gyro_z = (int16_t)((data[12] << 8) | data[13]);

    return ESP_OK;
}

/**
 * @brief 센서 보정
 */
static esp_err_t mpu6050_calibrate(void)
{
    ESP_LOGI(TAG_SENSOR, "MPU6050 보정 시작... 센서를 평평한 곳에 두세요!");

    int32_t accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
    int32_t gyro_x_sum = 0, gyro_y_sum = 0, gyro_z_sum = 0;
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t temp;

    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        esp_err_t ret = mpu6050_read_sensor_raw(&accel_x, &accel_y, &accel_z,
                                                 &gyro_x, &gyro_y, &gyro_z, &temp);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG_SENSOR, "보정 실패: 샘플 %d", i);
            return ret;
        }

        accel_x_sum += accel_x;
        accel_y_sum += accel_y;
        accel_z_sum += accel_z;
        gyro_x_sum += gyro_x;
        gyro_y_sum += gyro_y;
        gyro_z_sum += gyro_z;

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    calibration.accel_x_offset = accel_x_sum / CALIBRATION_SAMPLES;
    calibration.accel_y_offset = accel_y_sum / CALIBRATION_SAMPLES;
    calibration.accel_z_offset = (accel_z_sum / CALIBRATION_SAMPLES) - 16384;
    calibration.gyro_x_offset = gyro_x_sum / CALIBRATION_SAMPLES;
    calibration.gyro_y_offset = gyro_y_sum / CALIBRATION_SAMPLES;
    calibration.gyro_z_offset = gyro_z_sum / CALIBRATION_SAMPLES;

    ESP_LOGI(TAG_SENSOR, "MPU6050 보정 완료!");
    ESP_LOGI(TAG_SENSOR, "가속도 오프셋: X=%d Y=%d Z=%d",
             calibration.accel_x_offset, calibration.accel_y_offset, calibration.accel_z_offset);
    ESP_LOGI(TAG_SENSOR, "자이로 오프셋: X=%d Y=%d Z=%d",
             calibration.gyro_x_offset, calibration.gyro_y_offset, calibration.gyro_z_offset);

    return ESP_OK;
}

/**
 * @brief 보정 적용
 */
static void mpu6050_apply_calibration(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                                       int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
    *accel_x -= calibration.accel_x_offset;
    *accel_y -= calibration.accel_y_offset;
    *accel_z -= calibration.accel_z_offset;
    *gyro_x -= calibration.gyro_x_offset;
    *gyro_y -= calibration.gyro_y_offset;
    *gyro_z -= calibration.gyro_z_offset;
}

/**
 * @brief MPU6050 초기화
 */
esp_err_t mpu6050_init_sensor(void)
{
    esp_err_t ret;

    // I2C 버스 초기화
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "I2C 버스 초기화 실패");
        return ret;
    }
    ESP_LOGI(TAG_SENSOR, "I2C 버스 초기화 완료: SDA=%d, SCL=%d",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    // MPU6050 디바이스 추가
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "MPU6050 디바이스 추가 실패");
        return ret;
    }

    // WHO_AM_I 레지스터 확인
    uint8_t who_am_i;
    ret = mpu6050_register_read(MPU6050_WHO_AM_I_REG_ADDR, &who_am_i, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "WHO_AM_I 읽기 실패. MPU6050이 연결되었는지 확인하세요.");
        return ret;
    }
    ESP_LOGI(TAG_SENSOR, "MPU6050 WHO_AM_I = 0x%X", who_am_i);

    // MPU6050 깨우기
    ret = mpu6050_register_write_byte(MPU6050_PWR_MGMT_1_REG_ADDR, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "MPU6050 전원 관리 실패");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // 가속도계 범위 설정 (±2g)
    ret = mpu6050_register_write_byte(MPU6050_ACCEL_CONFIG_REG, ACCEL_RANGE_2G);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "가속도계 범위 설정 실패");
        return ret;
    }

    // 자이로스코프 범위 설정 (±250°/s)
    ret = mpu6050_register_write_byte(MPU6050_GYRO_CONFIG_REG, GYRO_RANGE_250);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "자이로스코프 범위 설정 실패");
        return ret;
    }

    ESP_LOGI(TAG_SENSOR, "MPU6050 초기화 완료 (±2g, ±250°/s)");

    // 보정 수행
    ret = mpu6050_calibrate();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SENSOR, "MPU6050 보정 실패");
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief MPU6050 센서 데이터 읽기
 */
esp_err_t mpu6050_read_data(mpu6050_data_t *data)
{
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t temp;

    // Raw 데이터 읽기
    esp_err_t ret = mpu6050_read_sensor_raw(&accel_x, &accel_y, &accel_z,
                                             &gyro_x, &gyro_y, &gyro_z, &temp);
    if (ret != ESP_OK) {
        return ret;
    }

    // 보정 적용
    mpu6050_apply_calibration(&accel_x, &accel_y, &accel_z, &gyro_x, &gyro_y, &gyro_z);

    // 물리 단위로 변환
    data->accel_x = accel_x / accel_sensitivity;
    data->accel_y = accel_y / accel_sensitivity;
    data->accel_z = accel_z / accel_sensitivity;
    data->gyro_x = gyro_x / gyro_sensitivity;
    data->gyro_y = gyro_y / gyro_sensitivity;
    data->gyro_z = gyro_z / gyro_sensitivity;
    data->temperature = (temp / 340.0) + 36.53;

    return ESP_OK;
}

/**
 * @brief MPU6050 종료
 */
esp_err_t mpu6050_deinit(void)
{
    if (dev_handle != NULL) {
        i2c_master_bus_rm_device(dev_handle);
        dev_handle = NULL;
    }

    if (bus_handle != NULL) {
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
    }

    ESP_LOGI(TAG_SENSOR, "MPU6050 종료 완료");
    return ESP_OK;
}
