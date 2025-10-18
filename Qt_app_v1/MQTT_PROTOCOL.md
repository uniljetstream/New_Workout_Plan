# Qt App ↔ WatchTower MQTT 통신 프로토콜

Qt 운동 앱과 WatchTower 시스템 간의 MQTT 통신 프로토콜 문서입니다.

## 목차
1. [운동 선택 및 시작 흐름](#운동-선택-및-시작-흐름)
2. [MQTT 토픽 구조](#mqtt-토픽-구조)
3. [메시지 형식](#메시지-형식)
4. [운동 이름 매핑](#운동-이름-매핑)

---

## 운동 선택 및 시작 흐름

### 1단계: 운동 선택 (Exercise Selection)

사용자가 운동을 선택하면:

```
사용자: "스쿼트" 버튼 클릭
   ↓
Qt App: 운동 이름 변환 ("스쿼트" → "squat")
   ↓
Qt App → WatchTower: 모드 선택 명령 전송
   Topic: qt/command/select_mode
   Message: {"mode": "squat", "timestamp": 1699999999}
   ↓
WatchTower: AI 서버에 모드 설정 요청
   ↓
WatchTower → Qt App: 모드 선택 확인 응답
   Topic: qt/response/mode_selected
   Message: {"status": "success", "mode": "squat"}
   ↓
Qt App: 운동 실행 페이지로 이동
```

### 2단계: 운동 시작 (Workout Start)

사용자가 "시작" 버튼을 누르면:

```
사용자: "시작" 버튼 클릭
   ↓
Qt App → WatchTower: 운동 시작 명령
   Topic: qt/command/start
   Message: {"command": "start", "mode": "squat", "timestamp": 1699999999}
   ↓
Qt App → Joystick: 에어마우스 모드 전환
   Topic: watchtower/command/joystick
   Message: {"command": "airmouse_mode"}
   ↓
WatchTower: 카메라 스트리밍 및 AI 분석 시작
   ↓
WatchTower → Qt App: 실시간 분석 결과 전송 (반복)
   Topic: qt/response/analysis
   Message: {
     "status": "success",
     "score": 85,
     "feedback": "자세가 좋습니다",
     "is_correct": true
   }
```

### 3단계: 운동 중지 (Workout Stop)

```
사용자: "중지" 버튼 클릭
   ↓
Qt App → WatchTower: 운동 중지 명령
   Topic: qt/command/stop
   Message: {"command": "stop", "timestamp": 1699999999}
   ↓
Qt App → Joystick: 센서 모드로 복귀 (선택 사항)
   Topic: watchtower/command/joystick
   Message: {"command": "sensor_mode"}
   ↓
WatchTower: 카메라 스트리밍 및 AI 분석 중지
```

---

## MQTT 토픽 구조

### Qt → WatchTower (Commands)

| Topic | 설명 | QoS |
|-------|------|-----|
| `qt/command/select_mode` | 운동 모드 선택 | 1 |
| `qt/command/start` | 운동 시작 | 1 |
| `qt/command/stop` | 운동 중지 | 1 |
| `qt/command/pose_index` | 현재 포즈 인덱스 업데이트 | 1 |
| `qt/command/request_analysis` | 단일 프레임 분석 요청 | 1 |

### WatchTower → Qt (Responses)

| Topic | 설명 | QoS |
|-------|------|-----|
| `qt/response/mode_selected` | 모드 선택 확인 | 1 |
| `qt/response/analysis` | 실시간 운동 분석 결과 | 1 |
| `qt/response/error` | 에러 메시지 | 1 |
| `qt/response/status` | 시스템 상태 업데이트 | 1 |

### Qt → Devices (Commands)

| Topic | 설명 | QoS |
|-------|------|-----|
| `watchtower/command/joystick` | 조이스틱 제어 명령 | 1 |
| `watchtower/command/watch` | 워치 제어 명령 | 1 |

### Devices → Qt (Data)

| Topic | 설명 | QoS |
|-------|------|-----|
| `joystick/sensor/data` | 조이스틱 센서/에어마우스 데이터 | 0 |
| `joystick/status` | 조이스틱 상태 | 1 |
| `watch/sensor/heartrate` | 심박수 데이터 | 0 |
| `watch/status` | 워치 상태 | 1 |

---

## 메시지 형식

### 1. 모드 선택 (qt/command/select_mode)

**Qt → WatchTower**

```json
{
  "mode": "squat",
  "timestamp": 1699999999
}
```

**WatchTower → Qt** (qt/response/mode_selected)

```json
{
  "status": "success",
  "mode": "squat",
  "timestamp": 1699999999
}
```

### 2. 운동 시작 (qt/command/start)

**Qt → WatchTower**

```json
{
  "command": "start",
  "mode": "squat",
  "timestamp": 1699999999
}
```

### 3. 운동 중지 (qt/command/stop)

**Qt → WatchTower**

```json
{
  "command": "stop",
  "timestamp": 1699999999
}
```

### 4. 실시간 분석 결과 (qt/response/analysis)

**WatchTower → Qt**

```json
{
  "status": "success",
  "score": 85,
  "feedback": "자세가 좋습니다",
  "is_correct": true,
  "keypoints": {
    "left_arm_angle": 165.3,
    "right_arm_angle": 170.2
  },
  "timestamp": 1699999999
}
```

### 5. 에러 메시지 (qt/response/error)

**WatchTower → Qt**

```json
{
  "message": "AI server connection failed",
  "code": "CONNECTION_ERROR",
  "timestamp": 1699999999
}
```

### 6. 에어마우스 모드 전환 (watchtower/command/joystick)

**Qt → Joystick**

```json
{
  "command": "airmouse_mode"
}
```

**또는 센서 모드로**

```json
{
  "command": "sensor_mode"}
```

**또는 캘리브레이션**

```json
{
  "command": "calibrate"
}
```

### 7. 에어마우스 데이터 (joystick/sensor/data)

**Joystick → Qt**

```json
{
  "mode": "airmouse",
  "mouse_x": 10.5,
  "mouse_y": -5.2,
  "scroll_delta": 0,
  "timestamp": 1699999999
}
```

### 8. 심박수 데이터 (watch/sensor/heartrate)

**Watch → Qt**

```json
{
  "heartrate": 75,
  "timestamp": 1699999999
}
```

---

## 운동 이름 매핑

Qt 앱의 UI는 한글 운동 이름을 사용하지만, MQTT 통신에서는 영어 모드명을 사용합니다.

| 한글 이름 | 영어 모드명 | WatchTower 지원 |
|-----------|-------------|-----------------|
| 스쿼트 | `squat` | ✅ Yes |
| 푸쉬업 | `pushup` | ✅ Yes |
| 플랭크 | `plank` | ⚠️ To be added |
| 런지 | `lunge` | ⚠️ To be added |
| 점핑잭 | `jumping_jack` | ⚠️ To be added |
| 마운틴 클라이머 | `mountain_climber` | ⚠️ To be added |
| 버피 | `burpee` | ⚠️ To be added |

**변환 로직:**

```cpp
QString MainWindow::convertExerciseNameToMode(const QString &exerciseName)
{
    static QMap<QString, QString> exerciseMap = {
        {"스쿼트", "squat"},
        {"푸쉬업", "pushup"},
        {"플랭크", "plank"},
        {"런지", "lunge"},
        {"점핑잭", "jumping_jack"},
        {"마운틴 클라이머", "mountain_climber"},
        {"버피", "burpee"}
    };

    return exerciseMap.value(exerciseName, "squat");  // Default: squat
}
```

---

## 주의사항

### WatchTower 설정 업데이트 필요

새로운 운동 모드를 추가하려면 WatchTower의 설정 파일을 업데이트해야 합니다:

**`WatchTower/watchtower_config.py`:**

```python
SUPPORTED_MODES = [
    'squat',
    'pushup',
    'plank',        # 추가
    'lunge',        # 추가
    'jumping_jack', # 추가
    'mountain_climber', # 추가
    'burpee'        # 추가
]
```

### AI 서버 설정 업데이트 필요

AI 서버에서 새로운 운동의 자세 분석 로직을 구현해야 합니다:

**`streaming/ai_server/ai_config.py`:**

```python
SUPPORTED_MODES = [
    'squat',
    'pushup',
    'plank',
    # ... 등등
]
```

**`streaming/ai_server/pose_analyzer.py`:**

각 운동에 대한 `_analyze_<mode>()` 메서드를 구현해야 합니다.

---

## 디버깅

### MQTT 메시지 모니터링

모든 MQTT 메시지를 모니터링하려면:

```bash
# 모든 토픽 구독
mosquitto_sub -h localhost -t '#' -v

# Qt 명령만 모니터링
mosquitto_sub -h localhost -t 'qt/command/#' -v

# WatchTower 응답만 모니터링
mosquitto_sub -h localhost -t 'qt/response/#' -v
```

### 수동으로 메시지 전송 (테스트)

```bash
# 모드 선택
mosquitto_pub -h localhost -t 'qt/command/select_mode' \
  -m '{"mode":"squat","timestamp":1699999999}'

# 운동 시작
mosquitto_pub -h localhost -t 'qt/command/start' \
  -m '{"command":"start","mode":"squat","timestamp":1699999999}'

# 운동 중지
mosquitto_pub -h localhost -t 'qt/command/stop' \
  -m '{"command":"stop","timestamp":1699999999}'
```

---

## 버전 정보

- **프로토콜 버전:** 1.0
- **마지막 업데이트:** 2025-10-13
- **호환 WatchTower 버전:** main branch
### 4. 단일 분석 요청 (qt/command/request_analysis)

**Qt → WatchTower**

```json
{
  "mode": "squat",
  "pose_index": 0,
  "timestamp": 1699999999
}
```

Qt는 pose_index 메시지 전송 후 이 요청을 발행하여 WatchTower가 해당 포즈에 대한 단일 프레임 분석을 수행하도록 트리거합니다.
