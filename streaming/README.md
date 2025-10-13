# WatchTower ↔ AI Server HTTP 통신

WatchTower(Jetson Nano)와 AI 서버 간 HTTP REST API 기반 실시간 운동 자세 분석 시스템

## 📁 디렉토리 구조

```
streaming/
├── ai_server/              # AI 분석 서버 (Flask)
│   ├── ai_server.py        # Flask 서버 메인
│   ├── ai_config.py        # 서버 설정
│   └── pose_analyzer.py    # YOLO Pose 분석 모듈
│
├── watchtower/             # WatchTower 클라이언트
│   ├── watchtower_client.py   # HTTP 클라이언트 메인
│   └── watchtower_config.py   # 클라이언트 설정
│
├── jetson_streaming/       # (기존) Jetson TCP 스트리밍
└── server_streaming/       # (기존) TCP 수신 서버
```

## 🔄 통신 프로토콜

### 1. 운동 모드 선택
**WatchTower → AI Server**
```http
POST /api/mode/select
Content-Type: application/json

{
  "mode": "t_pose"
}
```

**Response**
```json
{
  "status": "success",
  "message": "T_POSE mode selected",
  "mode": "t_pose"
}
```

### 2. 영상 스트리밍 (반복)
**WatchTower → AI Server**
```http
POST /api/stream/frame
Content-Type: application/json

{
  "frame": "base64_encoded_jpeg_image",
  "timestamp": 1234567890
}
```

**Response (실시간 분석 결과)**
```json
{
  "status": "success",
  "is_correct": false,
  "score": 75,
  "feedback": "왼팔 수평 (25°)",
  "keypoints": {
    "left_arm_angle": 165.3,
    "right_arm_angle": 170.2,
    "left_horizontal": 25.1,
    "right_horizontal": 15.3
  }
}
```

### 3. 스트리밍 중단
**WatchTower → AI Server**
```http
POST /api/stream/stop
Content-Type: application/json

{
  "mode": "t_pose"
}
```

**Response**
```json
{
  "status": "success",
  "message": "Stream stopped"
}
```

## 🚀 사용 방법

### 1. AI 서버 실행

```bash
# 1. YOLO 모델 준비
cd /home/ubuntu07/workingspace/final_project/New_Workout_Plan
# yolo11s-pose.pt 파일이 이 위치에 있어야 함

# 2. 서버 설정 (필요시)
# streaming/ai_server/ai_config.py 에서 HOST, PORT 수정

# 3. AI 서버 실행
cd streaming/ai_server
python3 ai_server.py
```

서버가 정상 실행되면:
```
==================================================
  AI 서버 (YOLO Pose 분석)
==================================================
주소: http://0.0.0.0:5000
지원 모드: t_pose, squat, pushup
모델: yolo11s-pose.pt
==================================================

API 엔드포인트:
  POST /api/mode/select  - 운동 모드 선택
  POST /api/stream/frame - 프레임 분석
  POST /api/stream/stop  - 스트리밍 중단
  GET  /api/status       - 서버 상태 확인
  GET  /api/health       - 헬스 체크
==================================================
```

### 2. WatchTower 클라이언트 실행

```bash
# 1. 클라이언트 설정
# streaming/watchtower/watchtower_config.py 에서 설정 수정
# AI_SERVER_HOST = '192.168.1.100'  # AI 서버의 실제 IP로 변경
# UART_PORT = '/dev/ttyUSB0'        # STM32 UART 포트 (필요시 수정)
# PANTILT_ENABLED = True            # 팬틸트 제어 활성화 여부

# 2. UART 권한 설정 (Jetson Nano에서 필요)
sudo usermod -a -G dialout $USER
# 재로그인 필요

# 3. WatchTower 클라이언트 실행
cd streaming/watchtower
python3 watchtower_client.py
```

클라이언트가 정상 실행되면:
```
==================================================
  WatchTower 클라이언트
==================================================
AI 서버: 192.168.1.100:5000
카메라: /dev/video0
스트림 FPS: 10
==================================================
✓ AI 서버 연결 성공: http://192.168.1.100:5000
→ 운동 모드 선택 요청: t_pose
✓ 서버 응답: T_POSE mode selected
✓ 카메라 초기화 완료: 640x480
✓ 팬틸트 추적기 초기화 완료
✓ UART 연결 성공: /dev/ttyUSB0 @ 115200 baud
✓ 팬틸트 제어 초기화 완료

==================================================
  스트리밍 시작: t_pose
  목표 FPS: 10
  지속 시간: 30초
  팬틸트 제어: 활성화
  종료: 'q' 키
==================================================

[0001] ✗ 조정 필요 | 점수: 50% | 왼팔 펴기 (155°), 왼팔 수평 (25°) | 추적: (350, 240) | Pan=95° Tilt=90°
[0002] ✗ 조정 필요 | 점수: 75% | 왼팔 수평 (22°) | 추적: (360, 235) | Pan=98° Tilt=88°
[0003] ✓ 정확 | 점수: 100% | 완벽한 T자 자세! | 추적: (320, 240) | Pan=90° Tilt=90°
...
```

### 3. 프로그램 종료

- WatchTower 클라이언트: `q` 키 또는 `Ctrl+C`
- AI 서버: `Ctrl+C`

## ⚙️ 설정 파일

### AI 서버 설정 (ai_config.py)

```python
class AIServerConfig:
    HOST = '0.0.0.0'                      # 서버 바인딩 주소
    PORT = 5000                           # 서버 포트
    MODEL_PATH = 'yolo11s-pose.pt'        # YOLO 모델 경로
    CONFIDENCE_THRESHOLD = 0.5            # 키포인트 신뢰도
    T_POSE_ARM_STRAIGHT_THRESHOLD = 160   # T자 팔 펴기 기준 (도)
    T_POSE_HORIZONTAL_THRESHOLD = 20      # T자 수평 기준 (도)
```

### WatchTower 설정 (watchtower_config.py)

```python
class WatchTowerConfig:
    # AI 서버 연결
    AI_SERVER_HOST = '192.168.1.100'  # AI 서버 IP (변경 필수!)
    AI_SERVER_PORT = 5000              # AI 서버 포트

    # 카메라 설정
    CAMERA_ID = 0                      # 카메라 장치 ID
    CAMERA_WIDTH = 640                 # 프레임 너비
    CAMERA_HEIGHT = 480                # 프레임 높이
    STREAM_FPS = 10                    # 전송 FPS (너무 높으면 부하 증가)
    JPEG_QUALITY = 85                  # JPEG 압축 품질
    SHOW_PREVIEW = True                # 로컬 프리뷰 표시 여부

    # 팬틸트 제어 (STM32 UART 통신, MG996R: 120도 가동)
    PANTILT_ENABLED = True             # 팬틸트 제어 활성화 여부
    UART_PORT = '/dev/ttyUSB0'         # UART 포트 (Jetson: /dev/ttyUSB0 또는 /dev/ttyACM0)
    UART_BAUDRATE = 115200             # 통신 속도
    PAN_MAX = 120                      # Pan 최대 각도 (MG996R: 120도)
    PAN_CENTER = 60                    # Pan 중앙 각도
    TILT_MAX = 120                     # Tilt 최대 각도 (MG996R: 120도)
    TILT_CENTER = 60                   # Tilt 중앙 각도
    TRACKING_SPEED = 10                # 추적 속도 (도/프레임)
    TRACKING_MAX_DELTA = 15            # 한 프레임당 최대 각도 변화량
```

## 🎯 지원하는 운동 모드

현재 구현된 모드:
- ✅ **t_pose** - T자 서기 (완전 구현)
- ⏳ **squat** - 스쿼트 (예약)
- ⏳ **pushup** - 푸시업 (예약)

## 🔧 커스터마이징

### 새로운 운동 모드 추가

1. `ai_server/ai_config.py`에 모드 추가:
```python
SUPPORTED_MODES = ['t_pose', 'squat', 'pushup', 'my_new_mode']
```

2. `ai_server/pose_analyzer.py`에 분석 로직 추가:
```python
def _analyze_my_new_mode(self, xy, conf):
    # 키포인트 분석 로직
    return {
        'status': 'success',
        'is_correct': True/False,
        'score': 0-100,
        'feedback': '피드백 메시지'
    }
```

3. `analyze_frame()` 메서드에 모드 연결:
```python
if self.current_mode == 'my_new_mode':
    return self._analyze_my_new_mode(xy, conf)
```

## 📊 API 테스트

### curl로 테스트

```bash
# 1. 서버 상태 확인
curl http://localhost:5000/api/health

# 2. 운동 모드 선택
curl -X POST http://localhost:5000/api/mode/select \
  -H "Content-Type: application/json" \
  -d '{"mode": "t_pose"}'

# 3. 서버 상태 조회
curl http://localhost:5000/api/status
```

## 🐛 문제 해결

### AI 서버 연결 실패
```
✗ AI 서버 연결 실패: Connection refused
```
→ `watchtower_config.py`의 `AI_SERVER_HOST`를 AI 서버의 실제 IP로 수정

### 카메라 열기 실패
```
✗ 카메라 열기 실패: 0
```
→ `watchtower_config.py`의 `CAMERA_ID` 확인 (0, 1, 2 등 시도)
→ `ls /dev/video*`로 카메라 장치 확인

### YOLO 모델 로드 실패
```
FileNotFoundError: yolo11s-pose.pt
```
→ AI 서버를 실행하는 디렉토리에 `yolo11s-pose.pt` 파일 배치
→ 또는 `ai_config.py`의 `MODEL_PATH`를 절대 경로로 수정

### FPS가 너무 낮음
→ `watchtower_config.py`의 `STREAM_FPS` 값 낮추기 (10 → 5)
→ `JPEG_QUALITY` 값 낮추기 (85 → 70)
→ AI 서버의 GPU 사용 확인

### 팬틸트 UART 연결 실패
```
✗ UART 연결 실패: [Errno 13] Permission denied: '/dev/ttyUSB0'
```
→ UART 권한 설정 필요:
```bash
sudo usermod -a -G dialout $USER
# 재로그인 후 다시 실행
```

→ UART 포트 확인:
```bash
ls /dev/ttyUSB* /dev/ttyACM*
# 실제 포트에 맞게 watchtower_config.py의 UART_PORT 수정
```

### 팬틸트 서보가 움직이지 않음
→ STM32 펌웨어가 올바른 프로토콜로 구현되었는지 확인
→ UART 통신 속도(Baudrate) 일치 확인: 양쪽 모두 115200
→ UART 명령어 프로토콜 (MG996R: 0-120도):
  - `PAN:60\n` - Pan 60도로 이동 (중앙)
  - `TILT:45\n` - Tilt 45도로 이동
  - `PANTILT:60,60\n` - Pan 60도, Tilt 60도로 동시 이동 (중앙)
  - `CENTER\n` - 중앙 위치로 이동 (60,60)

## 🤖 팬틸트 카메라 자동 추적

### 작동 원리
1. **AI 분석**: YOLO Pose가 사람을 감지하고 바운딩 박스 반환
2. **추적 계산**: 바운딩 박스 중심점과 프레임 중앙 비교
3. **각도 계산**: 비례 제어로 Pan/Tilt 각도 계산
4. **스무딩**: 3프레임 이동 평균으로 부드러운 움직임
5. **UART 전송**: STM32로 각도 명령 전송
6. **서보 제어**: STM32가 MG966R 서보모터 제어

### UART 통신 프로토콜 (WatchTower → STM32)

**명령어 형식:**
```
<CMD>:<VALUE>\n
```

**지원 명령어:**
| 명령어 | 형식 | 설명 | 예시 |
|--------|------|------|------|
| PAN | `PAN:<각도>\n` | Pan 각도 설정 (-60~60도) | `PAN:0\n` |
| TILT | `TILT:<각도>\n` | Tilt 각도 설정 (-60~60도) | `TILT:-30\n` |
| PANTILT | `PANTILT:<pan>,<tilt>\n` | Pan/Tilt 동시 설정 | `PANTILT:0,0\n` |
| CENTER | `CENTER\n` | 중앙 위치로 복귀 (0,0) | `CENTER\n` |
| STOP | `STOP\n` | 서보 모터 정지 | `STOP\n` |

**참고**: MG996R 서보모터는 120도 가동 범위를 가지며, 중앙(0,0)을 기준으로 -60~60도 범위로 제어합니다.



### 추적 알고리즘 설정

`watchtower_config.py`에서 추적 파라미터 조정:

```python
# 추적 속도 (값이 클수록 빠르게 추적)
TRACKING_SPEED = 10              # 기본: 10 (도/프레임)

# 최대 각도 변화량 (급격한 움직임 방지)
TRACKING_MAX_DELTA = 15          # 기본: 15 (도/프레임)

# 스무딩 프레임 수 (많을수록 부드럽지만 느림)
TRACKING_SMOOTH_FRAMES = 3       # 기본: 3

# Dead Zone (중앙 근처는 움직이지 않음)
TRACKING_DEAD_ZONE_X = 30        # 기본: 30 픽셀
TRACKING_DEAD_ZONE_Y = 30        # 기본: 30 픽셀
```

### 팬틸트 제어 비활성화

추적 기능 없이 실행하려면:
```python
# watchtower_config.py
PANTILT_ENABLED = False
```

## 📝 TODO

- [ ] 스쿼트 자세 분석 구현
- [ ] 푸시업 자세 분석 구현
- [ ] 운동 세션 기록 저장 (JSON/DB)
- [ ] 웹 대시보드 UI 추가
- [ ] MQTT와 통합 (워치/조이스틱 데이터)
- [x] 팬틸트 카메라 자동 추적 기능
