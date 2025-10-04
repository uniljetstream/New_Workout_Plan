/* MAX30102 Heart Rate Sensor Example

   This example code reads heart rate sensor data from MAX30102 using I2C communication.
   The sensor should be connected to ESP32 I2C pins:
   - SDA: GPIO21
   - SCL: GPIO22
   - VCC: 3.3V
   - GND: GND
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "max30102";

// I2C Configuration
#define I2C_MASTER_SCL_IO           22    /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21    /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0     /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000 /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0     /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0     /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

// MAX30102 Register Addresses
#define MAX30102_I2C_ADDR            0x57  // 7-bit address
#define MAX30102_REG_INTR_STATUS_1   0x00
#define MAX30102_REG_INTR_STATUS_2   0x01
#define MAX30102_REG_INTR_ENABLE_1    0x02
#define MAX30102_REG_INTR_ENABLE_2    0x03
#define MAX30102_REG_FIFO_WR_PTR     0x04
#define MAX30102_REG_OVF_COUNTER     0x05
#define MAX30102_REG_FIFO_RD_PTR     0x06
#define MAX30102_REG_FIFO_DATA       0x07
#define MAX30102_REG_FIFO_CONFIG     0x08
#define MAX30102_REG_MODE_CONFIG     0x09
#define MAX30102_REG_SPO2_CONFIG     0x0A
#define MAX30102_REG_LED1_PA         0x0C
#define MAX30102_REG_LED2_PA         0x0D
#define MAX30102_REG_PILOT_PA        0x10
#define MAX30102_REG_MULTI_LED_CTRL1 0x11
#define MAX30102_REG_MULTI_LED_CTRL2 0x12
#define MAX30102_REG_TEMP_INTR       0x1F
#define MAX30102_REG_TEMP_FRAC       0x20
#define MAX30102_REG_TEMP_CONFIG     0x21
#define MAX30102_REG_PROX_INT_THRESH 0x30
#define MAX30102_REG_REV_ID          0xFE
#define MAX30102_REG_PART_ID        0xFF

// Heart Rate Calculation Parameters
#define SAMPLE_RATE_MS 20           // Sample every 20ms (50Hz)
#define BUFFER_SIZE 500             // Buffer for storing samples
#define MIN_PEAK_DISTANCE 25        // Minimum samples between peaks (500ms at 20ms sample rate)
#define THRESHOLD_FACTOR 0.1        // Threshold factor for peak detection (very sensitive)
#define STABLE_SAMPLES 100          // Samples needed for stable measurement
#define MAX_HEART_RATE 200          // Maximum valid heart rate
#define MIN_HEART_RATE 40           // Minimum valid heart rate

// Global variables
static uint32_t red_buffer[BUFFER_SIZE];
static uint32_t ir_buffer[BUFFER_SIZE];
static uint32_t peak_times[50];     // Store peak timestamps
static uint8_t peak_count = 0;
static uint32_t last_peak_time = 0;
static uint32_t sample_count = 0;

// I2C Master Initialization
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Write data to MAX30102 register
static esp_err_t max30102_write_reg(uint8_t reg_addr, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Read data from MAX30102 register
static esp_err_t max30102_read_reg(uint8_t reg_addr, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Read multiple bytes from MAX30102 FIFO
static esp_err_t max30102_read_fifo(uint32_t *red_data, uint32_t *ir_data)
{
    uint8_t fifo_data[6];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, MAX30102_REG_FIFO_DATA, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, fifo_data, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        // Combine 3 bytes for each LED (18-bit data)
        *red_data = ((uint32_t)fifo_data[0] << 16) | ((uint32_t)fifo_data[1] << 8) | fifo_data[2];
        *ir_data = ((uint32_t)fifo_data[3] << 16) | ((uint32_t)fifo_data[4] << 8) | fifo_data[5];
        
        // Mask to 18-bit
        *red_data &= 0x03FFFF;
        *ir_data &= 0x03FFFF;
    }
    
    return ret;
}

// Initialize MAX30102 sensor
static esp_err_t max30102_init(void)
{
    ESP_LOGI(TAG, "Initializing MAX30102 sensor...");
    
    // Check if sensor is present
    uint8_t part_id;
    esp_err_t ret = max30102_read_reg(MAX30102_REG_PART_ID, &part_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read PART_ID register");
        return ret;
    }
    
    ESP_LOGI(TAG, "MAX30102 PART_ID: 0x%02X", part_id);
    
    // Reset the sensor
    max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x40);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Configure FIFO
    max30102_write_reg(MAX30102_REG_FIFO_CONFIG, 0x0F); // Sample averaging = 1, FIFO rollover enabled
    
    // Configure SpO2
    max30102_write_reg(MAX30102_REG_SPO2_CONFIG, 0x27); // SPO2_ADC range = 4096nA, SPO2 sample rate = 100Hz, LED pulse width = 411μs
    
    // Configure LED pulse amplitude
    max30102_write_reg(MAX30102_REG_LED1_PA, 0x24); // Red LED current = 7.6mA
    max30102_write_reg(MAX30102_REG_LED2_PA, 0x24); // IR LED current = 7.6mA
    
    // Configure multi-LED control
    max30102_write_reg(MAX30102_REG_MULTI_LED_CTRL1, 0x21); // Enable LED1 (Red) and LED2 (IR)
    
    // Set mode to SpO2 mode
    max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x03); // SpO2 mode
    
    ESP_LOGI(TAG, "MAX30102 initialized successfully!");
    return ESP_OK;
}

// Improved moving average filter for IR signal
static uint32_t moving_average_filter(uint32_t new_value)
{
    static uint32_t filter_buffer[8] = {0};
    static uint8_t buffer_index = 0;
    static uint32_t sum = 0;
    static bool initialized = false;
    
    if (!initialized) {
        // Initialize buffer with first value
        for (int i = 0; i < 8; i++) {
            filter_buffer[i] = new_value;
        }
        sum = new_value * 8;
        initialized = true;
        return new_value;
    }
    
    // Remove oldest value from sum
    sum -= filter_buffer[buffer_index];
    
    // Add new value
    filter_buffer[buffer_index] = new_value;
    sum += new_value;
    
    // Move to next position
    buffer_index = (buffer_index + 1) % 8;
    
    return sum / 8;
}

// Improved peak detection algorithm with balanced conditions
static bool detect_peak(uint32_t current_value, uint32_t previous_value, uint32_t next_value, uint32_t threshold, uint32_t baseline)
{
    // Check if current value is significantly higher than neighbors and above threshold
    uint32_t min_neighbor = (previous_value < next_value) ? previous_value : next_value;
    
    // Extremely sensitive conditions for peak detection
    if (current_value > min_neighbor + (baseline * 0.01) &&  // At least 1% above baseline (extremely sensitive)
        current_value > previous_value &&                     // Rising edge
        current_value > next_value) {                         // Peak condition
        return true;
    }
    return false;
}

// Calculate heart rate from peak intervals with improved algorithm
static float calculate_heart_rate(void)
{
    if (peak_count < 3) {
        return 0.0; // Need at least 3 peaks for reliable calculation
    }
    
    // Calculate intervals between peaks
    uint32_t intervals[49]; // Max 49 intervals for 50 peaks
    uint8_t valid_intervals = 0;
    
    for (int i = 1; i < peak_count; i++) {
        uint32_t interval = peak_times[i] - peak_times[i-1];
        if (interval >= MIN_PEAK_DISTANCE * SAMPLE_RATE_MS && 
            interval <= 2000) { // Max 2 seconds between peaks (30 BPM minimum)
            intervals[valid_intervals] = interval;
            valid_intervals++;
        }
    }
    
    if (valid_intervals < 2) {
        return 0.0;
    }
    
    // Sort intervals to find median (more robust than average)
    for (int i = 0; i < valid_intervals - 1; i++) {
        for (int j = i + 1; j < valid_intervals; j++) {
            if (intervals[i] > intervals[j]) {
                uint32_t temp = intervals[i];
                intervals[i] = intervals[j];
                intervals[j] = temp;
            }
        }
    }
    
    // Use median interval for more stable heart rate
    uint32_t median_interval;
    if (valid_intervals % 2 == 0) {
        median_interval = (intervals[valid_intervals/2 - 1] + intervals[valid_intervals/2]) / 2;
    } else {
        median_interval = intervals[valid_intervals/2];
    }
    
    float heart_rate = 60000.0 / median_interval; // Convert to BPM
    
    // Limit heart rate to reasonable range
    if (heart_rate < MIN_HEART_RATE || heart_rate > MAX_HEART_RATE) {
        return 0.0;
    }
    
    return heart_rate;
}

// Calculate and display real-time heart rate when peak is detected
static void display_realtime_heart_rate(uint32_t current_time)
{
    if (peak_count >= 2) {
        // Calculate interval since last peak
        uint32_t interval_ms = current_time - last_peak_time;
        
        // Prevent division by zero
        if (interval_ms == 0) {
            ESP_LOGI(TAG, "Peak #%d detected! (Interval calculation skipped - too close to previous peak)", peak_count);
            return;
        }
        
        // Calculate instant heart rate
        float instant_bpm = 60000.0 / interval_ms;
        
        // Calculate average interval from recent peaks
        if (peak_count >= 3) {
            uint32_t total_interval = 0;
            uint8_t valid_intervals = 0;
            
            for (int i = 1; i < peak_count; i++) {
                uint32_t interval = peak_times[i] - peak_times[i-1];
                if (interval >= MIN_PEAK_DISTANCE * SAMPLE_RATE_MS && 
                    interval <= 2000) { // Max 2 seconds between peaks
                    total_interval += interval;
                    valid_intervals++;
                }
            }
            
            if (valid_intervals > 0) {
                float avg_interval_ms = (float)total_interval / valid_intervals;
                float avg_bpm = 60000.0 / avg_interval_ms;
                
                ESP_LOGI(TAG, "=== Real-time Heart Rate Analysis ===");
                ESP_LOGI(TAG, "Peak #%d detected!", peak_count);
                ESP_LOGI(TAG, "Last interval: %lu ms", interval_ms);
                ESP_LOGI(TAG, "Instant BPM: %.1f", instant_bpm);
                ESP_LOGI(TAG, "Average interval: %.1f ms", avg_interval_ms);
                ESP_LOGI(TAG, "Average BPM: %.1f", avg_bpm);
                ESP_LOGI(TAG, "Valid intervals: %d", valid_intervals);
                ESP_LOGI(TAG, "================================");
            } else {
                ESP_LOGI(TAG, "Peak #%d detected! Interval: %lu ms, Instant BPM: %.1f", 
                         peak_count, interval_ms, instant_bpm);
            }
        } else {
            ESP_LOGI(TAG, "Peak #%d detected! Interval: %lu ms, Instant BPM: %.1f", 
                     peak_count, interval_ms, instant_bpm);
        }
    }
}

// Heart rate measurement task for MAX30102 with improved algorithm
static void heart_rate_task(void *pvParameters)
{
    uint32_t max_value = 0;
    uint32_t min_value = 0x03FFFF; // 18-bit max value
    uint32_t threshold = 0;
    uint32_t baseline = 0;
    uint32_t red_value, ir_value;
    bool sensor_stable = false;
    
    ESP_LOGI(TAG, "Starting MAX30102 heart rate measurement...");
    ESP_LOGI(TAG, "Please place your finger on the sensor and wait for measurement...");
    ESP_LOGI(TAG, "Sensor needs %d samples to stabilize...", STABLE_SAMPLES);
    
    while (1) {
        // Read data from MAX30102 FIFO
        esp_err_t ret = max30102_read_fifo(&red_value, &ir_value);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read from MAX30102 FIFO");
            vTaskDelay(pdMS_TO_TICKS(SAMPLE_RATE_MS));
            continue;
        }
        
        // Apply moving average filter to IR signal (better for heart rate)
        uint32_t filtered_ir = moving_average_filter(ir_value);
        
        // Store in buffer
        red_buffer[sample_count] = red_value;
        ir_buffer[sample_count] = filtered_ir;
        
        // Update min/max for threshold calculation (use IR signal)
        if (filtered_ir > max_value) max_value = filtered_ir;
        if (filtered_ir < min_value) min_value = filtered_ir;
        
        // Calculate baseline as running average of recent IR values
        static uint32_t baseline_buffer[20] = {0};
        static uint8_t baseline_index = 0;
        static bool baseline_initialized = false;
        
        baseline_buffer[baseline_index] = filtered_ir;
        baseline_index = (baseline_index + 1) % 20;
        
        if (!baseline_initialized && sample_count >= 20) {
            baseline_initialized = true;
        }
        
        if (baseline_initialized) {
            uint32_t sum = 0;
            for (int i = 0; i < 20; i++) {
                sum += baseline_buffer[i];
            }
            baseline = sum / 20;
        } else {
            baseline = filtered_ir;  // Use current value until buffer is full
        }
        
        // Calculate threshold as percentage above baseline
        threshold = baseline + (baseline * THRESHOLD_FACTOR);
        
        // Check if sensor is stable
        if (sample_count >= STABLE_SAMPLES) {
            sensor_stable = true;
        }
        
        // Peak detection using IR signal (only after sensor is stable)
        if (sensor_stable && sample_count >= 2) {
            bool is_peak = detect_peak(
                ir_buffer[sample_count], 
                ir_buffer[sample_count-1], 
                ir_buffer[sample_count-2], 
                threshold,
                baseline
            );
            
            if (is_peak) {
                uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                
                // Check minimum distance between peaks
                if (current_time - last_peak_time > MIN_PEAK_DISTANCE * SAMPLE_RATE_MS) {
                    if (peak_count < 50) {
                        // Update peak data
                        peak_times[peak_count] = current_time;
                        peak_count++;
                        last_peak_time = current_time;
                        
                        // Display real-time heart rate analysis
                        display_realtime_heart_rate(current_time);
                    }
                } else {
                    ESP_LOGI(TAG, "Peak detected but too close to previous peak (%.1f ms)", 
                             (float)(current_time - last_peak_time));
                }
            }
        }
        
        // Calculate and display heart rate every 2 seconds (more frequent updates)
        if (sample_count % (2000 / SAMPLE_RATE_MS) == 0 && sample_count > 0) {
            if (sensor_stable) {
                float heart_rate = calculate_heart_rate();
                
                if (heart_rate > 0) {
                    // Calculate instant heart rate from last two peaks
                    float instant_bpm = 0.0;
                    if (peak_count >= 2) {
                        uint32_t last_interval = peak_times[peak_count-1] - peak_times[peak_count-2];
                        if (last_interval > 0) {
                            instant_bpm = 60000.0 / last_interval;
                        }
                    }
                    
                    ESP_LOGI(TAG, "=== Heart Rate Measurement ===");
                    ESP_LOGI(TAG, "Red LED: %lu, IR LED: %lu", red_value, ir_value);
                    ESP_LOGI(TAG, "Filtered IR: %lu, Baseline: %lu", filtered_ir, baseline);
                    ESP_LOGI(TAG, "Threshold: %lu, Min: %lu, Max: %lu", threshold, min_value, max_value);
                    ESP_LOGI(TAG, "Peaks detected: %d", peak_count);
                    ESP_LOGI(TAG, "Average Heart Rate: %.1f BPM", heart_rate);
                    if (instant_bpm > 0) {
                        ESP_LOGI(TAG, "Instant Heart Rate: %.1f BPM", instant_bpm);
                    }
                    ESP_LOGI(TAG, "=============================");
                } else {
                    ESP_LOGI(TAG, "Red LED: %lu, IR LED: %lu, Filtered IR: %lu", red_value, ir_value, filtered_ir);
                    ESP_LOGI(TAG, "Baseline: %lu, Threshold: %lu, Peaks: %d", baseline, threshold, peak_count);
                    ESP_LOGI(TAG, "Waiting for stable heart rate signal...");
                }
            } else {
                ESP_LOGI(TAG, "Sensor stabilizing... (%d/%d samples)", sample_count, STABLE_SAMPLES);
                ESP_LOGI(TAG, "Red LED: %lu, IR LED: %lu, Filtered IR: %lu", red_value, ir_value, filtered_ir);
            }
        }
        
        sample_count++;
        if (sample_count >= BUFFER_SIZE) {
            sample_count = 0; // Reset buffer
        }
        
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_RATE_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 MAX30102 Heart Rate Sensor Application Starting...");
    
    // Initialize I2C master
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master");
        return;
    }
    
    // Initialize MAX30102 sensor
    ret = max30102_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MAX30102 sensor");
        return;
    }
    
    // Create heart rate measurement task
    xTaskCreate(heart_rate_task, "heart_rate_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "MAX30102 heart rate sensor application started successfully!");
    ESP_LOGI(TAG, "Connect MAX30102 sensor:");
    ESP_LOGI(TAG, "  VCC -> 3.3V");
    ESP_LOGI(TAG, "  GND -> GND");
    ESP_LOGI(TAG, "  SDA -> GPIO21");
    ESP_LOGI(TAG, "  SCL -> GPIO22");
    ESP_LOGI(TAG, "Monitor the output for heart rate measurements...");
    
    // Keep main task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
