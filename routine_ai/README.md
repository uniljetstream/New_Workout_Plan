# AI 서버 문서

## 개요

Flask 기반 AI 서버로, YOLO Pose 모델을 사용하여 운동 자세를 실시간으로 분석한다.
WatchTower 앱에서 전송된 프레임을 받아 자세 판정 결과를 반환한다.

## 서버 정보

- **주소**: `http://0.0.0.0:5000`
- **모델**: YOLOv11 Pose (`yolo11s-pose.pt`)
- **지원 운동**: 17개 (루틴 3개 + 개별 운동 14개)

## API 엔드포인트

### 1. 운동 모드 선택
```
POST /api/mode/select
```

**Request Body**
```json
{
  "mode": "squat"
}
```

**Response**
```json
{
  "status": "success",
  "message": "SQUAT mode selected",
  "mode": "squat",
  "poses": [...],
  "total_poses": 2
}
```

선택한 운동 모드에 따라 분석기를 초기화한다. 포즈 시퀀스 정보를 반환한다.

### 2. 프레임 분석
```
POST /api/stream/frame
```

**Request Body**
```json
{
  "frame": "base64_encoded_image",
  "pose_index": 0,
  "timestamp": 1234567890
}
```

**Response**
```json
{
  "status": "success",
  "is_correct": true,
  "score": 100,
  "feedback": "Perfect squat!",
  "current_pose": "squat_down",
  "pose_description": "스쿼트 자세 (무릎 90도)",
  "keypoints": {
    "xy": [[x1, y1], [x2, y2], ...],
    "conf": [0.9, 0.8, ...]
  },
  "tracking": {
    "center_x": 320,
    "center_y": 240,
    "bbox": [x1, y1, x2, y2]
  }
}
```

Base64로 인코딩된 프레임을 받아 YOLO Pose 추론을 수행하고, 현재 포즈에 맞는 자세를 분석한다.

### 3. 스트리밍 중단
```
POST /api/stream/stop
```

**Request Body**
```json
{
  "mode": "squat"
}
```

**Response**
```json
{
  "status": "success",
  "message": "Stream stopped",
  "mode": "squat"
}
```

### 4. 서버 상태 확인
```
GET /api/status
```

**Response**
```json
{
  "status": "running",
  "current_mode": "squat",
  "supported_modes": [...]
}
```

### 5. 헬스 체크
```
GET /api/health
```

**Response**
```json
{
  "status": "healthy"
}
```

## 지원 운동 모드

### 카테고리 루틴 (3개)

#### 1. bodyweight_routine (맨몸 운동 루틴)
- Squat (측면)
- Pushup (측면)
- Plank (측면)
- Lunge (측면)

#### 2. kettlebell_routine (케틀벨 루틴)
- Kettlebell Swing (측면)
- Kettlebell Deadlift (측면)
- Side Lunge (정면)
- Bridge (측면)
- Knee Drive (정면)

#### 3. barbell_routine (바벨 루틴)
- Barbell Row (측면)
- Barbell Upright Row (측면)
- Barbell Overhead Press (측면)
- Barbell Biceps Curl (측면)
- Barbell Reverse Curl (측면)

### 맨몸 운동 (4개)

- **squat** - 스쿼트 (측면)
- **pushup** - 푸시업 (측면)
- **plank** - 플랭크 (측면)
- **lunge** - 런지 (측면)

### 케틀벨 운동 (5개)

- **kettlebell_swing** - 케틀벨 스윙 (측면)
- **kettlebell_deadlift** - 케틀벨 데드리프트 (측면)
- **side_lunge** - 사이드 런지 (정면)
- **bridge** - 브릿지 (측면)
- **knee_drive** - 니 드라이브 (정면)

### 바벨 운동 (5개)

- **barbell_row** - 바벨 로우 (측면)
- **barbell_upright_row** - 바벨 업라이트 로우 (측면)
- **barbell_overhead_press** - 바벨 오버헤드 프레스 (측면)
- **barbell_biceps_curl** - 바벨 바이셉스 컬 (측면)
- **barbell_reverse_curl** - 바벨 리버스 컬 (측면)

## 정면/측면 기준

- **측면 촬영**: 니드라이브, 사이드런지를 제외한 모든 운동
- **정면 촬영**: 니드라이브, 사이드런지

## 코드 구조

### ai_server.py

Flask 서버의 메인 파일. API 엔드포인트를 정의하고 요청을 처리한다.

**주요 함수**
- `select_mode()` - 운동 모드 선택 처리
- `analyze_frame()` - 프레임 분석 및 자세 판정
- `stop_stream()` - 스트리밍 종료
- `get_status()` - 서버 상태 반환
- `health_check()` - 헬스 체크

### pose_analyzer.py

YOLO Pose 기반 자세 분석 모듈.

**PoseAnalyzer 클래스**
- `set_mode(mode)` - 운동 모드 설정
- `set_pose_index(pose_index)` - 현재 포즈 인덱스 설정
- `get_current_pose_info()` - 현재 포즈 정보 반환
- `analyze_frame(frame)` - 프레임 분석 및 자세 판정

**자세 분석 함수**

각 운동의 시작/완료 자세를 분석하는 함수들이 구현되어 있다.
- `_analyze_squat_stand()`, `_analyze_squat_down()`
- `_analyze_pushup_up()`, `_analyze_pushup_down()`
- `_analyze_swing_start()`, `_analyze_swing_up()`
- `_analyze_deadlift_down()`, `_analyze_deadlift_up()`
- `_analyze_row_start()`, `_analyze_row_pull()`, `_analyze_row_hold()`
- `_analyze_upright_start()`, `_analyze_upright_top()`
- `_analyze_overhead_start()`, `_analyze_overhead_top()`
- `_analyze_curl_down()`, `_analyze_curl_up()`
- `_analyze_lunge_center()`, `_analyze_lunge_left()`, `_analyze_lunge_right()`
- `_analyze_plank_knee()`, `_analyze_plank_hold()`
- `_analyze_bridge_down()`, `_analyze_bridge_up()`
- `_analyze_knee_start()`, `_analyze_knee_left()`, `_analyze_knee_right()`

**각도 계산**
- `_calculate_angle(p1, p2, p3)` - 3개 점으로 관절 각도 계산

### ai_config.py

서버 설정 및 운동 판정 기준값을 정의한다.

**AIServerConfig 클래스**

- **서버 설정**: HOST, PORT, DEBUG
- **모델 설정**: MODEL_PATH, CONFIDENCE_THRESHOLD
- **지원 모드**: SUPPORTED_MODES 리스트
- **포즈 시퀀스**: MODE_POSES 딕셔너리
- **판정 기준값**: 각 운동별 각도 임계값

## 자세 판정 방식

1. YOLO Pose로 17개 키포인트 추출
2. 신뢰도 임계값(0.5) 이상인 키포인트만 사용
3. 관절 각도 계산 (3개 점으로 각도 산출)
4. 설정된 임계값과 비교하여 정확도 점수 계산
5. 피드백 메시지 생성

**점수 산출**
- 각 체크 항목별로 점수 할당
- 모든 항목이 기준 내에 있으면 100점
- 부족한 항목은 피드백 메시지에 포함

## 키포인트 인덱스

YOLO Pose는 COCO 형식의 17개 키포인트를 사용한다.
```
0: nose
1: left_eye
2: right_eye
3: left_ear
4: right_ear
5: left_shoulder
6: right_shoulder
7: left_elbow
8: right_elbow
9: left_wrist
10: right_wrist
11: left_hip
12: right_hip
13: left_knee
14: right_knee
15: left_ankle
16: right_ankle
```

## 실행 방법
```bash
python ai_server.py
```

서버가 `0.0.0.0:5000`에서 실행된다.
