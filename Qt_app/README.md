# Qt Workout Application

홈 트레이닝 시스템을 위한 Qt 기반 사용자 인터페이스 애플리케이션입니다.

## 주요 기능

### 1. 페이지 구성
- **메인 메뉴**: 운동 선택 또는 설정 페이지로 이동
- **운동 선택**: T-Pose, 스쿼트, 푸쉬업, 사용자 정의 운동 선택
- **설정**: MQTT 연결 설정, 에어마우스 감도 조정, 캘리브레이션
- **운동 실행**: 실시간 피드백, 점수, 심박수 표시 및 에어마우스 커서 제어

### 2. 에어마우스 기능
- 조이스틱(ESP32 MPU6050)을 이용한 에어마우스 제어
- 커서 감도 조정 가능
- 스무딩 및 궤적 표시 옵션
- 실시간 커서 위치 표시

### 3. MQTT 통신
- WatchTower와의 MQTT 통신으로 운동 명령 전송
- 조이스틱 및 워치 센서 데이터 수신
- 에어마우스 모드 자동 전환

## 파일 구조

```
Qt_app/
├── main.cpp                     # 애플리케이션 진입점
├── mainwindow.h                 # 메인 윈도우 헤더
├── mainwindow.cpp               # 메인 윈도우 구현 (페이지 전환 로직)
├── config.h / config.cpp        # 설정 관리 클래스
├── cursorcanvas.h / cursorcanvas.cpp  # 에어마우스 캔버스
├── main.ui                      # 메인 메뉴 UI
├── exercise_selection.ui        # 운동 선택 UI
├── settings.ui                  # 설정 UI
├── workout.ui                   # 운동 실행 UI
├── workout_app.pro              # Qt 프로젝트 파일
└── README.md                    # 이 파일
```

## 빌드 방법

### 필수 요구사항
- Qt 5.12 이상
- Qt MQTT 모듈
- C++17 컴파일러

### 빌드 명령

```bash
cd Qt_app

# qmake로 Makefile 생성
qmake workout_app.pro

# 빌드
make

# 실행
./workout_app
```

또는 Qt Creator를 사용하여:
1. Qt Creator 열기
2. `workout_app.pro` 파일 열기
3. 빌드 및 실행 (Ctrl+R)

## 설정 파일

`config.json` 파일을 생성하여 기본 설정을 변경할 수 있습니다:

```json
{
  "mqtt_broker": {
    "host": "localhost",
    "port": 1883,
    "client_id": "qt_workout_app",
    "username": "",
    "password": ""
  },
  "mqtt_topics": {
    "joystick_data": "joystick/sensor/data",
    "joystick_status": "joystick/status",
    "watch_heartrate": "watch/sensor/heartrate",
    "watch_status": "watch/status",
    "watchtower_cmd_joystick": "watchtower/command/joystick",
    "watchtower_cmd_watch": "watchtower/command/watch",
    "qt_cmd_select": "qt/command/select_mode",
    "qt_cmd_start": "qt/command/start",
    "qt_cmd_stop": "qt/command/stop",
    "qt_response": "qt/response/#"
  },
  "exercise_modes": [
    "T Pose",
    "Squat",
    "Pushup"
  ],
  "ui_settings": {
    "window_width": 800,
    "window_height": 600,
    "auto_connect": false,
    "save_window_position": true
  },
  "logging": {
    "enabled": true,
    "max_log_lines": 1000,
    "timestamps": true
  }
}
```

## 사용 방법

### 1. MQTT 연결
1. 설정 페이지로 이동
2. 브로커 주소 및 포트 입력 (기본값: localhost:1883)
3. "연결" 버튼 클릭

### 2. 운동 시작
1. 메인 메뉴에서 "운동 선택" 클릭
2. 원하는 운동 선택 (T-Pose, 스쿼트, 푸쉬업)
3. 운동 페이지에서 "시작" 버튼 클릭
4. 조이스틱으로 에어마우스 제어 및 운동 수행

### 3. 에어마우스 테스트
1. 설정 페이지로 이동
2. "에어마우스 테스트" 버튼 클릭
3. 조이스틱을 움직여 커서 제어 확인
4. 감도, 스무딩, 궤적 표시 옵션 조정

### 4. 캘리브레이션
1. 설정 페이지에서 "캘리브레이션" 버튼 클릭
2. 조이스틱이 평평한 곳에 놓여 있는지 확인
3. 캘리브레이션 완료 메시지 확인

## 페이지 전환 흐름

```
메인 메뉴
├── 운동 선택 → 운동 실행 페이지
└── 설정
    └── 에어마우스 테스트 → 운동 실행 페이지 (테스트 모드)
```

## MQTT 통신 프로토콜

### Qt → WatchTower 명령

**운동 시작:**
```json
{
  "command": "start",
  "mode": "t_pose",
  "timestamp": 1234567890
}
```

**운동 중지:**
```json
{
  "command": "stop",
  "timestamp": 1234567890
}
```

**에어마우스 모드:**
```json
{
  "command": "airmouse_mode"
}
```

**센서 모드:**
```json
{
  "command": "sensor_mode"
}
```

**캘리브레이션:**
```json
{
  "command": "calibrate"
}
```

### Joystick → Qt 데이터

**에어마우스 데이터:**
```json
{
  "mode": "airmouse",
  "mouse_x": 10.5,
  "mouse_y": -5.2,
  "scroll_delta": 0,
  "timestamp": 1234567890
}
```

### Watch → Qt 데이터

**심박수 데이터:**
```json
{
  "heartrate": 75,
  "timestamp": 1234567890
}
```

### WatchTower → Qt 응답

**운동 피드백:**
```json
{
  "status": "success",
  "score": 85,
  "feedback": "자세가 좋습니다",
  "is_correct": true
}
```

## 트러블슈팅

### MQTT 연결 실패
- mosquitto 브로커가 실행 중인지 확인: `sudo systemctl status mosquitto`
- 방화벽 설정 확인 (포트 1883)
- 브로커 주소와 포트가 올바른지 확인

### 에어마우스가 작동하지 않음
- 조이스틱이 MQTT 브로커에 연결되어 있는지 확인
- 설정에서 캘리브레이션 실행
- 에어마우스 모드가 활성화되어 있는지 확인

### UI가 업데이트되지 않음
- MQTT 토픽 이름이 config.json과 일치하는지 확인
- 로그를 확인하여 메시지 수신 여부 확인

## 참고 사항

- qt_test_app의 에어마우스 구현을 참고하여 작성되었습니다
- WatchTower 시스템과 통합하여 사용하도록 설계되었습니다
- 에어마우스 기능은 운동 중 커서를 제어하기 위해 자동으로 활성화됩니다
- 설정은 자동으로 config.json에 저장됩니다

## 향후 개선 사항

- [ ] 운동 기록 저장 및 조회
- [ ] 실시간 카메라 영상 표시
- [ ] 그래프를 통한 운동 통계 시각화
- [ ] 여러 사용자 프로필 지원
- [ ] 사용자 정의 운동 생성 기능
