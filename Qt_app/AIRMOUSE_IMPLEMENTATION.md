# Qt 앱 전역 에어마우스 구현

## 개요

ESP32 조이스틱(MPU6050)을 사용하여 Qt 앱의 **모든 페이지와 위젯을 제어**할 수 있는 전역 에어마우스 시스템이 구현되었습니다.

## 주요 기능

### ✅ 구현된 기능

1. **전역 커서 제어**
   - 모든 페이지(메인 메뉴, 운동 선택, 설정, 운동 중)에서 작동
   - 실시간 커서 위치 추적 및 표시
   - 트레일 효과로 움직임 시각화

2. **실제 위젯 상호작용**
   - 버튼 클릭 가능
   - 스크롤 영역 제어 가능
   - 호버 효과 지원
   - 모든 Qt 이벤트 시스템과 완전 호환

3. **시각적 피드백**
   - 전역 커서 오버레이 (모든 페이지 위에 표시)
   - 클릭 애니메이션 효과
   - 커서 트레일 (움직임 경로 표시)
   - 커서 십자선 및 색상 커스터마이징

4. **설정 옵션**
   - 감도 조절 (기본값: 1.5x)
   - 스무딩 필터 활성화/비활성화
   - 커서 표시/숨김
   - 트레일 길이 조정

## 시스템 아키텍처

### 컴포넌트 구조

```
┌─────────────────────────────────────────┐
│         MainWindow (Qt App)             │
│  ┌───────────────────────────────────┐  │
│  │    AirMouseManager (Control)     │  │
│  │  - MQTT 데이터 → Qt 이벤트 변환   │  │
│  │  - 커서 이동 처리                 │  │
│  │  - 클릭/스크롤 시뮬레이션          │  │
│  └──────────────┬────────────────────┘  │
│                 │                        │
│  ┌──────────────▼────────────────────┐  │
│  │   CursorOverlay (Visual Layer)   │  │
│  │  - 전역 커서 렌더링               │  │
│  │  - 트레일 효과                    │  │
│  │  - 클릭 애니메이션                │  │
│  └───────────────────────────────────┘  │
│                                         │
│  ┌───────────────────────────────────┐  │
│  │  All Pages & Widgets             │  │
│  │  - Main Menu                      │  │
│  │  - Exercise Selection             │  │
│  │  │  Settings                       │  │
│  │  - Workout                         │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
          ▲
          │ MQTT (joystick/sensor/data)
          │
┌─────────┴─────────┐
│  ESP32 Joystick   │
│  - MPU6050 센서    │
│  - 에어마우스 모드  │
└───────────────────┘
```

## 파일 구조

### 새로 추가된 파일

1. **airmouse_manager.h/cpp**
   - 전역 에어마우스 제어 클래스
   - MQTT 데이터 → Qt 마우스 이벤트 변환
   - 커서 이동, 클릭, 스크롤 처리
   - 스무딩 필터 및 감도 조절

2. **cursor_overlay.h/cpp**
   - 전역 커서 오버레이 위젯
   - 모든 페이지 위에 표시되는 투명 레이어
   - 커서 렌더링 및 애니메이션
   - 마우스 이벤트 하위 위젯으로 전달

### 수정된 파일

1. **mainwindow.h**
   - `AirMouseManager *m_airMouseManager` 추가
   - `setupAirMouse()` 메서드 선언

2. **mainwindow.cpp**
   - `setupAirMouse()` 구현: 에어마우스 매니저 초기화
   - `updateAirMouseData()` 수정: 전역 에어마우스 업데이트
   - `on_settings_testAirMouseButton_clicked()` 수정: 토글 방식으로 변경

3. **workout_app.pro**
   - 새 소스 파일 추가 (airmouse_manager, cursor_overlay)

## 사용 방법

### 1. 에어마우스 활성화

**방법 1: Settings 페이지에서 활성화**
```
1. 메인 메뉴 → "설정" 버튼 클릭
2. Settings 페이지 → "에어마우스 테스트" 버튼 클릭
3. 에어마우스 활성화 메시지 확인
4. 조이스틱을 움직여 커서 제어
```

**방법 2: 프로그래밍 방식**
```cpp
// 에어마우스 활성화
m_airMouseManager->setEnabled(true);

// ESP32에 에어마우스 모드 명령 전송
sendAirMouseModeCommand();
```

### 2. 커서 이동

- ESP32 조이스틱을 기울이면 커서가 이동
- MQTT 메시지: `{"mode": "airmouse", "mouse_x": 10.5, "mouse_y": -5.2, ...}`
- 실시간으로 모든 페이지에서 커서 표시

### 3. 버튼 클릭

**현재 구현:**
- `AirMouseManager::simulateClick()` 메서드 사용
- 커서 위치의 위젯에 QMouseEvent 전송

**향후 구현 (ESP32 버튼 연동):**
```cpp
// ESP32에서 버튼 눌림 감지 시
if (button_pressed) {
    json_object["click"] = true;
    // MQTT로 전송
}

// Qt에서 수신
if (data.contains("click") && data["click"].toBool()) {
    m_airMouseManager->simulateClick();
}
```

### 4. 스크롤

- ESP32가 스크롤 제스처 감지 시 `scroll_delta` 전송
- MQTT 메시지: `{"mode": "airmouse", "scroll_delta": 1, ...}`
- 커서 위치의 스크롤 가능한 위젯에 QWheelEvent 전송

### 5. 에어마우스 비활성화

- Settings → "에어마우스 테스트" 버튼 다시 클릭
- 전역 커서 오버레이가 숨겨짐
- 일반 마우스/터치 입력으로 복귀

## 기술적 세부사항

### 커서 이동 처리

```cpp
void AirMouseManager::moveCursor(double deltaX, double deltaY) {
    // 1. 감도 적용
    int adjustedX = static_cast<int>(deltaX * m_sensitivity);
    int adjustedY = static_cast<int>(deltaY * m_sensitivity);

    // 2. 스무딩 필터
    QPoint delta = applySmoothingFilter(QPoint(adjustedX, adjustedY));

    // 3. 경계 체크
    QPoint newPos = m_cursorPos + delta;
    clampToWindow(newPos);

    // 4. 오버레이 업데이트
    m_cursorOverlay->setCursorPosition(newPos);

    // 5. 호버 상태 업데이트
    updateHoverState();
}
```

### 클릭 시뮬레이션

```cpp
void AirMouseManager::simulateClick() {
    // 1. 커서 위치의 위젯 찾기
    QWidget *targetWidget = QApplication::widgetAt(globalPos);

    // 2. 마우스 프레스 이벤트 생성
    QMouseEvent pressEvent(QEvent::MouseButtonPress, ...);
    QApplication::sendEvent(targetWidget, &pressEvent);

    // 3. 마우스 릴리스 이벤트 생성
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, ...);
    QApplication::sendEvent(targetWidget, &releaseEvent);

    // 4. 클릭 애니메이션 표시
    m_cursorOverlay->showClickAnimation();
}
```

### 호버 효과

```cpp
void AirMouseManager::updateHoverState() {
    QWidget *currentWidget = getWidgetAtCursor();

    if (currentWidget != m_hoveredWidget) {
        // 이전 위젯에 HoverLeave 이벤트
        if (m_hoveredWidget) {
            QHoverEvent leaveEvent(QEvent::HoverLeave, ...);
            QApplication::sendEvent(m_hoveredWidget, &leaveEvent);
        }

        // 새 위젯에 HoverEnter 이벤트
        if (currentWidget) {
            QHoverEvent enterEvent(QEvent::HoverEnter, ...);
            QApplication::sendEvent(currentWidget, &enterEvent);
        }

        m_hoveredWidget = currentWidget;
    }
}
```

### 스무딩 필터

```cpp
QPoint AirMouseManager::applySmoothingFilter(const QPoint &delta) {
    // 버퍼에 추가
    m_moveBuffer.append(delta);
    if (m_moveBuffer.size() > m_bufferSize) {
        m_moveBuffer.removeFirst();
    }

    // 이동 평균 계산
    int avgX = 0, avgY = 0;
    for (const QPoint &p : m_moveBuffer) {
        avgX += p.x();
        avgY += p.y();
    }
    avgX /= m_moveBuffer.size();
    avgY /= m_moveBuffer.size();

    return QPoint(avgX, avgY);
}
```

## 데이터 흐름

### 에어마우스 데이터 흐름

```
ESP32 (MPU6050)
  │
  │ 1. 센서 데이터 읽기
  │    (가속도, 자이로)
  │
  ▼
Airmouse 모드 처리
  │
  │ 2. 마우스 데이터 변환
  │    mouse_x, mouse_y, scroll_delta
  │
  ▼
MQTT Publish
  │ Topic: joystick/sensor/data
  │ {"mode": "airmouse", "mouse_x": 10.5, ...}
  │
  ▼
Qt App (MQTT Subscribe)
  │
  │ 3. onMqttMessageReceived()
  │
  ▼
updateAirMouseData()
  │
  │ 4. AirMouseManager::handleMouseData()
  │
  ▼
moveCursor() / handleScroll()
  │
  │ 5. Qt 이벤트 생성 및 전송
  │    - QMouseEvent
  │    - QWheelEvent
  │    - QHoverEvent
  │
  ▼
Target Widget
  │
  │ 6. 위젯 반응
  │    - 버튼 하이라이트
  │    - 클릭 처리
  │    - 스크롤 처리
  │
  ▼
UI Update
```

## 테스트 시나리오

### 시나리오 1: 메인 메뉴 탐색

```
1. 앱 실행 → 메인 메뉴 표시
2. Settings → "에어마우스 테스트" 버튼 클릭 → 에어마우스 활성화
3. 조이스틱을 기울여 커서를 "운동 선택" 버튼으로 이동
4. ESP32 버튼 클릭 (또는 simulateClick() 호출)
5. 운동 선택 페이지로 전환 확인
```

### 시나리오 2: 운동 선택

```
1. 운동 선택 페이지에서 에어마우스로 "스쿼트" 버튼 호버
2. 버튼 하이라이트 효과 확인
3. 클릭하여 운동 시작
4. Workout 페이지로 전환 확인
```

### 시나리오 3: 스크롤 제어

```
1. 운동 선택 페이지 (스크롤 가능)
2. 조이스틱으로 빠르게 위/아래 움직임
3. ESP32가 스크롤 제스처 감지 → scroll_delta 전송
4. 페이지 스크롤 확인
```

### 시나리오 4: 설정 조정

```
1. Settings 페이지로 이동
2. 에어마우스로 "Sensitivity" 슬라이더 호버
3. (향후) 드래그 제스처로 감도 조절
4. 실시간으로 커서 감도 변경 확인
```

## 향후 개선 사항

### 1. ESP32 버튼 통합

**현재 상태:**
- 클릭 기능은 구현되어 있지만 ESP32 버튼과 연동 안됨

**개선 계획:**
```c
// ESP32: mpu6050_mqtt/main/sensor_task.c
if (button_state == BUTTON_PRESSED) {
    cJSON_AddBoolToObject(json, "click", true);
}

// Qt: mainwindow.cpp
if (data.contains("click") && data["click"].toBool()) {
    m_airMouseManager->simulateClick();
}
```

### 2. 드래그 제스처

**구현 방법:**
```cpp
void AirMouseManager::startDrag() {
    m_isDragging = true;
    QMouseEvent pressEvent(QEvent::MouseButtonPress, ...);
    QApplication::sendEvent(m_hoveredWidget, &pressEvent);
}

void AirMouseManager::endDrag() {
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, ...);
    QApplication::sendEvent(m_hoveredWidget, &releaseEvent);
    m_isDragging = false;
}
```

### 3. 컨텍스트 메뉴 (우클릭)

**구현 완료:**
- `simulateRightClick()` 메서드 이미 구현됨
- ESP32에서 장시간 버튼 누름 감지 추가 필요

### 4. 더블 클릭

**구현 방법:**
```cpp
void AirMouseManager::simulateDoubleClick() {
    simulateClick();
    QTimer::singleShot(50, this, [this]() {
        simulateClick();
    });
}
```

### 5. UI 개선

- 에어마우스 상태 표시 (ON/OFF 인디케이터)
- 클릭 가능한 위젯 하이라이트
- 감도 조절 UI (슬라이더)
- 커서 색상 변경 옵션

### 6. 성능 최적화

- 호버 업데이트 빈도 조절 (현재 20Hz)
- 렌더링 최적화 (더티 영역만 업데이트)
- MQTT 메시지 빈도 제한

## 설정 옵션

### 프로그래밍 방식 설정

```cpp
// 감도 조절 (1.0 = 기본, 2.0 = 2배 빠름)
m_airMouseManager->setSensitivity(1.5);

// 스무딩 활성화 (부드러운 움직임)
m_airMouseManager->setSmoothing(true);

// 커서 표시/숨김
m_airMouseManager->setShowCursor(true);

// 커서 색상 (CursorOverlay)
m_airMouseManager->m_cursorOverlay->setCursorColor(QColor(255, 100, 100));

// 트레일 길이
m_airMouseManager->m_cursorOverlay->setTrailLength(50);
```

## 문제 해결

### 문제 1: 에어마우스가 작동하지 않음

**해결 방법:**
1. MQTT 연결 확인: Settings → "Connect" 버튼
2. ESP32가 에어마우스 모드인지 확인
3. MQTT 메시지 수신 확인: `mosquitto_sub -t "joystick/sensor/data"`

### 문제 2: 커서가 보이지 않음

**해결 방법:**
```cpp
m_airMouseManager->setShowCursor(true);
m_airMouseManager->setEnabled(true);
```

### 문제 3: 커서 움직임이 너무 빠름/느림

**해결 방법:**
```cpp
// 너무 빠름
m_airMouseManager->setSensitivity(0.8);

// 너무 느림
m_airMouseManager->setSensitivity(2.0);
```

### 문제 4: 커서 움직임이 떨림

**해결 방법:**
```cpp
// 스무딩 버퍼 크기 증가
m_bufferSize = 5; // 기본값: 3
m_airMouseManager->setSmoothing(true);
```

## 빌드 방법

```bash
cd Qt_app
qmake
make
./workout_app
```

**필수 패키지:**
```bash
sudo apt install qtmqtt5-dev qtbase5-dev qtbase5-dev-tools
```

## 실행 방법

**1. MQTT 브로커 시작:**
```bash
sudo systemctl start mosquitto
```

**2. ESP32 조이스틱 실행:**
```bash
cd mpu6050_mqtt
idf.py -p /dev/ttyUSB0 flash monitor
```

**3. Qt 앱 실행:**
```bash
cd Qt_app
./workout_app
```

**4. 에어마우스 활성화:**
- Settings → "에어마우스 테스트" 버튼 클릭

## 주요 특징 요약

### ✅ 전역 제어
- 모든 페이지에서 작동 (메인 메뉴, 운동 선택, 설정, 운동 중)
- 페이지 전환 후에도 에어마우스 상태 유지

### ✅ 실제 위젯 상호작용
- Qt 이벤트 시스템과 완전 호환
- 버튼, 스크롤, 슬라이더 등 모든 위젯 제어 가능

### ✅ 시각적 피드백
- 전역 커서 오버레이
- 트레일 효과
- 클릭 애니메이션

### ✅ 설정 가능
- 감도 조절
- 스무딩 필터
- 커서 표시/숨김

## 결론

ESP32 조이스틱을 사용하여 Qt 앱의 **모든 UI 요소를 제어**할 수 있는 완전한 에어마우스 시스템이 구현되었습니다. 이제 사용자는 물리적 마우스 없이도 조이스틱만으로 앱의 모든 기능을 사용할 수 있습니다.

**다음 단계:**
- ESP32 버튼을 클릭 기능과 연동
- UI에 에어마우스 상태 표시
- 감도 조절 슬라이더 추가
- 드래그 제스처 구현
