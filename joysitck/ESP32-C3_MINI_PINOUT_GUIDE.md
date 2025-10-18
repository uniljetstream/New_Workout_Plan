# ESP32-C3 Mini 핀 연결 가이드

## ESP32 vs ESP32-C3 Mini 핀 차이점

ESP32와 ESP32-C3 Mini는 **완전히 다른 핀 배치**를 가지고 있습니다!

### 주요 차이점
- **ESP32**: GPIO 21, 22를 I2C로 사용
- **ESP32-C3 Mini**: GPIO 8, 10을 I2C로 사용
- **핀 개수**: ESP32-C3 Mini가 더 적은 핀을 가짐
- **스트래핑 핀**: ESP32-C3 Mini는 GPIO 2, 8, 9가 부트 모드 관련

## ESP32-C3 Mini 핀 연결 방법

### 1. MPU6050 센서 연결
```
MPU6050    →    ESP32-C3 Mini
VCC        →    3.3V
GND        →    GND
SCL        →    GPIO10
SDA        →    GPIO8
```

### 2. 버튼 연결
```
버튼 한쪽  →    GPIO2 (BOOT 버튼과 같은 핀)
버튼 다른쪽 →    GND
```

### 3. 진동모터 연결
```
진동모터 양극  →    GPIO5
진동모터 음극  →    GND
```

## ESP32-C3 Mini 사용 가능한 GPIO 핀

### ✅ 사용 가능한 핀
- **GPIO 0**: 일반 GPIO (주의: 부트 모드 관련)
- **GPIO 1**: 일반 GPIO (주의: 부트 모드 관련)
- **GPIO 2**: BOOT 버튼 (사용 가능하지만 주의)
- **GPIO 3**: 일반 GPIO
- **GPIO 4**: 일반 GPIO
- **GPIO 5**: 일반 GPIO (진동모터용으로 사용)
- **GPIO 6**: 일반 GPIO
- **GPIO 7**: 일반 GPIO
- **GPIO 8**: I2C SDA (MPU6050용)
- **GPIO 9**: 일반 GPIO (주의: 부트 모드 관련)
- **GPIO 10**: I2C SCL (MPU6050용)

### ❌ 사용 불가능한 핀
- **GPIO 11-17**: 내부 SPI 플래시와 연결됨 (사용 금지)
- **GPIO 18-21**: USB 관련 핀 (사용 금지)

## 주의사항

1. **전압 레벨**: ESP32-C3 Mini는 3.3V 로직 사용
2. **풀업 저항**: I2C 통신 안정성을 위해 4.7kΩ 풀업 저항 권장
3. **스트래핑 핀**: GPIO 2, 8, 9는 부트 모드와 관련되어 있으므로 주의
4. **핀 개수**: ESP32보다 적은 핀을 가지므로 핀 배치에 주의

## 코드 변경사항

`config.h` 파일에서 다음 핀들이 변경되었습니다:

```c
// ESP32 → ESP32-C3 Mini 변경
#define I2C_MASTER_SCL_IO 22 → 10    // SCL 핀 변경
#define I2C_MASTER_SDA_IO 21 → 8     // SDA 핀 변경
#define BUTTON_GPIO 19 → 2           // 버튼 핀 변경
#define VIBRATION_MOTOR_GPIO 4 → 5   // 진동모터 핀 변경
```

이제 ESP32-C3 Mini에서 동일한 기능을 사용할 수 있습니다!
