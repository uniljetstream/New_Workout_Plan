/**
 * CST816S 정전식 터치 센서 드라이버
 *
 * CST816S는 I2C 통신을 사용하는 단일 터치 컨트롤러입니다.
 * 터치 좌표, 제스처 인식 기능을 제공합니다.
 *
 * 주요 기능:
 * - 단일 터치 좌표 감지
 * - 제스처 인식 (스와이프, 더블 탭 등)
 * - 인터럽트 기반 터치 감지 지원
 */

#ifndef CST816S_H
#define CST816S_H

#include "driver/i2c.h"
#include "driver/gpio.h"

// ============================================================================
// CST816S 상수 정의
// ============================================================================

#define CST816S_ADDR 0x15  // CST816S I2C 슬레이브 주소

// ============================================================================
// 데이터 구조체
// ============================================================================

/**
 * 터치 데이터 구조체
 *
 * 터치 센서에서 읽은 좌표 및 제스처 정보를 저장합니다.
 */
typedef struct {
    uint16_t x;         // X 좌표 (0 ~ 화면 너비)
    uint16_t y;         // Y 좌표 (0 ~ 화면 높이)
    uint8_t gesture;    // 제스처 타입 (0=없음, 1=위로 스와이프, 2=아래로 스와이프 등)
    uint8_t points;     // 터치 포인트 개수 (0=터치 없음, 1=터치됨)
} cst816s_data_t;

/**
 * CST816S 디바이스 구조체
 *
 * 터치 센서 제어에 필요한 I2C 포트와 GPIO 핀 정보를 저장합니다.
 */
typedef struct {
    i2c_port_t i2c_port;      // I2C 포트 번호 (I2C_NUM_0 또는 I2C_NUM_1)
    int sda_io;               // I2C SDA (데이터) 핀
    int scl_io;               // I2C SCL (클럭) 핀
    int rst_io;               // Reset 핀 (터치 센서 리셋)
    int int_io;               // Interrupt 핀 (터치 발생 시 LOW로 변경됨)
    cst816s_data_t data;      // 마지막으로 읽은 터치 데이터
} cst816s_t;

// ============================================================================
// 함수 선언 (Function Declarations)
// ============================================================================

/**
 * CST816S 터치 센서 초기화
 *
 * I2C 통신을 설정하고 터치 센서를 초기화합니다.
 * GPIO 핀을 설정하고 센서를 리셋합니다.
 *
 * @param touch  CST816S 디바이스 구조체 포인터
 * @param port   I2C 포트 번호 (I2C_NUM_0 또는 I2C_NUM_1)
 * @param sda    I2C SDA 핀 번호
 * @param scl    I2C SCL 핀 번호
 * @param rst    Reset 핀 번호
 * @param irq    Interrupt 핀 번호
 * @return       ESP_OK: 성공, 그 외: 에러 코드
 */
esp_err_t cst816s_init(cst816s_t *touch, i2c_port_t port, int sda, int scl, int rst, int irq);

/**
 * 터치 데이터 읽기
 *
 * CST816S에서 터치 좌표와 제스처 정보를 읽어옵니다.
 * 터치가 감지되면 touch->data 구조체에 좌표가 저장됩니다.
 *
 * @param touch  CST816S 디바이스 구조체 포인터
 * @return       true: 터치 감지됨, false: 터치 없음
 */
bool cst816s_read(cst816s_t *touch);

#endif
