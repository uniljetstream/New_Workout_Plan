# ESP32 버튼 GPIO 핀 연결 가이드

## 📌 버튼 GPIO 핀 번호

### **GPIO 0 (기본 설정)** ⭐ 권장

- **핀 이름**: BOOT 버튼 / GPIO0
- **위치**: ESP32 개발 보드에 기본 내장되어 있음
- **연결**: 별도 연결 불필요 (보드의 BOOT 버튼 사용)
- **특징**:
  - Active LOW (버튼 누르면 0V, 떼면 3.3V)
  - 내장 풀업 저항 사용
  - 별도 하드웨어 없이 즉시 테스트 가능

### 하드웨어 연결 (BOOT 버튼 사용 시)

```
[ESP32 개발 보드]
     BOOT 버튼 (GPIO 0)
         ↓
   별도 연결 불필요
```

**BOOT 버튼 위치**:
- ESP32 DevKit: 보드 왼쪽 하단의 "BOOT" 버튼
- ESP32-WROOM-32: 보드에 표시된 "IO0" 또는 "BOOT" 버튼

---

## 🔧 다른 GPIO 핀 사용하기

BOOT 버튼 대신 외부 버튼을 연결하려면:

### 권장 GPIO 핀
- **GPIO 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 23, 25, 26, 27, 32, 33**

### 피해야 할 GPIO 핀
- **GPIO 6~11**: SPI Flash 연결 (사용 금지)
- **GPIO 34~39**: 입력 전용 (풀업 불가능)
- **GPIO 0, 2**: 부팅 시 특별한 역할 (주의)
- **GPIO 21, 22**: I2C (MPU6050 사용 중)

### 외부 버튼 회로도

#### Active LOW (권장)
```
3.3V
  |
 [10kΩ 풀업 저항]
  |
  +------+------ GPIO 핀
  |      |
 [버튼]  [ESP32]
  |
 GND
```

#### Active HIGH
```
GPIO 핀 --+
          |
        [버튼]
          |
         GND
```

---

## ⚙️ 설정 변경하기

다른 GPIO 핀을 사용하려면 `config.h` 파일을 수정하세요:

**파일**: `mpu6050_mqtt/main/config.h`

```c
// ========== 버튼 GPIO 설정 ==========
#define BUTTON_GPIO 0                  // 변경: GPIO 핀 번호
#define BUTTON_ACTIVE_LEVEL 0          // 0 = Active LOW, 1 = Active HIGH
#define BUTTON_DEBOUNCE_MS 50          // 디바운싱 시간 (50ms)
```

### 설정 예시

#### GPIO 4 사용 (Active LOW)
```c
#define BUTTON_GPIO 4
#define BUTTON_ACTIVE_LEVEL 0
#define BUTTON_DEBOUNCE_MS 50
```

#### GPIO 15 사용 (Active HIGH)
```c
#define BUTTON_GPIO 15
#define BUTTON_ACTIVE_LEVEL 1
#define BUTTON_DEBOUNCE_MS 50
```

---

## 🧪 테스트 방법

### 1. ESP32 플래싱 후 시리얼 모니터 확인

```bash
cd mpu6050_mqtt
get_idf
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 2. 버튼 초기화 로그 확인

```
I (1234) BUTTON: Initializing button on GPIO 0...
I (1235) BUTTON: Button initialized successfully
I (1236) BUTTON: Button configuration: GPIO=0, Active Level=0, Debounce=50ms
```

### 3. 에어마우스 모드로 전환

MQTT 명령 전송:
```bash
mosquitto_pub -h <BROKER_IP> -t "watchtower/command/joystick" \
  -m '{"command":"airmouse_mode"}'
```

### 4. 버튼 눌러보기

BOOT 버튼을 누르면 시리얼 모니터에:
```
I (5678) MQTT: Mouse: X=0.00 Y=0.00 | Scroll: 0 | Button: PRESSED
I (5878) MQTT: Mouse: X=0.00 Y=0.00 | Scroll: 0 | Button: RELEASED
```

### 5. Qt 앱에서 확인

Qt 앱을 실행하고 에어마우스 모드로 전환한 후, BOOT 버튼을 누르면:
```
Button pressed - simulating click
Click simulated on "<button_name>"
```

---

## 🛠️ 문제 해결

### 버튼이 동작하지 않음
1. GPIO 핀 번호 확인
2. Active Level 설정 확인 (풀업이면 Active LOW)
3. 시리얼 모니터에서 버튼 초기화 로그 확인
4. `button_is_pressed()` 반환값 확인

### 버튼이 계속 눌려있는 것처럼 동작
1. BUTTON_ACTIVE_LEVEL을 반대로 설정 (0↔1)
2. 풀업/풀다운 저항 확인
3. 디바운싱 시간 증가 (`BUTTON_DEBOUNCE_MS 100`)

### 버튼 클릭이 여러 번 인식됨
1. 디바운싱 시간 증가
2. 버튼 품질 확인 (바운싱이 심한 버튼)

---

## 📚 핀 배치도

### ESP32 DevKit v1 핀 배치

```
                        ESP32 DevKit v1
                     +------------------+
                     |                  |
                     |      [USB]       |
                     |                  |
   (EN 버튼)  ------| EN            3V3|------ 3.3V
(BOOT 버튼)  GPIO 0 | VP   (GPIO 36) NC|
                     | VN   (GPIO 39) NC|
            GPIO 34  | 34            21 | GPIO 21 (I2C SDA - MPU6050)
            GPIO 35  | 35            22 | GPIO 22 (I2C SCL - MPU6050)
            GPIO 32  | 32            TX0|
            GPIO 33  | 33            RX0|
            GPIO 25  | 25            19 | GPIO 19
            GPIO 26  | 26            18 | GPIO 18
            GPIO 27  | 27             5 | GPIO 5
            GPIO 14  | 14            17 | GPIO 17
            GPIO 12  | 12            16 | GPIO 16
                 GND |-GND            4 | GPIO 4
            GPIO 13  | 13             2 | GPIO 2
            GPIO 15  | 15        GND   |------ GND
                     +------------------+
```

### 사용 가능한 GPIO (외부 버튼용)
- ✅ **GPIO 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 23, 25, 26, 27, 32, 33**
- ❌ **GPIO 21, 22**: MPU6050 I2C (사용 중)
- ❌ **GPIO 6~11**: SPI Flash (사용 금지)
- ⚠️ **GPIO 0, 2**: 부팅 시 주의

---

## 📝 요약

1. **기본 설정**: GPIO 0 (BOOT 버튼) - 별도 연결 불필요
2. **외부 버튼**: GPIO 4, 5, 12, 13 등 권장
3. **설정 파일**: `config.h`에서 핀 번호 변경
4. **테스트**: 시리얼 모니터 + Qt 앱에서 클릭 동작 확인

---

**업데이트 날짜**: 2025-10-14
**버전**: 1.0
