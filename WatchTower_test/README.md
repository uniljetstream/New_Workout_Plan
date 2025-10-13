# WatchTower MQTT 통신 테스트

조이스틱(ESP32)과 Watch(ESP32) MQTT 통신을 CLI 환경에서 테스트하기 위한 도구입니다.

## 📋 개요

이 프로그램은 **MQTT 통신만**을 테스트하기 위해 작성되었습니다.
- ✅ MQTT 브로커 연결
- ✅ 조이스틱 센서 데이터 수신 (가속도, 자이로)
- ✅ Watch 심박수 데이터 수신
- ✅ 디바이스 명령 전송 (start/stop)
- ✅ 실시간 데이터 출력 및 통계
- ❌ AI 서버 (제외)
- ❌ 카메라 (제외)
- ❌ 팬틸트/UART (제외)

## 🚀 시작하기

### 1. 사전 준비

#### MQTT 브로커 설치 및 실행 (mosquitto)

**설치:**
```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients
```

**실행:**
```bash
# 브로커 시작
sudo systemctl start mosquitto

# 부팅 시 자동 시작 설정
sudo systemctl enable mosquitto

# 상태 확인
sudo systemctl status mosquitto
```

**포트 확인:**
```bash
# 1883 포트가 열려있는지 확인
sudo netstat -tulpn | grep 1883
# 또는
sudo lsof -i :1883
```

#### Python 가상환경 활성화

```bash
cd /path/to/New_Workout_Plan
source venv/bin/activate
```

#### 필요한 패키지 설치

```bash
pip install paho-mqtt
```

### 2. 설정

#### mqtt_config.py 수정

프로젝트 환경에 맞게 설정을 수정하세요:

```python
class MQTTTestConfig:
    # MQTT 브로커 설정
    MQTT_BROKER_HOST = 'localhost'     # Jetson Nano에서 실행시 localhost
    MQTT_BROKER_PORT = 1883

    # 디스플레이 옵션
    SHOW_TIMESTAMP = True              # 타임스탬프 출력 여부
    SHOW_RAW_JSON = False              # Raw JSON 출력 여부 (디버깅용)
```

**원격 테스트:**
다른 PC에서 Jetson Nano의 MQTT 브로커에 접속하려면:
```python
MQTT_BROKER_HOST = '192.168.1.100'  # Jetson Nano IP 주소
```

### 3. 실행

```bash
cd WatchTower_test
python mqtt_test.py
```

## 📖 사용 방법

### 메인 메뉴

```
명령어:
  1 - 디바이스 시작 (start 명령 전송)
  2 - 디바이스 정지 (stop 명령 전송)
  3 - 통계 출력
  4 - 메뉴 다시 보기
  q - 종료
```

### 실행 흐름

1. **프로그램 시작**
   ```bash
   python mqtt_test.py
   ```

2. **MQTT 브로커 연결 확인**
   ```
   ============================================================
     WatchTower MQTT 통신 테스트
   ============================================================
   MQTT 브로커: localhost:1883
   ============================================================

   [초기화]
   → MQTT 브로커 연결 중: localhost:1883
   ✓ MQTT 브로커 연결 성공

     ✓ 토픽 구독: joystick/sensor/data
     ✓ 토픽 구독: watch/sensor/heartrate
     ✓ 토픽 구독: joystick/status
     ✓ 토픽 구독: watch/status
   ```

3. **실시간 센서 데이터 수신**
   ```
   [실시간 센서 데이터]
   ------------------------------------------------------------
   14:23:45.123 | [조이스틱] Accel: (0.52, -0.31, 9.78) | Gyro: (0.01, -0.02, 0.00)
   14:23:45.456 | [Watch] 심박수: 85 bpm
   14:23:45.789 | [조이스틱] Accel: (0.50, -0.33, 9.80) | Gyro: (0.00, -0.01, 0.01)
   14:23:46.012 | [Watch] 심박수: 86 bpm
   ```

4. **명령 전송**
   ```
   > 1  # start 명령 전송

   [명령 전송]
   ✓ 조이스틱에 start 명령 전송: {"command": "start", "mode": "t_pose", "timestamp": 1234567890}
   ✓ Watch에 start 명령 전송: {"command": "start", "mode": "t_pose", "timestamp": 1234567890}
   ```

5. **통계 확인**
   ```
   > 3  # 통계 출력

   ============================================================
   📊 MQTT 통신 통계
   ============================================================
     실행 시간: 45.3초
     명령 전송: 2회

     [조이스틱]
       데이터 수신: 145개 (평균 3.2/sec)
       상태 수신: 2개
       연결 상태: ✓ 활성

     [Watch]
       데이터 수신: 30개 (평균 0.7/sec)
       상태 수신: 2개
       연결 상태: ✓ 활성
   ============================================================
   ```

6. **종료**
   ```
   > q  # 종료

   → 종료 중...

   [종료 처리]

   [명령 전송]
   ✓ 조이스틱에 stop 명령 전송
   ✓ Watch에 stop 명령 전송

   [최종 통계 출력...]

   → MQTT 연결 해제 중...
   ✓ MQTT 연결 해제 완료

   ✓ 테스트 종료
   ```

## 🔍 MQTT 토픽 구조

### WatchTower → 디바이스 (Publish)

**명령 토픽:**
- `watchtower/command/joystick` - 조이스틱 명령
- `watchtower/command/watch` - Watch 명령

**메시지 형식 (start):**
```json
{
  "command": "start",
  "mode": "t_pose",
  "timestamp": 1234567890
}
```

**메시지 형식 (stop):**
```json
{
  "command": "stop",
  "timestamp": 1234567890
}
```

### 디바이스 → WatchTower (Subscribe)

**센서 데이터 토픽:**
- `joystick/sensor/data` - 조이스틱 센서 데이터 (가속도, 자이로)
- `watch/sensor/heartrate` - Watch 심박수 데이터

**상태 토픽:**
- `joystick/status` - 조이스틱 상태 (ready, stopped)
- `watch/status` - Watch 상태 (ready, stopped)

**조이스틱 센서 데이터 형식:**
```json
{
  "accel_x": 0.52,
  "accel_y": -0.31,
  "accel_z": 9.78,
  "gyro_x": 0.01,
  "gyro_y": -0.02,
  "gyro_z": 0.00,
  "timestamp": 1234567890
}
```

**Watch 센서 데이터 형식:**
```json
{
  "heart_rate": 85,
  "timestamp": 1234567890
}
```

**상태 메시지 형식:**
```json
{
  "status": "ready",
  "timestamp": 1234567890
}
```

## 🧪 테스트 시나리오

### 시나리오 1: 조이스틱만 연결

1. 조이스틱(ESP32) 실행
2. `python mqtt_test.py` 실행
3. 조이스틱 데이터만 출력 확인
4. Watch 데이터는 수신되지 않음 (정상)

**예상 출력:**
```
14:23:45.123 | [조이스틱] Accel: (0.52, -0.31, 9.78) | Gyro: (0.01, -0.02, 0.00)
14:23:45.789 | [조이스틱] Accel: (0.50, -0.33, 9.80) | Gyro: (0.00, -0.01, 0.01)
(Watch 데이터 없음)
```

### 시나리오 2: Watch만 연결

1. Watch(ESP32) 실행
2. `python mqtt_test.py` 실행
3. Watch 데이터만 출력 확인
4. 조이스틱 데이터는 수신되지 않음 (정상)

**예상 출력:**
```
14:23:45.456 | [Watch] 심박수: 85 bpm
14:23:46.012 | [Watch] 심박수: 86 bpm
(조이스틱 데이터 없음)
```

### 시나리오 3: 둘 다 연결

1. 조이스틱(ESP32) + Watch(ESP32) 실행
2. `python mqtt_test.py` 실행
3. 모든 센서 데이터 출력 확인

**예상 출력:**
```
14:23:45.123 | [조이스틱] Accel: (0.52, -0.31, 9.78) | Gyro: (0.01, -0.02, 0.00)
14:23:45.456 | [Watch] 심박수: 85 bpm
14:23:45.789 | [조이스틱] Accel: (0.50, -0.33, 9.80) | Gyro: (0.00, -0.01, 0.01)
14:23:46.012 | [Watch] 심박수: 86 bpm
```

### 시나리오 4: 명령 전송 테스트

1. 디바이스 연결
2. `python mqtt_test.py` 실행
3. 명령 `1` 입력 → start 명령 전송 확인
4. 명령 `2` 입력 → stop 명령 전송 확인
5. ESP32에서 MQTT 메시지 수신 확인

### 시나리오 5: 디버깅 모드 (Raw JSON 출력)

**mqtt_config.py 수정:**
```python
SHOW_RAW_JSON = True  # Raw JSON 출력 활성화
```

**예상 출력:**
```
[MQTT RX] joystick/sensor/data
{"accel_x": 0.52, "accel_y": -0.31, "accel_z": 9.78, "gyro_x": 0.01, "gyro_y": -0.02, "gyro_z": 0.00, "timestamp": 1234567890}

14:23:45.123 | [조이스틱] Accel: (0.52, -0.31, 9.78) | Gyro: (0.01, -0.02, 0.00)
```

## 🛠️ 문제 해결

### MQTT 브로커 연결 실패

**증상:**
```
✗ MQTT 브로커 연결 실패
```

**해결 방법:**
1. MQTT 브로커 실행 확인
   ```bash
   sudo systemctl status mosquitto
   ```
   실행 중이 아니면:
   ```bash
   sudo systemctl start mosquitto
   ```

2. 포트 확인
   ```bash
   sudo netstat -tulpn | grep 1883
   ```
   포트가 열려있지 않으면 방화벽 설정 확인

3. 설정 확인
   - `mqtt_config.py`의 `MQTT_BROKER_HOST` 확인
   - localhost 또는 올바른 IP 주소인지 확인

### 센서 데이터가 수신되지 않음

**증상:**
```
[실시간 센서 데이터]
------------------------------------------------------------
(아무 데이터도 출력되지 않음)
```

**해결 방법:**
1. ESP32 디바이스가 실행 중인지 확인
2. ESP32가 올바른 MQTT 브로커에 연결되었는지 확인
3. ESP32가 올바른 토픽으로 데이터를 전송하는지 확인
4. `mqtt_monitor.py`로 모든 토픽 확인:
   ```bash
   python mqtt_monitor.py  # 프로젝트 루트에 있는 모니터
   ```

### 권한 오류

**증상:**
```
Permission denied: '/dev/ttyUSB0'
```

**해결 방법:**
이 프로그램은 UART를 사용하지 않으므로 이 오류는 발생하지 않아야 합니다.
만약 발생한다면 다른 프로그램의 문제일 수 있습니다.

## 📁 파일 구조

```
WatchTower_test/
├── mqtt_test.py              # 메인 실행 파일 (CLI 인터페이스)
├── mqtt_controller.py        # MQTT 컨트롤러 (간소화 버전)
├── mqtt_config.py           # MQTT 설정 파일
└── README.md                # 이 파일
```

## 🔗 관련 파일

- **원본 WatchTower 코드:** `../WatchTower/`
- **MQTT 모니터:** `../mqtt_monitor.py`
- **ESP32 조이스틱 코드:** `../mpu6050_mqtt/`
- **프로젝트 문서:** `../README.md`, `../CLAUDE.md`

## 💡 팁

### 1. 동시에 여러 터미널 사용

**터미널 1: MQTT 테스트**
```bash
cd WatchTower_test
python mqtt_test.py
```

**터미널 2: MQTT 모니터 (모든 토픽 확인)**
```bash
python mqtt_monitor.py
```

**터미널 3: ESP32 로그**
```bash
cd mpu6050_mqtt
idf.py monitor
```

### 2. 실시간 통계 자동 출력

`mqtt_config.py`에서 설정:
```python
STATS_UPDATE_INTERVAL = 10.0  # 10초마다 자동 통계 출력
```

### 3. 타임스탬프 비활성화

데이터가 너무 많아서 화면이 복잡하다면:
```python
SHOW_TIMESTAMP = False  # 타임스탬프 제거
```

### 4. 소수점 자릿수 조정

센서 데이터 정밀도 조정:
```python
SENSOR_DATA_PRECISION = 3  # 소수점 3자리까지 표시
```

## 📝 개발 참고사항

### mqtt_controller.py 단독 테스트

컨트롤러만 테스트하려면:
```bash
python mqtt_controller.py
```

이 모드에서는 콜백 함수만 등록되어 센서 데이터만 출력됩니다.

### 커스터마이징

**새로운 토픽 추가:**
1. `mqtt_config.py`에 토픽 추가
2. `mqtt_controller.py`의 `_on_connect()`에서 구독 추가
3. `mqtt_controller.py`의 `_on_message()`에서 처리 로직 추가
4. `mqtt_test.py`에서 콜백 등록

**새로운 명령 추가:**
1. `mqtt_controller.py`에 명령 전송 함수 추가
2. `mqtt_test.py`의 `handle_command_input()`에서 명령 처리 추가

## ⚙️ 설정 옵션 전체 목록

`mqtt_config.py`:

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `MQTT_BROKER_HOST` | `'localhost'` | MQTT 브로커 주소 |
| `MQTT_BROKER_PORT` | `1883` | MQTT 브로커 포트 |
| `MQTT_KEEPALIVE` | `60` | Keep-alive 시간 (초) |
| `MQTT_QOS` | `1` | QoS 레벨 (0, 1, 2) |
| `MQTT_USERNAME` | `""` | 브로커 인증 사용자명 |
| `MQTT_PASSWORD` | `""` | 브로커 인증 비밀번호 |
| `SHOW_TIMESTAMP` | `True` | 타임스탬프 출력 여부 |
| `SHOW_RAW_JSON` | `False` | Raw JSON 출력 여부 |
| `SENSOR_DATA_PRECISION` | `2` | 센서 데이터 소수점 자릿수 |
| `ENABLE_STATISTICS` | `True` | 통계 수집 활성화 |
| `STATS_UPDATE_INTERVAL` | `10.0` | 통계 자동 출력 주기 (초) |

## 📞 지원

문제가 발생하거나 질문이 있으면:
1. 이 README의 "문제 해결" 섹션 확인
2. 프로젝트 루트의 `CLAUDE.md` 참고
3. MQTT 브로커 로그 확인: `sudo journalctl -u mosquitto -f`

## 📜 라이센스

이 프로젝트는 New_Workout_Plan 프로젝트의 일부입니다.
