# ESP32 센서 데이터 MQTT 전송 시스템

ESP32에서 센서 데이터를 읽어 MQTT를 통해 Jetson으로 전송하는 시스템입니다.

## 프로젝트 구조

```
9_mqtt/main/
├── config.h              # 모든 설정 (Wi-Fi, MQTT, 토픽, 주기)
├── wifi_handler.h/c      # Wi-Fi 연결 관리
├── mqtt_handler.h/c      # MQTT 통신 관리
├── sensor_task.h/c       # 센서 읽기 및 전송
├── app_main.c            # 메인 파일
└── CMakeLists.txt        # 빌드 설정
```

## 주요 기능

- ✅ 자동 Wi-Fi 연결 및 재연결
- ✅ MQTT 브로커 자동 연결
- ✅ 주기적 센서 데이터 발행 (기본 5초)
- ✅ MQTT 명령으로 전송 주기 동적 변경
- ✅ JSON 형식 데이터 전송
- ✅ 양방향 통신 (ESP32 ↔ Jetson)

---

## 1단계: 설정 변경

### config.h 파일 수정

```c
// Wi-Fi 설정
#define WIFI_SSID "YourWiFiName"           // 본인 Wi-Fi 이름
#define WIFI_PASSWORD "YourWiFiPassword"   // 본인 Wi-Fi 비밀번호

// MQTT 브로커 설정 (Jetson IP)
#define MQTT_BROKER_URL "mqtt://192.168.x.x:1883"  // Jetson IP 주소

// 센서 데이터 전송 주기
#define DEFAULT_PUBLISH_INTERVAL_MS 5000  // 5초 (원하는 값으로 변경 가능)
```

---

## 2단계: Jetson에 MQTT 브로커 설치

### Mosquitto 설치
```bash
# 패키지 업데이트
sudo apt update

# Mosquitto 브로커 및 클라이언트 도구 설치
sudo apt install mosquitto mosquitto-clients -y

# Mosquitto 서비스 시작
sudo systemctl start mosquitto

# 부팅 시 자동 시작 설정
sudo systemctl enable mosquitto

# 실행 상태 확인
sudo systemctl status mosquitto
```

### Jetson IP 주소 확인
```bash
hostname -I
# 예시 출력: 192.168.0.100
```

이 IP를 config.h의 `MQTT_BROKER_URL`에 설정하세요.

---

## 3단계: ESP32 빌드 및 플래시

```bash
cd /home/ubuntu07/workingspace/esp-idf/9_mqtt

# 빌드
idf.py build

# 플래시 및 모니터
idf.py flash monitor
```

---

## 4단계: Jetson에서 MQTT 테스트

### 터미널 1: 센서 데이터 모니터링
```bash
mosquitto_sub -h localhost -t "esp32/sensor/data" -v
```

**출력 예시:**
```
esp32/sensor/data {"sensor":"temperature","value":25.43,"unit":"C","timestamp":12345}
esp32/sensor/data {"sensor":"temperature","value":26.12,"unit":"C","timestamp":12350}
```

### 터미널 2: ESP32에 명령 보내기

**전송 주기 변경:**
```bash
# 1초로 변경
mosquitto_pub -h localhost -t "esp32/command" -m "INTERVAL:1000"

# 2초로 변경
mosquitto_pub -h localhost -t "esp32/command" -m "INTERVAL:2000"

# 10초로 변경
mosquitto_pub -h localhost -t "esp32/command" -m "INTERVAL:10000"
```

### 터미널 3: ESP32 응답 확인
```bash
mosquitto_sub -h localhost -t "esp32/response" -v
```

**출력 예시:**
```
esp32/response {"status":"ok","interval":2000}
```

### 모든 MQTT 메시지 모니터링 (디버깅용)
```bash
mosquitto_sub -h localhost -t "#" -v
```
- `#`: 모든 토픽 구독 (와일드카드)
- ESP32와 Jetson 간 모든 통신을 볼 수 있음

---

## MQTT 토픽 구조

| 토픽 | 방향 | 설명 | 데이터 형식 |
|------|------|------|-------------|
| `esp32/sensor/data` | ESP32 → Jetson | 센서 데이터 발행 | JSON |
| `esp32/command` | Jetson → ESP32 | 명령 전송 | 문자열 |
| `esp32/response` | ESP32 → Jetson | 명령 응답 | JSON |

### 데이터 형식

**센서 데이터 (esp32/sensor/data):**
```json
{
  "sensor": "temperature",
  "value": 25.43,
  "unit": "C",
  "timestamp": 12345
}
```

**명령 (esp32/command):**
```
INTERVAL:3000
```

**응답 (esp32/response):**
```json
{
  "status": "ok",
  "interval": 3000
}
```

---

## 센서 연동 방법

### sensor_task.c 파일 수정

현재는 더미 데이터를 생성합니다:
```c
bool sensor_read_data(float *sensor_value)
{
    // 현재는 더미 데이터 (0~100 사이 랜덤 값)
    *sensor_value = (float)(esp_random() % 10000) / 100.0f;
    return true;
}
```

**실제 센서로 변경 예시 (DHT11 온도 센서):**
```c
bool sensor_read_data(float *sensor_value)
{
    // DHT11 센서 읽기
    float temperature = dht_read_temperature();

    if (temperature == -1) {
        return false;  // 읽기 실패
    }

    *sensor_value = temperature;
    return true;
}
```

---

## 시스템 동작 흐름

```
ESP32 부팅
  ↓
Wi-Fi 연결 (자동)
  ↓
MQTT 브로커 연결 (자동)
  ↓
"esp32/command" 토픽 구독 (자동)
  ↓
센서 태스크 시작
  ↓
주기적으로 반복:
  1. 센서 데이터 읽기
  2. JSON 생성
  3. MQTT로 발행
  4. 설정된 주기만큼 대기
```

---

## 전송 주기 변경 방법

### 방법 1: MQTT 명령으로 변경 (실시간, 추천)
```bash
mosquitto_pub -h localhost -t "esp32/command" -m "INTERVAL:2000"
```

### 방법 2: 코드에서 기본값 변경
config.h:
```c
#define DEFAULT_PUBLISH_INTERVAL_MS 3000  // 3초로 변경
```

### 방법 3: 코드에서 직접 호출
```c
void app_main(void)
{
    // 초기화...

    // 전송 주기를 2초로 변경
    sensor_set_publish_interval(2000);
}
```

---

## 문제 해결

### Wi-Fi 연결 실패
1. config.h에서 SSID와 비밀번호 확인
2. ESP32와 Jetson이 같은 Wi-Fi 네트워크에 있는지 확인
3. Wi-Fi 신호 강도 확인

### MQTT 연결 실패
1. Jetson에서 Mosquitto가 실행 중인지 확인:
   ```bash
   sudo systemctl status mosquitto
   ```
2. 방화벽 확인:
   ```bash
   sudo ufw allow 1883
   ```
3. Jetson IP 주소가 정확한지 확인

### 센서 데이터가 전송되지 않음
1. ESP32 시리얼 모니터에서 로그 확인:
   ```bash
   idf.py monitor
   ```
2. MQTT 연결 상태 확인
3. 전송 주기가 너무 길게 설정되지 않았는지 확인

---

## WiFi 핸들러 상세 설명

### 📱 WiFi 연결 완전 시나리오

#### 🎬 시나리오 1: 성공적인 연결 (정상 흐름)

```
┌─────────────────────────────────────────────────────────────┐
│ app_main()에서 wifi_init_and_connect() 호출                  │
└─────────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [65줄] s_wifi_event_group = xEventGroupCreate()             │
│ → Event Group 생성 (비트 상태: 00000000)                     │
└─────────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [68-76줄] 네트워크 스택 초기화                               │
│  - esp_netif_init(): TCP/IP 스택 준비                       │
│  - esp_event_loop_create_default(): 이벤트 시스템 생성      │
│  - esp_netif_create_default_wifi_sta(): WiFi 인터페이스 생성│
└─────────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [79-81줄] WiFi 드라이버 초기화                               │
│  - WIFI_INIT_CONFIG_DEFAULT(): 기본 설정 로드               │
│  - esp_wifi_init(&cfg): WiFi 드라이버 시작                  │
└─────────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [89-94줄] 이벤트 핸들러 등록 (콜백 함수 연결)                │
│  - WIFI_EVENT + ESP_EVENT_ANY_ID → wifi_event_handler       │
│  - IP_EVENT + IP_EVENT_STA_GOT_IP → wifi_event_handler      │
│  → "WiFi 상태 바뀌면 wifi_event_handler 호출해줘!"          │
└─────────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [96-105줄] WiFi 설정                                         │
│  - wifi_config.sta.ssid = "YourWiFiName"                    │
│  - wifi_config.sta.password = "YourPassword"                │
│  - authmode = WIFI_AUTH_WPA_WPA2_PSK                        │
└─────────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [107-109줄] WiFi 모드 설정 및 시작                           │
│  - esp_wifi_set_mode(WIFI_MODE_STA): 스테이션 모드          │
│  - esp_wifi_set_config(): 설정 적용                         │
│  - esp_wifi_start(): WiFi 드라이버 시작! ⚡                 │
└─────────────────────────────────────────────────────────────┘
                         ↓
         🔔 **WIFI_EVENT_STA_START 이벤트 발생!** 🔔
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [26-29줄] wifi_event_handler 자동 호출                       │
│  if (event_id == WIFI_EVENT_STA_START) {                    │
│      esp_wifi_connect();  // 공유기에 연결 시도!            │
│  }                                                           │
└─────────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [122-123줄] xEventGroupWaitBits() - 대기 시작! ⏸️            │
│  → "WIFI_CONNECTED_BIT 또는 WIFI_FAIL_BIT 설정될 때까지    │
│     여기서 멈춰!" (blocking)                                 │
│  → main task는 여기서 정지 상태                              │
└─────────────────────────────────────────────────────────────┘
                         ↓
        ┌─────────────────────────────────┐
        │  백그라운드에서 WiFi 연결 중... │
        │  (AP와 핸드셰이크 진행)         │
        └─────────────────────────────────┘
                         ↓
              💡 연결 성공! IP 주소 받음 💡
                         ↓
         🔔 **IP_EVENT_STA_GOT_IP 이벤트 발생!** 🔔
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [45-51줄] wifi_event_handler 자동 호출                       │
│  - event_data에서 IP 정보 추출                              │
│  - ESP_LOGI("Got IP: 192.168.0.100")  // 로그 출력          │
│  - s_retry_num = 0;  // 재시도 카운터 리셋                  │
│  - xEventGroupSetBits(..., WIFI_CONNECTED_BIT);             │
│    → Event Group 비트: 00000001 설정! ✅                     │
└─────────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [122-123줄] xEventGroupWaitBits() 깨어남! ⏯️                 │
│  → bits = 00000001 (WIFI_CONNECTED_BIT)                     │
└─────────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│ [125-129줄] 비트 확인                                        │
│  if (bits & WIFI_CONNECTED_BIT) {                           │
│      ESP_LOGI("Wi-Fi connected successfully");              │
│      return true;  ✅ 성공!                                  │
│  }                                                           │
└─────────────────────────────────────────────────────────────┘
                         ↓
                  ✅ **연결 완료!**
```

---

#### 🚨 시나리오 2: 연결 실패 (재시도 후 포기)

```
[109줄] esp_wifi_start() → WiFi 드라이버 시작
         ↓
[28줄] esp_wifi_connect() 호출 → 연결 시도
         ↓
    ❌ 연결 실패! (비밀번호 틀림, 공유기 꺼짐 등)
         ↓
🔔 **WIFI_EVENT_STA_DISCONNECTED 이벤트 발생!** 🔔
         ↓
┌─────────────────────────────────────────────────────────────┐
│ [30-43줄] wifi_event_handler 호출                            │
│  - s_retry_num = 0 < WIFI_MAX_RETRY (5)?                    │
│  - YES → esp_wifi_connect() 재시도 1회                      │
│  - s_retry_num = 1                                          │
│  - ESP_LOGI("Retry to connect... (attempt 1/5)")            │
└─────────────────────────────────────────────────────────────┘
         ↓
    ❌ 또 실패! → WIFI_EVENT_STA_DISCONNECTED
         ↓
┌─────────────────────────────────────────────────────────────┐
│  - s_retry_num = 1 < 5? YES → 재시도 2회                    │
│  - s_retry_num = 2                                          │
│  - ESP_LOGI("Retry to connect... (attempt 2/5)")            │
└─────────────────────────────────────────────────────────────┘
         ↓
    ... (3회, 4회, 5회 반복) ...
         ↓
    ❌ 5번째도 실패! → WIFI_EVENT_STA_DISCONNECTED
         ↓
┌─────────────────────────────────────────────────────────────┐
│ [30-43줄] wifi_event_handler 호출                            │
│  - s_retry_num = 5 < 5? NO! ❌                              │
│  - else 블록 진입                                            │
│  - xEventGroupSetBits(..., WIFI_FAIL_BIT);                  │
│    → Event Group 비트: 00000010 설정! ⛔                     │
│  - ESP_LOGE("Failed to connect to Wi-Fi")                   │
└─────────────────────────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────────────────────────┐
│ [122-123줄] xEventGroupWaitBits() 깨어남!                    │
│  → bits = 00000010 (WIFI_FAIL_BIT)                          │
└─────────────────────────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────────────────────────┐
│ [130-134줄] 비트 확인                                        │
│  if (bits & WIFI_FAIL_BIT) {                                │
│      ESP_LOGE("Wi-Fi connection failed");                   │
│      return false;  ❌ 실패!                                 │
│  }                                                           │
└─────────────────────────────────────────────────────────────┘
         ↓
       ❌ **연결 실패 반환**
```

---

#### 🔄 시나리오 3: 재시도 중 성공

```
1회 실패 → s_retry_num = 1
         ↓
2회 실패 → s_retry_num = 2
         ↓
3회 시도 → 💡 성공! IP 받음
         ↓
🔔 IP_EVENT_STA_GOT_IP 발생 🔔
         ↓
┌─────────────────────────────────────────────────────────────┐
│ [45-51줄] wifi_event_handler 호출                            │
│  - ESP_LOGI("Got IP: 192.168.0.100")                        │
│  - s_retry_num = 0;  ← 재시도 카운터 초기화!                │
│  - xEventGroupSetBits(..., WIFI_CONNECTED_BIT);             │
└─────────────────────────────────────────────────────────────┘
         ↓
       ✅ 성공 처리 (시나리오 1과 동일)
```

---

#### 🧵 멀티태스킹 관점에서 보기

```
[Main Task]                           [WiFi Task (백그라운드)]
    |                                         |
wifi_init_and_connect() 호출               |
    |                                         |
esp_wifi_start() ────────────────────────→ WiFi 드라이버 시작
    |                                         |
    |                              esp_wifi_connect() 실행
    |                                         |
    |                                    AP와 핸드셰이크...
xEventGroupWaitBits()                        |
    | (BLOCKED) ⏸️                            |
    | 대기 중...                              |
    | ...                                     |
    | ...                              연결 시도 중...
    | ...                                     |
    |                                    IP 주소 받음 💡
    |                                         |
    | ←───── IP_EVENT_STA_GOT_IP ──────────  |
    |                                         |
    |                              wifi_event_handler 실행
    |                                         |
    | ←─── xEventGroupSetBits() ────────  WIFI_CONNECTED_BIT 설정
    |                                         |
깨어남! ⏯️                                   |
    |                                         |
bits 확인                                    |
    |                                         |
return true ✅                               |
```

---

### 💡 핵심 포인트

#### 1. **두 개의 실행 컨텍스트**
- **Main Task**: `wifi_init_and_connect()` 실행
- **Event Handler**: `wifi_event_handler()` 자동 실행 (이벤트 발생 시)

#### 2. **Event Group의 역할**
- **Bridge** 역할: 두 태스크 간 통신 수단
- Main Task: "성공/실패 알려줄 때까지 기다릴게" (대기)
- Event Handler: "연결됐어!" 또는 "실패했어!" (신호)

#### 3. **Blocking vs Non-blocking**
- `xEventGroupWaitBits()`는 **blocking** 함수
- 비트가 설정될 때까지 Main Task는 **정지 상태**
- 다른 태스크는 계속 실행 가능 (FreeRTOS 멀티태스킹)

#### 4. **재시도 로직**
- 최대 5회 재시도 (`WIFI_MAX_RETRY`)
- 재시도마다 `WIFI_EVENT_STA_DISCONNECTED` 발생
- 5회 초과 시 `WIFI_FAIL_BIT` 설정

#### 5. **이벤트 순서**
```
WIFI_EVENT_STA_START (시작)
    → esp_wifi_connect() 호출
    → 연결 시도...
    → IP_EVENT_STA_GOT_IP (성공) 또는
    → WIFI_EVENT_STA_DISCONNECTED (실패)
```

---

## 참고 자료

- [ESP-IDF 공식 문서](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [MQTT 프로토콜](https://mqtt.org/)
- [Eclipse Mosquitto](https://mosquitto.org/)

---

## 라이선스

이 프로젝트는 퍼블릭 도메인입니다.
