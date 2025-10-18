# PN-VM102 진동모터 사용 가이드

## 📌 핀 설정

### **하드웨어 연결**
```
ESP32 DevKit          트랜지스터 회로          PN-VM102 모터
GPIO 4 (PWM)    →    베이스 (1kΩ 저항)    →    모터 제어
3.3V            →    컬렉터              →    모터 (+)
GND             →    에미터              →    모터 (-)
```

### **필요한 부품**
- **PN-VM102 진동모터** 1개
- **2N2222 NPN 트랜지스터** 1개
- **1kΩ 저항** 1개
- **1N4007 다이오드** 1개 (역기전력 보호용)

### **회로도**
```
3.3V (또는 외부 전원)
  |
  +----[PN-VM102 모터]----+
  |                       |
  |                    [1N4007 다이오드]
  |                       |
  +----[2N2222 트랜지스터]----+
  |                       |
  |                    [1kΩ 저항]
  |                       |
  +---- GPIO 4 (ESP32)    |
  |
 GND
```

## ⚙️ 소프트웨어 설정

### **config.h 설정**
```c
// ========== 진동모터 설정 ==========
#define VIBRATION_MOTOR_GPIO 4          // 진동모터 PWM 핀 (GPIO 4)
#define VIBRATION_MOTOR_CHANNEL 0       // PWM 채널 (0~7)
#define VIBRATION_MOTOR_FREQ 1000       // PWM 주파수 (1kHz)
#define VIBRATION_MOTOR_RESOLUTION 8    // PWM 해상도 (8bit = 0~255)
#define VIBRATION_MOTOR_MAX_DUTY 200    // 최대 듀티 사이클 (200/255 ≈ 78%)
```

## 🎮 MQTT 명령어

### **1. 기본 진동 명령**
```json
{
  "command": "vibrate",
  "intensity": 50,
  "duration": 1000
}
```
- `intensity`: 진동 강도 (0~100%)
- `duration`: 진동 지속 시간 (밀리초)

### **2. 진동 정지 명령**
```json
{
  "command": "vibrate_stop"
}
```

### **3. 진동 패턴 명령**
```json
{
  "command": "vibrate_pattern",
  "interval": 200
}
```
- `interval`: 패턴 간격 (밀리초)

## 🔧 API 함수

### **초기화**
```c
bool vibration_motor_init(void);
```

### **진동 시작**
```c
bool vibration_motor_start(uint8_t intensity, uint32_t duration_ms);
```

### **진동 정지**
```c
bool vibration_motor_stop(void);
```

### **상태 확인**
```c
bool vibration_motor_is_running(void);
```

### **강도 설정**
```c
bool vibration_motor_set_intensity(uint8_t intensity);
```

### **패턴 실행**
```c
bool vibration_motor_run_pattern(const uint8_t* pattern, uint8_t pattern_length, uint32_t interval_ms);
```

## 📝 사용 예시

### **MQTT 명령 테스트**
```bash
# 기본 진동 (50% 강도, 1초간)
mosquitto_pub -h <BROKER_IP> -t "watchtower/command/joystick" \
  -m '{"command":"vibrate","intensity":50,"duration":1000}'

# 강한 진동 (80% 강도, 2초간)
mosquitto_pub -h <BROKER_IP> -t "watchtower/command/joystick" \
  -m '{"command":"vibrate","intensity":80,"duration":2000}'

# 진동 정지
mosquitto_pub -h <BROKER_IP> -t "watchtower/command/joystick" \
  -m '{"command":"vibrate_stop"}'

# 진동 패턴 실행
mosquitto_pub -h <BROKER_IP> -t "watchtower/command/joystick" \
  -m '{"command":"vibrate_pattern","interval":300}'
```

### **C 코드에서 직접 사용**
```c
#include "vibration_motor.h"

// 진동모터 초기화
vibration_motor_init();

// 진동 시작 (70% 강도, 1.5초간)
vibration_motor_start(70, 1500);

// 진동 정지
vibration_motor_stop();

// 패턴 실행
uint8_t pattern[] = {30, 0, 50, 0, 70, 0};
vibration_motor_run_pattern(pattern, 6, 200);
```

## ⚠️ 주의사항

1. **전원 공급**: PN-VM102는 3.3V에서 동작하지만, 전류가 필요할 수 있으므로 외부 전원 사용을 권장합니다.

2. **트랜지스터 사용**: ESP32의 GPIO 핀은 직접 모터를 구동하기에는 전류가 부족하므로 반드시 트랜지스터를 사용해야 합니다.

3. **다이오드 보호**: 모터의 역기전력으로부터 회로를 보호하기 위해 다이오드를 반드시 설치하세요.

4. **PWM 설정**: 현재 설정된 최대 듀티 사이클은 78%로, 모터 보호를 위해 제한되어 있습니다.

## 🔍 문제 해결

### **진동이 작동하지 않음**
1. GPIO 4 핀 연결 확인
2. 트랜지스터 회로 확인
3. 전원 공급 확인
4. 시리얼 모니터에서 초기화 로그 확인

### **진동이 너무 약함**
1. `VIBRATION_MOTOR_MAX_DUTY` 값 증가 (최대 255)
2. 외부 전원 사용
3. 트랜지스터 교체

### **진동이 계속됨**
1. `vibration_motor_stop()` 호출
2. 타이머 설정 확인
3. PWM 채널 확인

## 📊 현재 핀 사용 현황

| 핀 | 용도 | 상태 |
|---|---|---|
| GPIO 21 | MPU6050 SDA | 사용 중 |
| GPIO 22 | MPU6050 SCL | 사용 중 |
| GPIO 19 | 버튼 입력 | 사용 중 |
| GPIO 4 | 진동모터 PWM | **새로 추가** |

## 🚀 빌드 및 플래싱

```bash
cd /home/ubuntu/New_Workout_Plan/mpu6050_mqtt
get_idf
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

**업데이트 날짜**: 2025-01-16  
**버전**: 1.0

