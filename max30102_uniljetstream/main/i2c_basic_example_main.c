/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* MAX30102 Heart Rate and SpO2 Sensor Example

   I2C example for MAX30102 heart rate and blood oxygen sensor.

   The sensor used in this example is a MAX30102 pulse oximetry and heart-rate sensor.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

static const char *TAG = "MAX30102";

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          CONFIG_I2C_MASTER_FREQUENCY /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS       1000

// MAX30102 I2C Address (7-bit)
// Common addresses: 0x57 (most common), 0x55, 0xAE>>1=0x57, 0xAF>>1=0x57
#define MAX30102_ADDR               0x57
#define MAX30102_ADDR_ALT           0x55

// MAX30102 Register Addresses
#define MAX30102_INT_STATUS_1       0x00
#define MAX30102_INT_STATUS_2       0x01
#define MAX30102_INT_ENABLE_1       0x02
#define MAX30102_INT_ENABLE_2       0x03
#define MAX30102_FIFO_WR_PTR        0x04
#define MAX30102_FIFO_OVF_COUNTER   0x05
#define MAX30102_FIFO_RD_PTR        0x06
#define MAX30102_FIFO_DATA          0x07
#define MAX30102_FIFO_CONFIG        0x08
#define MAX30102_MODE_CONFIG        0x09
#define MAX30102_SPO2_CONFIG        0x0A
#define MAX30102_LED1_PA            0x0C  // Red LED
#define MAX30102_LED2_PA            0x0D  // IR LED
#define MAX30102_PILOT_PA           0x10
#define MAX30102_MULTI_LED_CTRL1    0x11
#define MAX30102_MULTI_LED_CTRL2    0x12
#define MAX30102_DIE_TEMP_INT       0x1F
#define MAX30102_DIE_TEMP_FRAC      0x20
#define MAX30102_DIE_TEMP_CONFIG    0x21
#define MAX30102_REV_ID             0xFE
#define MAX30102_PART_ID            0xFF

// Mode Configuration
#define MAX30102_MODE_HEART_RATE    0x02
#define MAX30102_MODE_SPO2          0x03
#define MAX30102_MODE_MULTI_LED     0x07

/**
 * @brief Read a sequence of bytes from MAX30102 sensor registers
 */
static esp_err_t max30102_register_read(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS);
}

/**
 * @brief Write a byte to MAX30102 sensor register
 */
static esp_err_t max30102_register_write_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);
}

/**
 * @brief Reset MAX30102 sensor
 */
static esp_err_t max30102_reset(i2c_master_dev_handle_t dev_handle)
{
    return max30102_register_write_byte(dev_handle, MAX30102_MODE_CONFIG, 0x40);
}

/**
 * @brief Initialize MAX30102 sensor for SpO2 mode
 */
static esp_err_t max30102_init(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t ret;

    // Reset the sensor
    ret = max30102_reset(dev_handle);
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(50));

    // Configure FIFO: Sample averaging = 4, FIFO rollover = false, FIFO almost full = 17
    ret = max30102_register_write_byte(dev_handle, MAX30102_FIFO_CONFIG, 0x4F);
    if (ret != ESP_OK) return ret;

    // Set mode to SpO2 mode
    ret = max30102_register_write_byte(dev_handle, MAX30102_MODE_CONFIG, MAX30102_MODE_SPO2);
    if (ret != ESP_OK) return ret;

    // Configure SpO2: SPO2_ADC range = 4096nA, SPO2 sample rate = 100Hz, LED pulse width = 411us
    ret = max30102_register_write_byte(dev_handle, MAX30102_SPO2_CONFIG, 0x27);
    if (ret != ESP_OK) return ret;

    // Set LED pulse amplitude (Red LED)
    ret = max30102_register_write_byte(dev_handle, MAX30102_LED1_PA, 0x24);
    if (ret != ESP_OK) return ret;

    // Set LED pulse amplitude (IR LED)
    ret = max30102_register_write_byte(dev_handle, MAX30102_LED2_PA, 0x24);
    if (ret != ESP_OK) return ret;

    return ESP_OK;
}

/**
 * @brief Read FIFO data from MAX30102
 */
static esp_err_t max30102_read_fifo(i2c_master_dev_handle_t dev_handle, uint32_t *red_led, uint32_t *ir_led)
{
    uint8_t fifo_data[6];
    esp_err_t ret;

    ret = max30102_register_read(dev_handle, MAX30102_FIFO_DATA, fifo_data, 6);
    if (ret != ESP_OK) return ret;

    // Each LED reading is 18 bits (3 bytes), but we only use the lower 18 bits
    *red_led = ((uint32_t)fifo_data[0] << 16) | ((uint32_t)fifo_data[1] << 8) | fifo_data[2];
    *red_led &= 0x3FFFF; // Mask to 18 bits

    *ir_led = ((uint32_t)fifo_data[3] << 16) | ((uint32_t)fifo_data[4] << 8) | fifo_data[5];
    *ir_led &= 0x3FFFF; // Mask to 18 bits

    return ESP_OK;
}

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle, uint8_t addr)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,  // Disable internal pullup, use external resistors
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, bus_handle);
    if (ret != ESP_OK) return ret;

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    return i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle);
}

void gpio_test(void)
{
    ESP_LOGI(TAG, "=== GPIO Test Mode ===");
    ESP_LOGI(TAG, "Testing GPIO%d (SDA) and GPIO%d (SCL)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    ESP_LOGI(TAG, "Connect multimeter or oscilloscope to measure");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << I2C_MASTER_SDA_IO) | (1ULL << I2C_MASTER_SCL_IO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Toggling GPIOs at 1Hz for 10 seconds...");
    for (int i = 0; i < 10; i++) {
        gpio_set_level(I2C_MASTER_SDA_IO, 1);
        gpio_set_level(I2C_MASTER_SCL_IO, 1);
        ESP_LOGI(TAG, "HIGH - Measure voltage (should be ~3.3V)");
        vTaskDelay(pdMS_TO_TICKS(500));

        gpio_set_level(I2C_MASTER_SDA_IO, 0);
        gpio_set_level(I2C_MASTER_SCL_IO, 0);
        ESP_LOGI(TAG, "LOW - Measure voltage (should be ~0V)");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "GPIO test complete");
}

void app_main(void)
{
    uint8_t data;
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle = NULL;
    esp_err_t ret;

    // Uncomment to run GPIO test
    // gpio_test();
    // return;

    // Initialize I2C bus only (without device)
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,  // Try internal pullup
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    ESP_LOGI(TAG, "I2C bus initialized (SDA=GPIO%d, SCL=GPIO%d, Freq=%dHz)",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);

    // I2C scan to find device
    ESP_LOGI(TAG, "Scanning I2C bus...");
    bool device_found = false;
    uint8_t found_addr = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        ret = i2c_master_probe(bus_handle, addr, I2C_MASTER_TIMEOUT_MS);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found device at address 0x%02X", addr);
            if (addr == MAX30102_ADDR || addr == MAX30102_ADDR_ALT) {
                found_addr = addr;
            }
            device_found = true;
        }
    }

    if (!device_found) {
        ESP_LOGE(TAG, "No I2C devices found!");
        ESP_LOGE(TAG, "Hardware issue - check:");
        ESP_LOGE(TAG, "  1. SDA=GPIO21, SCL=GPIO22 connections");
        ESP_LOGE(TAG, "  2. 3.3V and GND connections");
        ESP_LOGE(TAG, "  3. Pull-up resistors (4.7k ohm on SDA and SCL)");
        ESP_LOGE(TAG, "  4. Sensor module is powered on");
        return;
    }

    // If MAX30102 was found, use that address, otherwise use default
    uint8_t sensor_addr = (found_addr != 0) ? found_addr : MAX30102_ADDR;
    ESP_LOGI(TAG, "Using MAX30102 address: 0x%02X", sensor_addr);

    // Add device to bus
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = sensor_addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

    // Read Part ID (should be 0x15 for MAX30102)
    ret = max30102_register_read(dev_handle, MAX30102_PART_ID, &data, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read Part ID: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "MAX30102 Part ID: 0x%02X (Expected: 0x15)", data);

    // Read Revision ID
    ESP_ERROR_CHECK(max30102_register_read(dev_handle, MAX30102_REV_ID, &data, 1));
    ESP_LOGI(TAG, "MAX30102 Revision ID: 0x%02X", data);

    // Initialize MAX30102 sensor
    ESP_ERROR_CHECK(max30102_init(dev_handle));
    ESP_LOGI(TAG, "MAX30102 initialized successfully in SpO2 mode");

    // Continuous reading loop
    uint32_t red_led, ir_led;
    ESP_LOGI(TAG, "Starting continuous reading...");
    ESP_LOGI(TAG, "Place your finger on the sensor");

    while (1) {
        // Read FIFO data
        if (max30102_read_fifo(dev_handle, &red_led, &ir_led) == ESP_OK) {
            ESP_LOGI(TAG, "Red: %6lu, IR: %6lu", red_led, ir_led);
        } else {
            ESP_LOGW(TAG, "Failed to read FIFO data");
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Read every 100ms
    }

    // Cleanup (unreachable in this example due to infinite loop)
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}
