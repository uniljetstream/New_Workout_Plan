/**
 * ST7789 LCD 디스플레이 드라이버
 *
 * 240x280 해상도의 ST7789 컨트롤러 기반 TFT LCD를 제어하는 드라이버입니다.
 * SPI 통신을 사용하여 고속으로 픽셀 데이터를 전송합니다.
 *
 * 주요 기능:
 * - SPI 통신을 통한 LCD 초기화
 * - 특정 영역(윈도우) 설정 및 픽셀 데이터 전송
 * - 사각형 영역 채우기
 */

#ifndef ST7789_H
#define ST7789_H

#include "driver/spi_master.h"
#include "driver/gpio.h"

// ============================================================================
// ST7789 명령어 정의 (Command Definitions)
// ============================================================================

#define ST7789_NOP      0x00  // No Operation - 아무 동작 안 함
#define ST7789_SWRESET  0x01  // Software Reset - 소프트웨어 리셋
#define ST7789_SLPIN    0x10  // Sleep In - 저전력 모드 진입
#define ST7789_SLPOUT   0x11  // Sleep Out - 저전력 모드 해제
#define ST7789_INVOFF   0x20  // Display Inversion Off - 색상 반전 끄기
#define ST7789_INVON    0x21  // Display Inversion On - 색상 반전 켜기
#define ST7789_DISPOFF  0x28  // Display Off - 화면 끄기
#define ST7789_DISPON   0x29  // Display On - 화면 켜기
#define ST7789_CASET    0x2A  // Column Address Set - X 좌표 범위 설정
#define ST7789_RASET    0x2B  // Row Address Set - Y 좌표 범위 설정
#define ST7789_RAMWR    0x2C  // Memory Write - 픽셀 데이터 쓰기
#define ST7789_MADCTL   0x36  // Memory Access Control - 화면 회전/미러링 설정
#define ST7789_COLMOD   0x3A  // Color Mode - 색상 포맷 설정 (RGB565 등)

// ============================================================================
// ST7789 구조체 (Structure)
// ============================================================================

/**
 * ST7789 LCD 디바이스 구조체
 *
 * LCD 제어에 필요한 SPI 핸들과 GPIO 핀 정보를 저장합니다.
 */
typedef struct {
    spi_device_handle_t spi;  // SPI 디바이스 핸들
    int dc_io;                // Data/Command 핀 (0=명령, 1=데이터)
    int rst_io;               // Reset 핀 (하드웨어 리셋)
    int bl_io;                // Backlight 핀 (백라이트 제어)
    int width;                // 화면 가로 해상도
    int height;               // 화면 세로 해상도
} st7789_t;

// ============================================================================
// 함수 선언 (Function Declarations)
// ============================================================================

/**
 * ST7789 LCD 초기화
 *
 * SPI 버스를 설정하고 LCD를 초기화합니다.
 * GPIO 핀을 설정하고 LCD 컨트롤러의 초기화 시퀀스를 실행합니다.
 *
 * @param lcd     ST7789 디바이스 구조체 포인터
 * @param host    SPI 호스트 (SPI2_HOST 또는 SPI3_HOST)
 * @param mosi    SPI MOSI 핀 번호
 * @param sclk    SPI 클럭 핀 번호
 * @param cs      Chip Select 핀 번호
 * @param dc      Data/Command 핀 번호
 * @param rst     Reset 핀 번호
 * @param bl      Backlight 핀 번호
 * @return        ESP_OK: 성공, 그 외: 에러 코드
 */
esp_err_t st7789_init(st7789_t *lcd, spi_host_device_t host, int mosi, int sclk, int cs, int dc, int rst, int bl);

/**
 * LCD 그리기 영역(윈도우) 설정
 *
 * 픽셀 데이터를 쓸 화면 영역을 지정합니다.
 * 이후 st7789_write_data()로 전송되는 데이터는 이 영역에 순차적으로 기록됩니다.
 *
 * @param lcd  ST7789 디바이스 구조체 포인터
 * @param x0   시작 X 좌표
 * @param y0   시작 Y 좌표
 * @param x1   끝 X 좌표
 * @param y1   끝 Y 좌표
 */
void st7789_set_window(st7789_t *lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * LCD에 픽셀 데이터 전송
 *
 * st7789_set_window()로 설정된 영역에 픽셀 데이터를 씁니다.
 * 데이터는 RGB565 포맷 (픽셀당 2바이트)으로 전송됩니다.
 *
 * @param lcd   ST7789 디바이스 구조체 포인터
 * @param data  픽셀 데이터 배열
 * @param len   데이터 길이 (바이트 단위)
 */
void st7789_write_data(st7789_t *lcd, const uint8_t *data, size_t len);

/**
 * 사각형 영역을 단일 색상으로 채우기
 *
 * 지정된 영역을 하나의 색상으로 빠르게 채웁니다.
 *
 * @param lcd    ST7789 디바이스 구조체 포인터
 * @param x      시작 X 좌표
 * @param y      시작 Y 좌표
 * @param w      사각형 너비
 * @param h      사각형 높이
 * @param color  채울 색상 (RGB565 포맷)
 */
void st7789_fill_rect(st7789_t *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

#endif
