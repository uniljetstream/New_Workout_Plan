# 포즈 시퀀스 시스템 사용 가이드

## 개요

이제 각 운동 모드에서 **여러 개의 포즈**를 순차적으로 확인할 수 있습니다.

예를 들어 스쿼트 모드에서는:
1. **스쿼트 준비 자세** (선 자세) - 다리를 펴고 서 있는 자세
2. **스쿼트 자세** (무릎 90도) - 무릎을 굽혀 앉은 자세

이렇게 2개의 포즈를 순차적으로 확인합니다.

## 변경된 API

### 1. 모드 선택 API (변경됨)

**요청**: `POST /api/mode/select`
```json
{
  "mode": "squat"
}
```

**응답** (포즈 시퀀스 정보 추가):
```json
{
  "status": "success",
  "mode": "squat",
  "message": "SQUAT mode selected",
  "total_poses": 2,
  "poses": [
    {
      "name": "squat_stand",
      "description": "스쿼트 준비 자세 (선 자세)",
      "duration": 1.0
    },
    {
      "name": "squat_down",
      "description": "스쿼트 자세 (무릎 90도)",
      "duration": 2.0
    }
  ]
}
```

### 2. 프레임 분석 API (변경됨)

**요청**: `POST /api/stream/frame`
```json
{
  "frame": "base64_encoded_image",
  "pose_index": 0,  // 🆕 현재 확인할 포즈 인덱스 (0부터 시작)
  "timestamp": 1234567890
}
```

**응답** (keypoints 제거, 포즈 정보 추가):
```json
{
  "status": "success",
  "is_correct": false,
  "score": 75,
  "feedback": "왼쪽 다리 펴기 (155°)",
  "current_pose": "squat_stand",  // 🆕 현재 포즈 이름
  "pose_description": "스쿼트 준비 자세 (선 자세)",  // 🆕 포즈 설명
  "tracking": {
    "center_x": 320,
    "center_y": 240,
    "bbox": [100, 50, 540, 430]
  }
}
```

**변경 사항**:
- ✅ `pose_index` 파라미터 추가 (현재 확인할 포즈 지정)
- ✅ `current_pose` 필드 추가 (현재 확인 중인 포즈 이름)
- ✅ `pose_description` 필드 추가 (현재 포즈 설명)
- ❌ `keypoints` 필드 제거 (각도 정보 제거)

## WatchTower에서 사용하는 방법

### 방법 1: 포즈 인덱스 수동 변경

```python
from http_client import HTTPClient

client = HTTPClient()

# 1. 모드 선택
result = client.select_mode('squat')
print(f"총 포즈 개수: {result['total_poses']}")
for i, pose in enumerate(result['poses']):
    print(f"  [{i}] {pose['description']}")

# 2. 첫 번째 포즈 분석 (준비 자세)
client.set_pose_index(0)  # 포즈 인덱스 0으로 설정
result = client.send_frame(frame)  # pose_index=0으로 전송됨
print(f"현재 포즈: {result['pose_description']}")
print(f"피드백: {result['feedback']}")

# 3. 두 번째 포즈 분석 (앉은 자세)
client.set_pose_index(1)  # 포즈 인덱스 1으로 설정
result = client.send_frame(frame)  # pose_index=1으로 전송됨
print(f"현재 포즈: {result['pose_description']}")
print(f"피드백: {result['feedback']}")
```

### 방법 2: 포즈 인덱스 직접 전달

```python
# 포즈 인덱스를 직접 전달 (내부 상태 변경 없음)
result = client.send_frame(frame, pose_index=0)  # 첫 번째 포즈
result = client.send_frame(frame, pose_index=1)  # 두 번째 포즈
```

### 방법 3: 자동 포즈 전환 (시간 기반)

```python
import time

client = HTTPClient()
result = client.select_mode('squat')

# 포즈별 duration에 따라 자동 전환
for i, pose in enumerate(result['poses']):
    duration = pose.get('duration', 3.0)  # 기본 3초
    print(f"\n[{i}] {pose['description']} - {duration}초간 유지")

    client.set_pose_index(i)
    start_time = time.time()

    while time.time() - start_time < duration:
        ret, frame = camera.read()
        if not ret:
            continue

        analysis = client.send_frame(frame)

        if analysis['is_correct']:
            print(f"✓ 정확한 자세! 점수: {analysis['score']}%")
        else:
            print(f"✗ {analysis['feedback']}")

        time.sleep(1.0 / 5)  # 5 FPS

    print(f"→ {pose['description']} 완료!")
```

### 방법 4: 정확도 기반 자동 전환

```python
# 현재 포즈가 정확할 때 다음 포즈로 자동 전환
client = HTTPClient()
result = client.select_mode('squat')

correct_count = 0
required_correct_frames = 10  # 10프레임 연속 정확해야 다음 포즈로

for pose_idx in range(result['total_poses']):
    client.set_pose_index(pose_idx)
    correct_count = 0

    print(f"\n[{pose_idx}] {result['poses'][pose_idx]['description']}")

    while correct_count < required_correct_frames:
        ret, frame = camera.read()
        if not ret:
            continue

        analysis = client.send_frame(frame)

        if analysis['is_correct']:
            correct_count += 1
            print(f"✓ 정확! ({correct_count}/{required_correct_frames})")
        else:
            correct_count = 0  # 리셋
            print(f"✗ {analysis['feedback']}")

        time.sleep(0.1)

    print(f"→ {result['poses'][pose_idx]['description']} 완료!")
```

## 현재 지원되는 모드 및 포즈

### 1. t_pose (T자 서기)
- **포즈 1개**
  - `t_pose_stand`: T자 서기 자세

### 2. squat (스쿼트)
- **포즈 2개**
  - `squat_stand`: 스쿼트 준비 자세 (선 자세)
  - `squat_down`: 스쿼트 자세 (무릎 90도)

### 3. pushup (푸시업) - 향후 구현 예정
- **포즈 2개**
  - `pushup_up`: 푸시업 준비 자세 (팔 펴기)
  - `pushup_down`: 푸시업 자세 (팔 굽히기)

## 새로운 운동 모드 추가 방법

### 1. ai_config.py에 포즈 정의 추가

```python
# ai_server/ai_config.py
MODE_POSES = {
    'new_exercise': [
        {
            'name': 'new_exercise_pose1',
            'description': '첫 번째 자세 설명',
            'duration': 2.0
        },
        {
            'name': 'new_exercise_pose2',
            'description': '두 번째 자세 설명',
            'duration': 3.0
        }
    ]
}
```

### 2. pose_analyzer.py에 분석 로직 추가

```python
# ai_server/pose_analyzer.py

def analyze_frame(self, frame):
    # ...기존 코드...

    # 포즈 이름에 따라 분석
    pose_name = pose_info['name']

    if pose_name == 'new_exercise_pose1':
        result = self._analyze_new_exercise_pose1(xy, conf, bbox)
    elif pose_name == 'new_exercise_pose2':
        result = self._analyze_new_exercise_pose2(xy, conf, bbox)

    # ...

def _analyze_new_exercise_pose1(self, xy, conf, bbox=None):
    """새 운동 포즈 1 분석"""
    # YOLO 키포인트 추출 및 각도 계산
    # ...

    return {
        'status': 'success',
        'is_correct': True/False,
        'score': 0-100,
        'feedback': '피드백 메시지',
        'tracking': {...}  # 선택사항
    }
```

### 3. SUPPORTED_MODES에 추가

```python
# ai_server/ai_config.py
SUPPORTED_MODES = [
    't_pose',
    'squat',
    'pushup',
    'new_exercise'  # 추가
]
```

## 테스트 방법

### AI 서버 테스트

```bash
cd ai_server
python ai_server.py
```

별도 터미널에서:
```bash
# 모드 선택 테스트
curl -X POST http://localhost:5000/api/mode/select \
  -H "Content-Type: application/json" \
  -d '{"mode": "squat"}'

# 응답 확인 (poses 배열 확인)
```

### WatchTower 테스트

```bash
cd WatchTower
python http_client.py
```

메인 함수가 자동으로 실행되어 테스트됩니다.

## 주의사항

1. **pose_index는 0부터 시작**합니다
2. **잘못된 pose_index 전달 시 에러** 반환됩니다
3. **모드 변경 시 pose_index는 자동으로 0으로 초기화**됩니다
4. **keypoints 필드는 더 이상 반환되지 않습니다** (내부 각도 정보 숨김)
5. **각 포즈의 duration은 권장 시간**이며, 클라이언트가 자유롭게 조정 가능합니다

## 문제 해결

### Q: "Invalid pose_index" 에러가 발생합니다
A: `total_poses` 범위를 초과했는지 확인하세요. 인덱스는 `0 ~ (total_poses - 1)` 범위입니다.

### Q: 모드 선택 후 poses 배열이 비어있습니다
A: `MODE_POSES`에 해당 모드가 정의되어 있는지 확인하세요.

### Q: 새로운 포즈를 추가했는데 "not implemented yet" 에러가 발생합니다
A: `pose_analyzer.py`의 `analyze_frame()` 메서드에서 해당 포즈 이름을 처리하는 분기를 추가해야 합니다.

## 예제 시나리오: 스쿼트 10회

```python
# 스쿼트 10회 카운트
client = HTTPClient()
result = client.select_mode('squat')

squat_count = 0
target_count = 10

while squat_count < target_count:
    # 1. 준비 자세 (서기)
    client.set_pose_index(0)
    while True:
        analysis = client.send_frame(camera.read()[1])
        if analysis['is_correct']:
            print("준비 자세 완료!")
            break
        print(f"피드백: {analysis['feedback']}")
        time.sleep(0.2)

    # 2. 스쿼트 자세 (앉기)
    client.set_pose_index(1)
    while True:
        analysis = client.send_frame(camera.read()[1])
        if analysis['is_correct']:
            squat_count += 1
            print(f"✓ 스쿼트 {squat_count}회 완료!")
            break
        print(f"피드백: {analysis['feedback']}")
        time.sleep(0.2)

print(f"🎉 스쿼트 {target_count}회 완료!")
```
