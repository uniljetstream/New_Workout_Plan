/**
 * 하드웨어 모듈 구현
 *
 * LCD와 터치 센서 초기화를 담당합니다.
 */

#include "hardware.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"

static const char *TAG = "HW";

// 전역 하드웨어 객체
st7789_t lcd;
cst816s_t touch;

esp_err_t hardware_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "하드웨어 초기화 시작...");

    // ========================================================================
    // ST7789 LCD 초기화
    // ========================================================================
    ESP_LOGI(TAG, "LCD 초기화 중...");
    ret = st7789_init(&lcd, SPI2_HOST, LCD_DIN, LCD_CLK, LCD_CS, LCD_DC, LCD_RST, LCD_BL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD 초기화 실패");
        return ret;
    }

    // ========================================================================
    // CST816S 터치 센서 초기화
    // ========================================================================
    ESP_LOGI(TAG, "터치 센서 초기화 중...");
    ret = cst816s_init(&touch, I2C_NUM_0, TP_SDA, TP_SCL, TP_RST, TP_IRQ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "터치 센서 초기화 실패");
        return ret;
    }

    ESP_LOGI(TAG, "하드웨어 초기화 완료");
    return ESP_OK;
}
