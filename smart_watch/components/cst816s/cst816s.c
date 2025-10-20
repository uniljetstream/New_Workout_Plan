/**
 * CST816S 터치 센서 드라이버 구현
 *
 * I2C 통신을 사용하여 CST816S 터치 컨트롤러를 제어합니다.
 */

#include "cst816s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "CST816S";

// ============================================================================
// 공개 함수 (Public Functions)
// ============================================================================

/**
 * CST816S 터치 센서 초기화 함수
 *
 * 터치 센서를 사용하기 위한 모든 초기화 작업을 수행합니다:
 * 1. I2C 마스터 모드 설정
 * 2. I2C 드라이버 설치
 * 3. GPIO 핀 설정 (RST, INT)
 * 4. 하드웨어 리셋
 * 5. 자동 슬립 모드 비활성화
 *
 * @param touch  CST816S 디바이스 구조체 포인터
 * @param port   I2C 포트 번호 (I2C_NUM_0 또는 I2C_NUM_1)
 * @param sda    I2C SDA 핀 번호
 * @param scl    I2C SCL 핀 번호
 * @param rst    Reset 핀 번호
 * @param irq    Interrupt 핀 번호
 * @return       ESP_OK: 성공, 그 외: 에러 코드
 */
esp_err_t cst816s_init(cst816s_t *touch, i2c_port_t port, int sda, int scl, int rst, int irq)
{
    esp_err_t ret;

    // 터치 구조체 초기화
    touch->i2c_port = port;
    touch->sda_io = sda;
    touch->scl_io = scl;
    touch->rst_io = rst;
    touch->int_io = irq;

    // ========================================================================
    // 1. I2C 마스터 모드 설정
    // ========================================================================
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,           // 마스터 모드 (ESP32가 제어)
        .sda_io_num = sda,                 // SDA 핀
        .scl_io_num = scl,                 // SCL 핀
        .sda_pullup_en = GPIO_PULLUP_ENABLE,  // SDA 풀업 저항 활성화
        .scl_pullup_en = GPIO_PULLUP_ENABLE,  // SCL 풀업 저항 활성화
        .master.clk_speed = 400000,        // 400kHz (Fast Mode I2C)
    };

    ret = i2c_param_config(port, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C 파라미터 설정 실패");
        return ret;
    }

    // ========================================================================
    // 2. I2C 드라이버 설치
    // ========================================================================
    ret = i2c_driver_install(port, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C 드라이버 설치 실패");
        return ret;
    }

    // ========================================================================
    // 3. GPIO 핀 설정
    // ========================================================================

    // RST 핀: 출력 모드 (터치 센서를 리셋하기 위해)
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,     // 출력 모드
        .pin_bit_mask = (1ULL << rst) // RST 핀 비트마스크
    };
    gpio_config(&io_conf);

    // INT 핀: 입력 모드 (터치 발생 시 인터럽트 신호 수신)
    io_conf.mode = GPIO_MODE_INPUT;       // 입력 모드
    io_conf.pin_bit_mask = (1ULL << irq); // INT 핀 비트마스크
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // 풀업 저항 활성화
    gpio_config(&io_conf);

    // ========================================================================
    // 4. 하드웨어 리셋
    // ========================================================================
    gpio_set_level(rst, 0);  // RST 핀을 LOW로 (리셋 활성화)
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(rst, 1);  // RST 핀을 HIGH로 (리셋 해제)
    vTaskDelay(pdMS_TO_TICKS(50));

    // ========================================================================
    // 5. 자동 슬립 모드 비활성화
    // ========================================================================
    // 레지스터 0xFE에 0x01을 써서 자동 슬립 기능 끄기
    // (터치가 없어도 센서가 계속 동작하도록)
    uint8_t cmd[] = {0xFE, 0x01};
    i2c_master_write_to_device(port, CST816S_ADDR, cmd, 2, pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "CST816S 초기화 완료");
    return ESP_OK;
}

/**
 * 터치 데이터 읽기 함수
 *
 * CST816S의 레지스터에서 터치 좌표와 제스처 정보를 읽어옵니다.
 *
 * 레지스터 맵 (0x02부터 6바이트 읽기):
 * - 0x02: 터치 포인트 개수 (하위 4비트)
 * - 0x03-0x04: X 좌표 (12비트)
 * - 0x05-0x06: Y 좌표 (12비트)
 * - 0x07: 제스처 타입
 *
 * @param touch  CST816S 디바이스 구조체 포인터
 * @return       true: 터치 감지됨, false: 터치 없음 또는 에러
 */
bool cst816s_read(cst816s_t *touch)
{
    uint8_t data[6];  // 6바이트 터치 데이터 버퍼
    esp_err_t ret;

    // ========================================================================
    // 레지스터 0x02부터 6바이트 읽기
    // ========================================================================
    uint8_t reg = 0x02;  // 시작 레지스터 주소
    ret = i2c_master_write_read_device(
        touch->i2c_port,     // I2C 포트
        CST816S_ADDR,        // 슬레이브 주소 (0x15)
        &reg, 1,             // 읽을 레지스터 주소 (1바이트)
        data, 6,             // 읽은 데이터 저장 버퍼 (6바이트)
        pdMS_TO_TICKS(100)   // 타임아웃 (100ms)
    );

    if (ret != ESP_OK) {
        // I2C 통신 실패
        return false;
    }

    // ========================================================================
    // 터치 포인트 개수 파싱
    // ========================================================================
    touch->data.points = data[0] & 0x0F;  // 하위 4비트만 사용

    if (touch->data.points == 0) {
        // 터치 없음
        return false;
    }

    // ========================================================================
    // 좌표 데이터 파싱
    // ========================================================================
    // X 좌표: data[1]의 하위 4비트 + data[2] (총 12비트)
    touch->data.x = ((data[1] & 0x0F) << 8) | data[2];

    // Y 좌표: data[3]의 하위 4비트 + data[4] (총 12비트)
    touch->data.y = ((data[3] & 0x0F) << 8) | data[4];

    // 제스처 타입
    touch->data.gesture = data[5];

    // 터치 감지됨
    return true;
}
