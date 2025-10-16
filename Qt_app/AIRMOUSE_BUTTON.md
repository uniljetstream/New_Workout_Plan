# Qt 앱 에어마우스 버튼 입력 가이드

## 📋 개요

Qt 앱의 에어마우스 기능에 **버튼 1개 (왼쪽 클릭)** 지원을 추가했습니다.

## 🎮 버튼 동작

### 버튼 1: 왼쪽 클릭 (Left Click)
- **동작**: 커서 위치의 위젯 클릭
- **사용처**:
  - 메인 메뉴에서 버튼 선택
  - Exercise Selection에서 운동 선택
  - Settings에서 설정 변경
  - Workout에서 시작/중지 버튼 클릭

## 📡 MQTT 메시지 형식

### ESP32 → Qt 앱

**토픽**: `joystick/sensor/data`

**메시지 형식**:
```json
{
  "mode": "airmouse",
  "mouse_x": 10.5,           // X축 이동량 (픽셀)
  "mouse_y": -5.2,           // Y축 이동량 (픽셀)
  "scroll_delta": 0,         // 스크롤 (-1: 아래, 0: 없음, 1: 위)
  "button_pressed": true,    // ⭐ 버튼 눌림 상태 (true/false)
  "timestamp": 1234567890
}
```

### 필드 설명

| 필드 | 타입 | 설명 | 필수 |
|------|------|------|------|
| `mode` | string | "airmouse" 고정 | ✅ |
| `mouse_x` | float | X축 이동량 (픽셀) | ✅ |
| `mouse_y` | float | Y축 이동량 (픽셀) | ✅ |
| `scroll_delta` | int | 스크롤 방향 (-1, 0, 1) | ❌ |
| `button_pressed` | bool | 버튼 눌림 상태 | ⭐ **새로 추가** |
| `timestamp` | int | Unix timestamp | ❌ |

## 🔧 구현 세부사항

### Qt 코드 변경사항

**파일**: `Qt_app/mainwindow.cpp`

**메서드**: `updateAirMouseData()`

```cpp
void MainWindow::updateAirMouseData(const QJsonObject &data)
{
    if (m_airMouseManager && m_airMouseManager->isEnabled()) {
        m_airMouseManager->handleMouseData(data);

        // 버튼 입력 처리 (왼쪽 클릭만)
        if (data.contains("button_pressed")) {
            bool buttonPressed = data["button_pressed"].toBool();

            // 버튼이 눌렸을 때만 클릭 시뮬레이션
            if (buttonPressed) {
                m_airMouseManager->simulateClick();
                qDebug() << "Button pressed - simulating click";
            }
        }
    }
}
```

### 동작 순서

1. ESP32가 MQTT로 `button_pressed: true` 전송
2. Qt 앱이 MQTT 메시지 수신
3. `updateAirMouseData()` 메서드가 버튼 상태 확인
4. `AirMouseManager::simulateClick()` 호출
5. 커서 위치의 위젯에 `QMouseEvent` 전송
6. 클릭 애니메이션 표시

## 🎯 사용 예시

### 1. 메뉴 버튼 클릭
```
1. 조이스틱을 움직여 "운동 선택" 버튼 위로 커서 이동
2. 버튼을 눌러 클릭
3. Exercise Selection 페이지로 이동
```

### 2. 운동 선택
```
1. "스쿼트" 버튼 위로 커서 이동
2. 버튼을 눌러 선택
3. Workout 페이지로 이동
```

### 3. 운동 시작/중지
```
1. "시작" 버튼 위로 커서 이동
2. 버튼을 눌러 운동 시작
3. "중지" 버튼 위로 커서 이동
4. 버튼을 눌러 운동 중지
```

## ⚙️ ESP32 구현 가이드

### 필요한 하드웨어
- **버튼 1개**: GPIO 0 (BOOT 버튼) 또는 다른 GPIO 핀

### 구현 예시 (의사코드)

```c
// GPIO 초기화
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << BUTTON_GPIO),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};
gpio_config(&io_conf);

// 버튼 상태 읽기 및 MQTT 전송
bool button_pressed = !gpio_get_level(BUTTON_GPIO); // Active LOW

// JSON 생성
cJSON *root = cJSON_CreateObject();
cJSON_AddStringToObject(root, "mode", "airmouse");
cJSON_AddNumberToObject(root, "mouse_x", mouse_x);
cJSON_AddNumberToObject(root, "mouse_y", mouse_y);
cJSON_AddNumberToObject(root, "scroll_delta", scroll_delta);
cJSON_AddBoolToObject(root, "button_pressed", button_pressed);  // ⭐ 추가
cJSON_AddNumberToObject(root, "timestamp", timestamp);

// MQTT 발행
char *json_string = cJSON_Print(root);
esp_mqtt_client_publish(mqtt_client, "joystick/sensor/data", json_string, 0, 1, 0);
```

## 🧪 테스트 방법

### MQTT 테스트 메시지

```bash
# 버튼 누름
mosquitto_pub -h 10.10.16.111 -t "joystick/sensor/data" -m '{
  "mode": "airmouse",
  "mouse_x": 0,
  "mouse_y": 0,
  "scroll_delta": 0,
  "button_pressed": true,
  "timestamp": 1234567890
}'

# 버튼 떼기
mosquitto_pub -h 10.10.16.111 -t "joystick/sensor/data" -m '{
  "mode": "airmouse",
  "mouse_x": 0,
  "mouse_y": 0,
  "scroll_delta": 0,
  "button_pressed": false,
  "timestamp": 1234567890
}'
```

### 디버그 로그

Qt 앱 실행 시 버튼 클릭이 감지되면 다음 로그가 출력됩니다:

```
Button pressed - simulating click
Click simulated on "<button_name>"
```

## 📌 주의사항

1. **버튼 바운싱**: ESP32에서 디바운싱 처리 필요
2. **연속 클릭 방지**: 이전 상태를 저장하여 `true` → `false` 전환 시에만 클릭
3. **MQTT QoS**: 버튼 입력은 QoS 1 권장 (최소 1회 전달 보장)
4. **타이밍**: 버튼을 너무 빨리 누르면 클릭이 누락될 수 있음

## 🔄 향후 확장 가능성

### 더블 클릭 (2개 버튼 사용 시)
```json
{
  "button_pressed": true,
  "button2_pressed": false
}
```

### 드래그 앤 드롭 (버튼 길게 누름)
```json
{
  "button_pressed": true,
  "button_hold_time": 1500  // ms
}
```

## 📚 관련 파일

- `Qt_app/mainwindow.cpp` - 버튼 입력 처리
- `Qt_app/airmouse_manager.cpp` - 클릭 시뮬레이션
- `Qt_app/cursor_overlay.cpp` - 클릭 애니메이션

---

**업데이트 날짜**: 2025-10-14
**버전**: 1.0
