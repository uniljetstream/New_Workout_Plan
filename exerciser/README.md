# Exerciser Vibration Motor Controller

ESP32-C3 기반 진동모터 제어 프로젝트입니다. MQTT 메시지를 수신하여 진동모터를 제어합니다.

## 하드웨어 요구사항

- ESP32-C3 개발 보드
- 진동모터 (DC 모터)
- 트랜지스터 또는 MOSFET (모터 드라이버용)
- 다이오드 (플라이백 보호)

## 회로 연결

기본 설정으로 GPIO2에 진동모터가 연결되어 있습니다.

```
ESP32-C3 GPIO2 -> 트랜지스터 베이스 (저항 통해)
트랜지스터 컬렉터 -> 진동모터 (+)
진동모터 (-) -> GND
다이오드 -> 진동모터 양단에 병렬 연결 (역방향 보호)
```

## 설정

[main/main.c](main/main.c) 파일에서 다음 항목들을 수정하세요:

- `WIFI_SSID`: WiFi 네트워크 이름
- `WIFI_PASS`: WiFi 비밀번호
- `MQTT_BROKER`: MQTT 브로커 주소 (기본값: mqtt://10.10.16.111)
- `MQTT_TOPIC`: MQTT 토픽 (기본값: exerciser/vibration)
- `VIBRATION_MOTOR_GPIO`: 진동모터 GPIO 핀 (기본값: GPIO_NUM_2)

## 빌드 및 플래시

```bash
cd exerciser
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## 사용법

MQTT 메시지를 `exerciser/vibration` 토픽으로 전송하면 진동모터가 작동합니다.

### 예시

```bash
# 200ms 진동 (기본값)
mosquitto_pub -h 10.10.16.111 -t "exerciser/vibration" -m "vibrate"

# 500ms 진동
mosquitto_pub -h 10.10.16.111 -t "exerciser/vibration" -m "500"

# 1000ms 진동
mosquitto_pub -h 10.10.16.111 -t "exerciser/vibration" -m "1000"
```

메시지 내용이 숫자인 경우 해당 ms 동안 진동하며, 숫자가 아닌 경우 기본값 200ms 동안 진동합니다.

## 기능

- WiFi 자동 연결 및 재연결
- MQTT 브로커 연결
- MQTT 메시지 수신 시 진동모터 작동
- 진동 시간을 메시지로 제어 가능 (1-5000ms)
- 시작 시 테스트 진동 (500ms)
