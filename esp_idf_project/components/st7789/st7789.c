/**
 * ST7789 LCD 드라이버 구현
 *
 * SPI 통신을 사용하여 ST7789 LCD 컨트롤러를 제어합니다.
 */

#include "st7789.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ST7789";

// ============================================================================
// 내부 함수 (Private Functions)
// ============================================================================

/**
 * LCD에 명령(Command) 전송
 *
 * DC 핀을 LOW로 설정하여 명령 모드로 진입한 후 1바이트를 전송합니다.
 *
 * @param lcd ST7789 디바이스 구조체 포인터
 * @param cmd 전송할 명령 바이트
 */
static void st7789_write_cmd(st7789_t *lcd, uint8_t cmd)
{
    gpio_set_level(lcd->dc_io, 0);  // 명령 모드 (Command mode)
    spi_transaction_t t = {
        .length = 8,                // 8비트 전송
        .tx_buffer = &cmd,          // 명령 바이트 주소
        .flags = 0
    };
    spi_device_polling_transmit(lcd->spi, &t);
}

/**
 * LCD에 1바이트 데이터 전송
 *
 * DC 핀을 HIGH로 설정하여 데이터 모드로 진입한 후 1바이트를 전송합니다.
 *
 * @param lcd  ST7789 디바이스 구조체 포인터
 * @param data 전송할 데이터 바이트
 */
static void st7789_write_data_byte(st7789_t *lcd, uint8_t data)
{
    gpio_set_level(lcd->dc_io, 1);  // 데이터 모드 (Data mode)
    spi_transaction_t t = {
        .length = 8,                // 8비트 전송
        .tx_buffer = &data,         // 데이터 바이트 주소
        .flags = 0
    };
    spi_device_polling_transmit(lcd->spi, &t);
}

/**
 * LCD에 여러 바이트 데이터 전송
 *
 * DC 핀을 HIGH로 설정하여 데이터 모드로 진입한 후 여러 바이트를 전송합니다.
 * 주로 픽셀 데이터 전송에 사용됩니다.
 *
 * @param lcd  ST7789 디바이스 구조체 포인터
 * @param data 전송할 데이터 배열
 * @param len  데이터 길이 (바이트 단위)
 */
void st7789_write_data(st7789_t *lcd, const uint8_t *data, size_t len)
{
    if (len == 0) return;  // 길이가 0이면 아무것도 하지 않음

    gpio_set_level(lcd->dc_io, 1);  // 데이터 모드 (Data mode)

    const size_t chunk_size = 32 * 1024;  // ESP32-C6 DMA 전송 한계(32KB) 회피
    size_t remaining = len;
    const uint8_t *cursor = data;

    while (remaining > 0) {
        size_t current = remaining > chunk_size ? chunk_size : remaining;
        spi_transaction_t t = {
            .length = current * 8,  // 비트 단위로 변환 (바이트 * 8)
            .tx_buffer = cursor,
            .flags = 0
        };

        esp_err_t ret = spi_device_polling_transmit(lcd->spi, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI 전송 실패: %s", esp_err_to_name(ret));
            return;
        }

        cursor += current;
        remaining -= current;
    }
}

// ============================================================================
// 공개 함수 (Public Functions)
// ============================================================================

/**
 * ST7789 LCD 초기화 함수
 *
 * LCD를 사용하기 위한 모든 초기화 작업을 수행합니다:
 * 1. GPIO 핀 설정 (DC, RST, BL)
 * 2. SPI 버스 초기화
 * 3. SPI 디바이스 추가
 * 4. 하드웨어 리셋
 * 5. LCD 초기화 명령 시퀀스 실행
 * 6. 백라이트 켜기
 *
 * @param lcd   ST7789 디바이스 구조체 포인터
 * @param host  SPI 호스트 (SPI2_HOST 또는 SPI3_HOST)
 * @param mosi  SPI MOSI 핀 번호
 * @param sclk  SPI 클럭 핀 번호
 * @param cs    Chip Select 핀 번호
 * @param dc    Data/Command 핀 번호
 * @param rst   Reset 핀 번호
 * @param bl    Backlight 핀 번호
 * @return      ESP_OK: 성공, 그 외: 에러 코드
 */
esp_err_t st7789_init(st7789_t *lcd, spi_host_device_t host, int mosi, int sclk, int cs, int dc, int rst, int bl)
{
    esp_err_t ret;

    // LCD 구조체 초기화
    lcd->dc_io = dc;
    lcd->rst_io = rst;
    lcd->bl_io = bl;
    lcd->width = 240;   // 화면 가로 해상도
    lcd->height = 280;  // 화면 세로 해상도

    // ========================================================================
    // 1. GPIO 핀 설정 (DC, RST, BL)
    // ========================================================================
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,  // 출력 모드
        .pin_bit_mask = (1ULL << dc) | (1ULL << rst) | (1ULL << bl)  // 비트마스크
    };
    gpio_config(&io_conf);

    // ========================================================================
    // 2. SPI 버스 초기화
    // ========================================================================
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,       // MOSI 핀
        .miso_io_num = -1,         // MISO 사용 안 함 (LCD는 단방향 통신)
        .sclk_io_num = sclk,       // 클럭 핀
        .quadwp_io_num = -1,       // Quad SPI 사용 안 함
        .quadhd_io_num = -1,       // Quad SPI 사용 안 함
        .max_transfer_sz = 240 * 280 * 2 + 8  // 최대 전송 크기 (전체 화면 + 여유)
    };

    ret = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI 버스 초기화 실패");
        return ret;
    }

    // ========================================================================
    // 3. SPI 디바이스 추가
    // ========================================================================
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 26 * 1000 * 1000,  // 26 MHz (ESP32 최대 안정 속도)
        .mode = 0,                           // SPI 모드 0 (CPOL=0, CPHA=0)
        .spics_io_num = cs,                  // CS 핀
        .queue_size = 7,                     // 트랜잭션 큐 크기
        .flags = 0,
    };

    ret = spi_bus_add_device(host, &devcfg, &lcd->spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI 디바이스 추가 실패");
        return ret;
    }

    // ========================================================================
    // 4. 하드웨어 리셋
    // ========================================================================
    gpio_set_level(rst, 0);  // RST 핀을 LOW로 (리셋 활성화)
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(rst, 1);  // RST 핀을 HIGH로 (리셋 해제)
    vTaskDelay(pdMS_TO_TICKS(100));

    // ========================================================================
    // 5. LCD 초기화 명령 시퀀스
    // ========================================================================

    // 소프트웨어 리셋
    st7789_write_cmd(lcd, ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    // 슬립 모드 해제
    st7789_write_cmd(lcd, ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 색상 모드 설정 (RGB565, 16비트 컬러)
    st7789_write_cmd(lcd, ST7789_COLMOD);
    st7789_write_data_byte(lcd, 0x55);  // 16-bit color (RGB565)

    // 메모리 액세스 제어 (화면 방향 설정)
    st7789_write_cmd(lcd, ST7789_MADCTL);
    st7789_write_data_byte(lcd, 0x00);  // 기본 방향

    // 색상 반전 켜기 (일부 LCD는 반전이 필요함)
    st7789_write_cmd(lcd, ST7789_INVON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 디스플레이 켜기
    st7789_write_cmd(lcd, ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // ========================================================================
    // 6. 백라이트 켜기
    // ========================================================================
    gpio_set_level(bl, 1);

    ESP_LOGI(TAG, "ST7789 초기화 완료");
    return ESP_OK;
}

/**
 * LCD 그리기 영역(윈도우) 설정
 *
 * 픽셀을 쓸 화면 영역을 지정합니다.
 * ST7789의 CASET (Column Address Set)와 RASET (Row Address Set) 명령을 사용합니다.
 *
 * 주의: 240x280 디스플레이의 경우 Y 좌표에 20픽셀 오프셋이 추가됩니다.
 * (일부 ST7789 모듈은 물리적 디스플레이 영역이 오프셋되어 있음)
 *
 * @param lcd  ST7789 디바이스 구조체 포인터
 * @param x0   시작 X 좌표 (0 ~ 239)
 * @param y0   시작 Y 좌표 (0 ~ 279)
 * @param x1   끝 X 좌표 (0 ~ 239)
 * @param y1   끝 Y 좌표 (0 ~ 279)
 */
void st7789_set_window(st7789_t *lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // 240x280 디스플레이를 위한 오프셋 추가
    // X 좌표는 오프셋 없음
    x0 += 0;
    x1 += 0;
    // Y 좌표는 20픽셀 오프셋 (하드웨어 특성)
    y0 += 20;
    y1 += 20;

    // ========================================================================
    // Column Address Set (X 좌표 범위 설정)
    // ========================================================================
    st7789_write_cmd(lcd, ST7789_CASET);
    uint8_t col_data[4] = {
        x0 >> 8,    // 시작 X의 상위 바이트
        x0 & 0xFF,  // 시작 X의 하위 바이트
        x1 >> 8,    // 끝 X의 상위 바이트
        x1 & 0xFF   // 끝 X의 하위 바이트
    };
    st7789_write_data(lcd, col_data, 4);

    // ========================================================================
    // Row Address Set (Y 좌표 범위 설정)
    // ========================================================================
    st7789_write_cmd(lcd, ST7789_RASET);
    uint8_t row_data[4] = {
        y0 >> 8,    // 시작 Y의 상위 바이트
        y0 & 0xFF,  // 시작 Y의 하위 바이트
        y1 >> 8,    // 끝 Y의 상위 바이트
        y1 & 0xFF   // 끝 Y의 하위 바이트
    };
    st7789_write_data(lcd, row_data, 4);

    // ========================================================================
    // Memory Write 명령 전송
    // ========================================================================
    // 이후 전송되는 데이터는 설정된 윈도우 영역에 순차적으로 기록됨
    st7789_write_cmd(lcd, ST7789_RAMWR);
}

/**
 * 사각형 영역을 단일 색상으로 채우기
 *
 * 지정된 영역을 하나의 색상으로 빠르게 채웁니다.
 * 화면 클리어, 배경 그리기 등에 사용할 수 있습니다.
 *
 * @param lcd    ST7789 디바이스 구조체 포인터
 * @param x      사각형 시작 X 좌표
 * @param y      사각형 시작 Y 좌표
 * @param w      사각형 너비
 * @param h      사각형 높이
 * @param color  채울 색상 (RGB565 포맷: 0xRRRRRGGGGGGBBBBB)
 */
void st7789_fill_rect(st7789_t *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // 그리기 영역 설정
    st7789_set_window(lcd, x, y, x + w - 1, y + h - 1);

    // 전체 픽셀 개수 계산
    uint16_t pixels = w * h;

    // RGB565 색상을 바이트 배열로 변환 (Big Endian)
    uint8_t color_data[2] = {
        color >> 8,    // 상위 바이트
        color & 0xFF   // 하위 바이트
    };

    // 모든 픽셀을 같은 색상으로 채움
    for (uint16_t i = 0; i < pixels; i++) {
        st7789_write_data(lcd, color_data, 2);
    }
}
