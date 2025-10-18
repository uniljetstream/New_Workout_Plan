# PN-VM102 진동모터 간단 연결 가이드

## 📌 핀 설정 (간단한 2선식)

### **하드웨어 연결**
```
ESP32 DevKit          PN-VM102 모터
GPIO 4           →    모터 핀 1
GND              →    모터 핀 2
```

**끝!** 트랜지스터, 저항, 다이오드 모두 불필요합니다.

### **필요한 부품**
- **PN-VM102 진동모터** 1개
- **점퍼 와이어** 2개

## ⚙️ 소프트웨어 설정

### **config.h 설정**
```c
// ========== 진동모터 설정 ==========
#define VIBRATION_MOTOR_GPIO 4          // 진동모터 GPIO 핀 (GPIO 4)
#define VIBRATION_MOTOR_ACTIVE_LEVEL 1   // 진동모터 활성 레벨 (1 = HIGH, 0 = LOW)
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
- `intensity`: 무시됨 (단순 ON/OFF 방식)
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

## 📝 사용 예시

### **MQTT 명령 테스트**
```bash
# 진동 시작 (1초간)
mosquitto_pub -h <BROKER_IP> -t "watchtower/command/joystick" \
  -m '{"command":"vibrate","intensity":50,"duration":1000}'

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

// 진동 시작 (1.5초간)
vibration_motor_start(100, 1500);

// 진동 정지
vibration_motor_stop();
```

## ⚠️ 주의사항

1. **전원 공급**: ESP32의 GPIO 핀은 3.3V, 최대 40mA까지 공급 가능합니다.
2. **단순 제어**: 강도 조절은 불가능하고 ON/OFF 방식만 지원됩니다.
3. **지속 시간**: 타이머를 사용하여 자동으로 정지됩니다.

## 🔍 문제 해결

### **진동이 작동하지 않음**
1. GPIO 4 핀 연결 확인
2. GND 연결 확인
3. 시리얼 모니터에서 초기화 로그 확인

### **진동이 계속됨**
1. `vibration_motor_stop()` 호출
2. 타이머 설정 확인

## 📊 현재 핀 사용 현황

| 핀 | 용도 | 상태 |
|---|---|---|
| GPIO 21 | MPU6050 SDA | 사용 중 |
| GPIO 22 | MPU6050 SCL | 사용 중 |
| GPIO 19 | 버튼 입력 | 사용 중 |
| GPIO 4 | 진동모터 GPIO | **새로 추가** |

## 🚀 빌드 및 플래싱

```bash
cd /home/ubuntu/New_Workout_Plan/mpu6050_mqtt
get_idf
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## 💡 동작 원리

1. **진동 시작**: GPIO 4 핀을 HIGH(3.3V)로 설정
2. **진동 정지**: GPIO 4 핀을 LOW(0V)로 설정
3. **타이머**: 설정된 시간 후 자동으로 정지

---

**업데이트 날짜**: 2025-01-16  
**버전**: 2.0 (간단한 GPIO 방식)

