# MPU6050 & WatchTower Qt5 Test Application

간단한 Qt5 C++ 테스트 애플리케이션으로 MPU6050 센서와 WatchTower 시스템의 MQTT 통신을 테스트할 수 있습니다.

## Features

### MQTT Connection
- MQTT 브로커 연결/해제
- 브로커 주소 및 포트 설정
- 실시간 연결 상태 표시

### WatchTower Commands
- **Exercise Mode Selection**: T-pose, Squat, Pushup 선택
- **Start/Stop Workout**: 운동 시작/정지 명령
- **Joystick Mode Control**:
  - Sensor Mode: 원시 센서 데이터 (가속도계, 자이로)
  - AirMouse Mode: 마우스 움직임 데이터
- **Calibration**: 조이스틱 캘리브레이션

### Real-time Data Display
- **Joystick (MPU6050) Data**:
  - Sensor Mode: Accel X/Y/Z, Gyro X/Y/Z
  - AirMouse Mode: Mouse X/Y, Scroll Delta
  - Device Status
- **Smart Watch Data**:
  - Heart Rate (BPM)
  - Device Status
- **WatchTower Response**:
  - AI 분석 결과
  - 모드 선택 확인
  - 시스템 상태

### Message Log
- 모든 MQTT 메시지 로그 (타임스탬프 포함)
- 색상별 메시지 분류 (연결, 오류, 수신, 발송)
- 로그 클리어 기능

### Configuration Management
- **config.json** 파일을 통한 설정 관리
- MQTT 브로커 설정
- MQTT 토픽 커스터마이징
- 운동 모드 추가/수정
- UI 설정 (창 크기, 자동 연결)
- 로깅 설정 (활성화, 최대 라인 수, 타임스탬프)

## Configuration

애플리케이션은 `config.json` 파일을 통해 모든 설정을 관리합니다. 첫 실행 시 기본 설정이 사용되며, 설정 파일을 수정하여 커스터마이징할 수 있습니다.

### config.json 구조

```json
{
    "mqtt_broker": {
        "host": "localhost",              // MQTT 브로커 주소
        "port": 1883,                     // MQTT 브로커 포트
        "client_id": "qt_test_app",       // MQTT 클라이언트 ID
        "username": "",                    // MQTT 인증 사용자명 (선택)
        "password": ""                     // MQTT 인증 비밀번호 (선택)
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
        "window_width": 900,              // 창 너비
        "window_height": 700,             // 창 높이
        "auto_connect": false,            // 시작 시 자동 연결
        "save_window_position": true      // 창 위치 저장 (향후 구현)
    },
    "logging": {
        "enabled": true,                  // 로깅 활성화
        "max_log_lines": 1000,           // 최대 로그 라인 수
        "timestamps": true                // 타임스탬프 표시
    }
}
```

### 설정 커스터마이징 예제

#### 1. Jetson Nano를 MQTT 브로커로 사용
```json
{
    "mqtt_broker": {
        "host": "192.168.1.100",
        "port": 1883,
        "client_id": "qt_test_app"
    }
}
```

#### 2. 새로운 운동 모드 추가
```json
{
    "exercise_modes": [
        "T Pose",
        "Squat",
        "Pushup",
        "Plank",
        "Lunges"
    ]
}
```

#### 3. 커스텀 MQTT 토픽 사용
```json
{
    "mqtt_topics": {
        "joystick_data": "custom/joystick/data",
        "watch_heartrate": "custom/watch/hr",
        "watchtower_cmd_joystick": "custom/cmd/joystick"
    }
}
```

#### 4. 자동 연결 활성화
```json
{
    "ui_settings": {
        "auto_connect": true
    }
}
```

#### 5. 로깅 비활성화 또는 제한
```json
{
    "logging": {
        "enabled": true,
        "max_log_lines": 500,
        "timestamps": false
    }
}
```

## Prerequisites

### Ubuntu/Debian
```bash
# Qt5 development tools
sudo apt-get install qt5-default qtbase5-dev

# Qt MQTT module
sudo apt-get install libqt5mqtt5 libqt5mqtt5-dev

# If Qt MQTT is not available in apt, install from source:
git clone https://github.com/qt/qtmqtt.git
cd qtmqtt
qmake
make
sudo make install
```

### Jetson Nano
```bash
# Qt5 is usually pre-installed, but if not:
sudo apt-get install qt5-default qtbase5-dev qtcreator

# Install Qt MQTT
sudo apt-get install libqt5mqtt5-dev
```

### MQTT Broker
```bash
# Install Mosquitto MQTT broker
sudo apt-get install mosquitto mosquitto-clients

# Start broker
sudo systemctl start mosquitto
sudo systemctl enable mosquitto

# Check status
sudo systemctl status mosquitto
```

## Build Instructions

### Method 1: Qt Creator (권장)

#### Qt6 Creator에서 Qt5 프로젝트 열기

**CMake 사용 (권장):**
```bash
# Qt Creator 실행
qtcreator

# File → Open File or Project
# qt_test_app/CMakeLists.txt 선택
```

**또는 qmake 사용:**
```bash
# File → Open File or Project
# qt_test_app/qt_test_app.pro 선택
```

**Kit 설정:**
1. Configure Project 화면에서 **Qt 5.15.3 Desktop** Kit 선택
2. Kit이 없으면 [QT_CREATOR_SETUP.md](QT_CREATOR_SETUP.md) 참조하여 수동 설정

**빌드 및 실행:**
- **Ctrl + B**: 빌드
- **Ctrl + R**: 실행

**상세한 Qt Creator 설정 방법은 [QT_CREATOR_SETUP.md](QT_CREATOR_SETUP.md) 참조**

### Method 2: Command Line (CMake)
```bash
cd qt_test_app

# Build directory 생성
mkdir build
cd build

# CMake 설정
cmake ..

# 빌드
make

# 실행
./qt_test_app
```

### Method 3: Command Line (qmake)
```bash
cd qt_test_app

# Generate Makefile
qmake qt_test_app.pro

# Build
make

# Run
./qt_test_app
```

## Usage

### 1. Configure Application (선택 사항)

첫 실행 전에 `config.json` 파일을 생성하여 설정을 커스터마이징할 수 있습니다:

```bash
cd qt_test_app

# config.json 파일이 없으면 기본 설정이 사용됩니다
# 필요시 config.json을 생성하고 수정:
cat > config.json << 'EOF'
{
    "mqtt_broker": {
        "host": "192.168.1.100",
        "port": 1883,
        "client_id": "qt_test_app"
    },
    "ui_settings": {
        "auto_connect": true
    }
}
EOF
```

**참고**: config.json이 없으면 애플리케이션이 기본 설정으로 실행되며, 로그에 "Failed to load config.json, using defaults" 메시지가 표시됩니다.

### 2. Start MQTT Broker
```bash
sudo systemctl start mosquitto

# Verify it's running
sudo lsof -i :1883
```

### 3. Launch Test Application
```bash
cd qt_test_app
./qt_test_app
```

애플리케이션 시작 시:
- `config.json` 파일을 로드 (존재하는 경우)
- 설정된 창 크기로 UI 초기화
- `auto_connect: true`인 경우 자동으로 MQTT 브로커에 연결

### 4. Connect to MQTT Broker (수동 연결)
1. 브로커 주소 확인 (config.json 또는 UI에서 수정 가능)
2. "Connect" 버튼 클릭
3. 연결 상태가 "Connected"로 변경되는지 확인
4. 로그에 "Subscribed to all topics" 메시지 확인

### 5. Test Joystick (MPU6050)
조이스틱 ESP32 디바이스가 실행 중이어야 합니다:

```bash
# ESP32 플래시 및 실행
cd mpu6050_mqtt
get_idf
idf.py -p /dev/ttyUSB0 flash monitor
```

애플리케이션에서:
- "Sensor Mode" 클릭 → 센서 데이터 확인
- "AirMouse Mode" 클릭 → 마우스 데이터 확인
- "Calibrate Joystick" 클릭 → 캘리브레이션

### 6. Test WatchTower Integration
WatchTower 시스템이 실행 중이어야 합니다:

```bash
cd WatchTower
python watchtower_main.py
```

애플리케이션에서:
1. Exercise Mode 선택 (T Pose/Squat/Pushup)
2. "Select Mode" 클릭
3. "Start Workout" 클릭
4. WatchTower Response에서 결과 확인
5. "Stop Workout" 클릭

## MQTT Topics

### Subscribed Topics (받는 토픽)
- `joystick/sensor/data` - 조이스틱 센서 또는 에어마우스 데이터
- `joystick/status` - 조이스틱 상태
- `watch/sensor/heartrate` - 심박수 데이터
- `watch/status` - 워치 상태
- `qt/response/#` - WatchTower 응답 (모든 서브토픽)

### Published Topics (보내는 토픽)
- `qt/command/select_mode` - 운동 모드 선택
- `qt/command/start` - 운동 시작
- `qt/command/stop` - 운동 정지
- `watchtower/command/joystick` - 조이스틱 명령
- `watchtower/command/watch` - 워치 명령

## Message Formats

### Start Command
```json
{
  "command": "start",
  "mode": "t_pose",
  "timestamp": 1234567890
}
```

### Joystick Sensor Data
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

### Joystick AirMouse Data
```json
{
  "mode": "airmouse",
  "mouse_x": 10.5,
  "mouse_y": -5.2,
  "scroll_delta": 1,
  "timestamp": 1234567890
}
```

## Troubleshooting

### Qt MQTT Module Not Found
```bash
# Check if Qt MQTT is installed
dpkg -l | grep libqt5mqtt

# Install from source if not available
git clone https://github.com/qt/qtmqtt.git
cd qtmqtt
git checkout 5.15  # or your Qt version
qmake
make
sudo make install
```

### MQTT Connection Failed
```bash
# Check if mosquitto is running
sudo systemctl status mosquitto

# Check port
sudo lsof -i :1883

# Test with mosquitto_sub
mosquitto_sub -h localhost -p 1883 -t '#' -v
```

### No Data from Joystick
1. ESP32가 실행 중인지 확인
2. ESP32 시리얼 모니터에서 WiFi/MQTT 연결 확인
3. `mqtt_monitor.py`로 메시지 확인:
   ```bash
   python mqtt_monitor.py
   ```

### Build Errors
```bash
# Clean and rebuild
make clean
qmake
make

# Or full clean
make distclean
qmake qt_test_app.pro
make
```

## Development

### Project Structure
```
qt_test_app/
├── CMakeLists.txt       # CMake project file (Qt Creator용)
├── qt_test_app.pro      # qmake project file
├── main.cpp             # Entry point
├── mainwindow.h         # Main window header
├── mainwindow.cpp       # Main window implementation
├── mainwindow.ui        # UI design file
├── config.h             # Configuration class header
├── config.cpp           # Configuration class implementation
├── config.json          # Configuration file (runtime)
├── QT_CREATOR_SETUP.md  # Qt Creator 설정 가이드
└── README.md            # This file
```

### Configuration Class (config.h/cpp)

Config 클래스는 싱글톤 패턴으로 구현되어 있으며, JSON 파일로부터 설정을 로드/저장합니다.

**주요 기능**:
- MQTT 브로커 설정 (host, port, client_id, username, password)
- MQTT 토픽 설정 (모든 subscribe/publish 토픽)
- 운동 모드 설정 (exercise_modes 배열)
- UI 설정 (창 크기, 자동 연결 등)
- 로깅 설정 (활성화, 최대 라인, 타임스탬프)

**사용 예제**:
```cpp
// Config 인스턴스 가져오기
Config &config = Config::instance();

// 설정 로드
config.loadFromFile("config.json");

// 설정 값 읽기
QString broker = config.mqttBroker();
int port = config.mqttPort();
QStringList modes = config.exerciseModes();

// 설정 값 변경
config.setMqttBroker("192.168.1.100");
config.setMqttPort(1883);

// 설정 저장
config.saveToFile("config.json");
```

### Adding New Features
1. Edit `mainwindow.ui` in Qt Designer for UI changes
2. Add slots in `mainwindow.h`
3. Implement slots in `mainwindow.cpp`
4. Connect signals/slots in constructor
5. For new configuration options:
   - Add member variables to `config.h`
   - Add getters/setters to `config.h`
   - Update `loadFromFile()` and `saveToFile()` in `config.cpp`
   - Update `config.json` with new fields

### Debugging
```bash
# Run with debug symbols
qmake CONFIG+=debug
make
gdb ./qt_test_app
```

## Related Documentation
- [CLAUDE.md](../CLAUDE.md) - Project overview and architecture
- [WatchTower/README.md](../WatchTower/README.md) - WatchTower system documentation
- [mpu6050_mqtt/README.md](../mpu6050_mqtt/README.md) - ESP32 joystick documentation

## License
This is part of the New_Workout_Plan project.
