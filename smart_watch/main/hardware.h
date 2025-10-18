/**
 * 하드웨어 모듈 헤더
 *
 * LCD, 터치 센서 등 하드웨어 초기화를 담당합니다.
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include "esp_err.h"
#include "st7789.h"
#include "cst816s.h"

// 하드웨어 핀 정의
#define LCD_DIN     23  // SPI MOSI
#define LCD_CLK     19  // SPI Clock
#define LCD_CS      21   // Chip Select
#define LCD_DC      2  // Data/Command
#define LCD_RST     1   // Reset
#define LCD_BL      0  // Backlight
#define LCD_WIDTH   240
#define LCD_HEIGHT  280

#define TP_SDA      22  // I2C Data
#define TP_SCL      23  // I2C Clock
#define TP_RST      16  // Touch Reset
#define TP_IRQ      17  // Touch Interrupt

// 심박 센서 (MAX30102) 핀 구성
#define HR_SDA      TP_SDA   // 터치 센서와 동일 I2C SDA
#define HR_SCL      TP_SCL   // 터치 센서와 동일 I2C SCL
#define HR_INT      19       // (옵션) 센서 인터럽트 입력

// 전역 하드웨어 객체
extern st7789_t lcd;
extern cst816s_t touch;

/**
 * 하드웨어 초기화
 *
 * - ST7789 LCD 초기화
 * - CST816S 터치 센서 초기화
 *
 * @return ESP_OK: 성공, 그 외: 에러 코드
 */
esp_err_t hardware_init(void);

#endif // HARDWARE_H
