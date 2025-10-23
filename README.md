# New Workout Plan

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-orange.svg)](https://github.com/espressif/esp-idf)
[![Python](https://img.shields.io/badge/Python-3.10+-green.svg)](https://www.python.org/)
[![Qt](https://img.shields.io/badge/Qt-6.x-brightgreen.svg)](https://www.qt.io/)

**New Workout Plan**은 AI 자세 인식과 IoT 센서를 결합한 스마트 홈 트레이닝 시스템입니다.
카메라 기반 YOLO Pose 분석, 심박수 모니터링, 가속도 센서 피드백을 통해 실시간으로 운동 자세를 교정하고 건강 데이터를 추적합니다.

## 목차

- [시스템 개요](#시스템-개요)
- [주요 기능](#주요-기능)
- [시스템 구성](#시스템-구성)
- [기술 스택](#기술-스택)
- [설치 및 실행](#설치-및-실행)
- [사용 방법](#사용-방법)
- [프로젝트 구조](#프로젝트-구조)
- [통신 프로토콜](#통신-프로토콜)
- [지원 운동 모드](#지원-운동-모드)
- [하드웨어 구성](#하드웨어-구성)
- [문제 해결](#문제-해결)
- [기여하기](#기여하기)
- [라이선스](#라이선스)

---

## 시스템 개요

New Workout Plan은 다음과 같은 핵심 컴포넌트로 구성됩니다:

```
┌─────────────────────────────────────────────────────────────────┐
│                        AI 서버 (Flask + YOLO)                    │
│  - 실시간 자세 분석 (YOLO11s-Pose)                               │
│  - 운동별 포즈 시퀀스 검증                                        │
│  - REST API 제공                                                 │
└──────────┬──────────────────────────────────────────────────────┘
           │ HTTP/REST (이미지 ↑ / 분석 결과 ↓)
           │
┌──────────┴──────────────────────────────────────────────────────┐
│              WatchTower (Jetson Nano)                            │
│  - MQTT 브로커 (Mosquitto)                                       │
│  - 카메라 스트리밍 (OpenCV)                                       │
│  - 팬-틸트 카메라 제어 (UART → STM32)                            │
│  - 데이터 통합 및 중계                                            │
└────────┬─────────────────────────────────────────┬──────────────┘
         │ MQTT Pub/Sub                            │
         │                                         │
┌────────┴──────────────┐              ┌───────────┴──────────────┐
│   Smart Watch          │              │      Joystick            │
│   (ESP32 + FreeRTOS)   │              │      (ESP32 + MPU6050)   │
│  - 심박수 센서          │              │  - 가속도/자이로 센서     │
│  - LVGL 터치 UI         │              │  - 에어마우스 모드        │
│  - 실시간 심박 전송     │              │  - 진동 피드백            │
└────────────────────────┘              └──────────────────────────┘
```

### 동작 흐름

1. **사용자**: Qt 애플리케이션에서 운동 모드 선택 (예: Squat, Lunge, Barbell Routine)
2. **WatchTower**:
   - AI 서버에 모드 설정 요청 (HTTP POST)
   - 조이스틱/워치에 시작 명령 전송 (MQTT)
   - 카메라 영상을 AI 서버로 전송
3. **AI 서버**: 실시간 자세 분석 후 피드백 반환 (점수, 교정 메시지)
4. **디바이스**:
   - **Smart Watch**: 심박수 측정 및 전송 (MQTT)
   - **Joystick**: 센서 데이터 또는 에어마우스 좌표 전송 (MQTT)
5. **Qt 앱**: 분석 결과, 심박수, 비디오 프레임을 실시간 표시

---

## 주요 기능

### 🏋️ AI 기반 자세 분석
- **YOLO11s-Pose** 모델을 사용한 17개 키포인트 검출
- 운동별 포즈 시퀀스 검증 (예: 스쿼트 준비 → 앉은 자세)
- 실시간 자세 점수 및 교정 피드백

### 💓 심박수 모니터링
- **MAX30102** 센서를 통한 실시간 심박 측정
- LVGL 기반 터치 스크린 UI
- MQTT를 통한 무선 데이터 전송

### 🎮 스마트 조이스틱
- **MPU6050** 가속도/자이로 센서
- **에어마우스 모드**: Qt 앱 UI 제어
- **센서 모드**: 운동 동작 추적
- 진동 모터를 통한 햅틱 피드백

### 📹 자동 팬-틸트 추적
- STM32 기반 서보 모터 제어 (MG996R x2)
- UART 통신으로 각도 명령 전송
- 사용자 움직임 자동 추적 (옵션)

### 🖥️ 통합 Qt 애플리케이션
- 운동 모드 선택 및 실시간 피드백 표시
- 심박수, 센서 데이터 시각화
- 에어마우스로 마우스 없이 UI 제어

---

## 시스템 구성

### 하드웨어 컴포넌트

| 구성 요소 | 하드웨어 | 역할 |
|----------|---------|------|
| **WatchTower** | Jetson Nano, STM32F411R, MG996R x2 | MQTT 브로커, 카메라, 팬-틸트 제어 |
| **AI 서버** | PC/Server (CUDA 권장) | YOLO Pose 분석 |
| **Smart Watch** | ESP32, MAX30102, ST7789 LCD, CST816S 터치 | 심박 측정 및 표시 |
| **Joystick** | ESP32, MPU6050, 진동 모터 | 운동 보조 및 에어마우스 |

### 소프트웨어 컴포넌트

| 디렉토리 | 언어/프레임워크 | 설명 |
|---------|----------------|------|
| [`WatchTower/`](WatchTower/) | Python, OpenCV, paho-mqtt | MQTT 허브 및 카메라 스트리밍 |
| [`ai_server_v3/`](ai_server_v3/) | Python, Flask, Ultralytics YOLO | 자세 분석 API 서버 |
| [`Qt_app_v3/`](Qt_app_v3/) | C++, Qt6 | 사용자 인터페이스 |
| [`smart_watch/`](smart_watch/) | C, ESP-IDF, FreeRTOS, LVGL | 스마트워치 펌웨어 |
| [`joystick/`](joystick/) | C, ESP-IDF, FreeRTOS | 조이스틱/에어마우스 펌웨어 |
| [`pan-Tilt-cam/`](pan-Tilt-cam/) | C, STM32 HAL | 팬-틸트 서보 제어 |

---

## 기술 스택

### 임베디드 시스템
- **ESP-IDF v5.x**: ESP32 펌웨어 개발
- **FreeRTOS**: 실시간 멀티태스킹
- **LVGL v8**: 임베디드 GUI 라이브러리
- **STM32 HAL**: STM32 드라이버

### AI/ML
- **YOLO11s-Pose**: Ultralytics 경량 자세 추정 모델
- **OpenCV**: 이미지 처리 및 카메라 입력
- **NumPy**: 수치 연산

### 백엔드
- **Flask**: REST API 서버
- **Mosquitto**: MQTT 브로커
- **paho-mqtt**: Python/C MQTT 클라이언트

### 프론트엔드
- **Qt6**: C++ GUI 프레임워크
- **Qt Widgets**: UI 컴포넌트

### 통신
- **MQTT**: IoT 디바이스 간 메시징
- **HTTP/REST**: WatchTower ↔ AI 서버
- **UART**: Jetson ↔ STM32

---

## 설치 및 실행

### 사전 요구사항

#### 공통
- **Wi-Fi 네트워크**: 모든 디바이스가 동일 네트워크에 연결
- **MQTT 브로커**: Jetson Nano에 Mosquitto 설치

#### WatchTower (Jetson Nano)
- **OS**: Ubuntu 20.04 (JetPack 4.6+)
- **Python**: 3.8+
- **카메라**: USB 웹캠 또는 CSI 카메라
- **UART**: STM32와 통신용 (ttyUSB0 등)

#### AI 서버
- **OS**: Linux/Windows/macOS
- **Python**: 3.10+
- **GPU**: CUDA 지원 GPU (옵션, CPU도 가능)

#### ESP32 디바이스
- **ESP-IDF**: v5.0+
- **USB 드라이버**: CP210x 또는 CH340

---

### 1. WatchTower 설치 (Jetson Nano)

#### 1.1 MQTT 브로커 설치
```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients -y
sudo systemctl enable mosquitto
sudo systemctl start mosquitto

# 연결 테스트
mosquitto_sub -h localhost -t "test" -v
```

#### 1.2 Python 환경 설정
```bash
cd /home/ubuntu07/workingspace/final_project/New_Workout_Plan

# 가상환경 생성
python3 -m venv venv
source venv/bin/activate

# 의존성 설치
pip install --upgrade pip
pip install opencv-python paho-mqtt requests pyserial numpy
```

#### 1.3 설정 파일 수정
[`WatchTower/watchtower_config.py`](WatchTower/watchtower_config.py) 파일을 수정합니다:

```python
# MQTT 브로커 설정 (로컬)
MQTT_BROKER_HOST = "localhost"
MQTT_BROKER_PORT = 1883

# AI 서버 주소 (AI 서버가 실행되는 머신의 IP)
AI_SERVER_HOST = "192.168.0.100"  # 실제 AI 서버 IP로 변경
AI_SERVER_PORT = 5000

# 카메라 설정
CAMERA_INDEX = 0  # /dev/video0
```

#### 1.4 실행
```bash
cd WatchTower
python3 watchtower_main.py
```

---

### 2. AI 서버 설치

#### 2.1 Python 환경 설정
```bash
cd ai_server_v3

# 가상환경 생성 (선택사항)
python3 -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate

# 의존성 설치
pip install --upgrade pip
pip install flask ultralytics opencv-python numpy
```

#### 2.2 YOLO 모델 다운로드
```bash
# yolo11s-pose.pt 파일이 없다면:
wget https://github.com/ultralytics/assets/releases/download/v8.0.0/yolo11s-pose.pt
# 또는 Python으로 자동 다운로드:
python3 -c "from ultralytics import YOLO; YOLO('yolo11s-pose.pt')"
```

#### 2.3 실행
```bash
python3 ai_server.py
# 출력: * Running on http://0.0.0.0:5000
```

---

### 3. Smart Watch 펌웨어 (ESP32)

#### 3.1 ESP-IDF 설정
```bash
# ESP-IDF 환경 활성화
cd ~/esp/esp-idf
source export.sh

cd /home/ubuntu07/workingspace/final_project/New_Workout_Plan/smart_watch
```

#### 3.2 설정 메뉴
```bash
idf.py menuconfig
```

다음 항목을 설정:
- **Workout MQTT 설정**:
  - MQTT Broker URI: `mqtt://192.168.x.x:1883` (Jetson IP)
  - Device ID: `esp32-heart-rate`
- **Wi-Fi Configuration**:
  - SSID: 본인 Wi-Fi 이름
  - Password: Wi-Fi 비밀번호

#### 3.3 빌드 및 플래시
```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

### 4. Joystick 펌웨어 (ESP32)

#### 4.1 설정
```bash
cd joystick
idf.py menuconfig
```

- **MQTT Configuration**:
  - Broker URI: `mqtt://192.168.x.x:1883`
- **Wi-Fi Configuration**: Smart Watch와 동일

#### 4.2 빌드 및 플래시
```bash
idf.py build
idf.py -p /dev/ttyUSB1 flash monitor
```

---

### 5. Qt 애플리케이션

#### 5.1 Qt 설치
```bash
# Ubuntu
sudo apt install qt6-base-dev qt6-multimedia-dev

# macOS
brew install qt@6

# Windows: Qt 공식 사이트에서 설치
```

#### 5.2 빌드
```bash
cd Qt_app_v3
qmake workout_app.pro  # 또는 Qt Creator에서 프로젝트 열기
make
```

#### 5.3 실행
```bash
./workout_app
```

---

### 6. STM32 팬-틸트 제어 (선택사항)

STM32CubeIDE에서 [`pan-Tilt-cam/`](pan-Tilt-cam/) 프로젝트를 열어 빌드 및 플래시합니다.

---

## 사용 방법

### 시작 순서

1. **AI 서버 실행** (터미널 1)
   ```bash
   cd ai_server_v3
   python3 ai_server.py
   ```

2. **WatchTower 실행** (터미널 2 - Jetson Nano)
   ```bash
   cd WatchTower
   python3 watchtower_main.py
   ```

3. **ESP32 디바이스 전원 켜기**
   - Smart Watch: 부팅 후 자동으로 MQTT 연결
   - Joystick: 기본 에어마우스 모드로 시작

4. **Qt 애플리케이션 실행** (터미널 3)
   ```bash
   cd Qt_app_v3
   ./workout_app
   ```

### 운동 시작하기

1. Qt 앱에서 **운동 모드 선택** (예: "Squat")
2. 카메라 앞에 서서 **자세 준비**
3. **"Start" 버튼** 클릭 또는 에어마우스로 선택
4. 화면에 표시되는 **실시간 피드백** 확인:
   - 자세 점수
   - 교정 메시지
   - 현재 포즈 단계
   - 심박수
5. 운동 완료 후 **"Stop" 버튼** 클릭

### 에어마우스 사용

- **마우스 이동**: 조이스틱을 기울여서 커서 이동
- **클릭**: 조이스틱 버튼 누르기
- **스크롤**: 특정 제스처 (구현에 따라 다름)

---

## 프로젝트 구조

```
New_Workout_Plan/
├── WatchTower/                   # MQTT 허브 및 카메라 제어
│   ├── watchtower_main.py        # 메인 스크립트
│   ├── mqtt_controller.py        # MQTT 클라이언트
│   ├── http_client.py            # AI 서버 통신
│   ├── uart_controller.py        # STM32 UART 통신
│   ├── pantilt_tracker.py        # 팬-틸트 추적 로직
│   └── watchtower_config.py      # 설정 파일
│
├── ai_server_v3/                 # AI 자세 분석 서버
│   ├── ai_server.py              # Flask API 서버
│   ├── pose_analyzer.py          # YOLO Pose 분석 로직
│   ├── ai_config.py              # 운동 모드 및 포즈 정의
│   └── yolo11s-pose.pt           # YOLO 모델 (gitignore)
│
├── Qt_app_v3/                    # Qt 사용자 인터페이스
│   ├── main.cpp
│   ├── mainwindow.cpp/h
│   ├── workout_page_widget.cpp/h
│   ├── airmouse_manager.cpp/h
│   └── *.ui                      # UI 디자인 파일
│
├── smart_watch/                  # ESP32 스마트워치
│   ├── main/
│   │   ├── watch_mqtt_client.c/h # MQTT 클라이언트
│   │   ├── sensor.c/h            # MAX30102 센서 드라이버
│   │   ├── ui.c/h                # LVGL UI
│   │   └── wifi.c/h              # Wi-Fi 관리
│   ├── components/               # LVGL, 디스플레이 드라이버
│   └── CMakeLists.txt
│
├── joystick/                     # ESP32 조이스틱/에어마우스
│   ├── main/
│   │   ├── mqtt_handler.c/h
│   │   ├── sensor_task.c/h       # MPU6050 센서
│   │   └── airmouse.c/h          # 에어마우스 로직
│   └── README.md
│
├── pan-Tilt-cam/                 # STM32 팬-틸트 제어
│   ├── Core/Src/main.c
│   └── *.ioc                     # STM32CubeMX 설정
│
├── .gitignore
├── README.md                     # 이 파일
└── SYSTEM_PROTOCOL.md            # 통신 프로토콜 문서
```

---

## 통신 프로토콜

자세한 MQTT 토픽, HTTP API 엔드포인트, 메시지 포맷은 [**SYSTEM_PROTOCOL.md**](SYSTEM_PROTOCOL.md) 문서를 참조하세요.

### MQTT 토픽 요약

| 발행자 | 토픽 | 페이로드 예시 |
|-------|------|--------------|
| Qt App | `qt/command/select_mode` | `{"mode":"squat","timestamp":...}` |
| Qt App | `qt/command/start` | `{"command":"start","mode":"squat"}` |
| WatchTower | `qt/response/analysis` | `{"score":85,"feedback":"자세 정확"}` |
| WatchTower | `watchtower/command/joystick` | `{"command":"sensor_mode"}` |
| Smart Watch | `watch/sensor/heartrate` | `{"heart_rate":75,"mode":"squat"}` |
| Joystick | `joystick/sensor/data` | `{"accel_x":0.5,"gyro_z":-0.2}` |

### HTTP API 요약

| 메서드 | 엔드포인트 | 설명 |
|--------|-----------|------|
| POST | `/api/mode/select` | 운동 모드 설정 |
| POST | `/api/stream/frame` | 프레임 분석 요청 |
| POST | `/api/stream/stop` | 스트리밍 중지 |
| GET | `/api/health` | 서버 상태 확인 |

---

## 지원 운동 모드

### 맨몸 운동 (Bodyweight)
- **Squat** (스쿼트): 준비 자세 → 앉은 자세
- **Lunge** (런지): 중앙 → 왼쪽 런지 → 중앙 → 오른쪽 런지

### 케틀벨 운동 (Kettlebell)
- **Kettlebell Swing**: 시작 → 스윙 업
- **Kettlebell Deadlift**: 바닥 → 서기

### 바벨 운동 (Barbell)
- **Barbell Row**: 시작 → 당기기
- **Barbell Upright Row**: 시작 → 당기기
- **Barbell Overhead Press**: 시작 → 머리 위로
- **Barbell Biceps Curl**: 팔 내림 → 팔 올림
- **Barbell Reverse Curl**: 팔 내림 → 팔 올림

### 루틴 모드
- **Bodyweight Routine**: 스쿼트 + 런지 연속 수행
- **Kettlebell Routine**: 스윙 + 데드리프트
- **Barbell Routine**: 5가지 바벨 운동 연속

---

## 하드웨어 구성

### 핀 연결 (참고)

#### Smart Watch (ESP32)
| 기능 | 핀 | 설명 |
|------|---|------|
| ST7789 LCD | SPI (MOSI, SCLK, CS, DC, RST) | 디스플레이 |
| CST816S 터치 | I2C (SDA, SCL) | 터치 입력 |
| MAX30102 | I2C (SDA, SCL) | 심박 센서 |
| 전원 | 3.3V, GND | |

#### Joystick (ESP32)
| 기능 | 핀 | 설명 |
|------|---|------|
| MPU6050 | I2C (SDA, SCL) | 가속도/자이로 |
| 진동 모터 | GPIO (PWM) | 햅틱 피드백 |
| 버튼 | GPIO (Input) | 클릭 입력 |

#### Pan-Tilt (STM32F411R)
| 기능 | 핀 | 설명 |
|------|---|------|
| Pan 서보 | TIM PWM | 좌우 회전 (MG996R) |
| Tilt 서보 | TIM PWM | 상하 회전 (MG996R) |
| UART | USART2 (TX, RX) | Jetson과 통신 |

---

## 문제 해결

### MQTT 연결 실패
```bash
# Mosquitto 상태 확인
sudo systemctl status mosquitto

# 방화벽 포트 열기 (필요시)
sudo ufw allow 1883/tcp

# 연결 테스트
mosquitto_sub -h <Jetson_IP> -t "test" -v
```

### AI 서버 연결 실패
- AI 서버가 실행 중인지 확인: `curl http://<AI_Server_IP>:5000/api/health`
- 방화벽에서 5000번 포트 허용
- `watchtower_config.py`의 `AI_SERVER_HOST` IP 주소 확인

### ESP32 Wi-Fi 연결 안 됨
- `idf.py menuconfig`에서 SSID/Password 재확인
- Wi-Fi 라우터 2.4GHz 대역 사용 확인 (ESP32는 5GHz 미지원)
- 시리얼 모니터에서 에러 로그 확인: `idf.py monitor`

### 카메라가 안 보임
```bash
# 카메라 장치 확인
ls /dev/video*

# 권한 확인
sudo chmod 666 /dev/video0

# OpenCV 테스트
python3 -c "import cv2; print(cv2.VideoCapture(0).isOpened())"
```

### Qt 앱에서 영상이 안 나옴
- WatchTower에서 `qt/response/frame` 토픽이 발행되는지 확인:
  ```bash
  mosquitto_sub -h localhost -t "qt/response/frame" -v
  ```
- Qt 앱의 MQTT 연결 상태 확인

---

## 기여하기

이 프로젝트는 교육 및 연구 목적으로 제작되었습니다. 기여를 환영합니다!

### 기여 방법
1. 이 저장소를 Fork합니다
2. 새로운 브랜치를 생성합니다 (`git checkout -b feature/new-exercise`)
3. 변경사항을 커밋합니다 (`git commit -m 'Add new exercise mode'`)
4. 브랜치에 Push합니다 (`git push origin feature/new-exercise`)
5. Pull Request를 생성합니다

### 개발 가이드
- 새로운 운동 모드 추가: [`ai_server_v3/ai_config.py`](ai_server_v3/ai_config.py) 수정
- MQTT 토픽 변경 시: `SYSTEM_PROTOCOL.md`도 함께 업데이트
- 코드 스타일: PEP 8 (Python), Google C++ Style Guide

---

## 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

---

## 감사의 글

- **Ultralytics YOLO**: https://github.com/ultralytics/ultralytics
- **ESP-IDF**: https://github.com/espressif/esp-idf
- **LVGL**: https://lvgl.io/
- **Mosquitto MQTT**: https://mosquitto.org/
- **Qt**: https://www.qt.io/

---

## 연락처

프로젝트 관련 문의: [GitHub Issues](https://github.com/uniljetstream/New_Workout_Plan/issues)

---

**Built with ❤️ for a healthier lifestyle**
