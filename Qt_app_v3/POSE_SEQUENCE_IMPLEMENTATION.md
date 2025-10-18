# Qt App Pose Sequence Implementation

## 개요

Qt 앱이 AI Server 및 WatchTower의 포즈 시퀀스 시스템과 동기화되도록 구현 완료.

## 구현된 기능

### 1. 포즈 시퀀스 상태 관리 (mainwindow.h)

추가된 멤버 변수:
```cpp
int m_currentPoseIndex;      // 현재 포즈 인덱스 (0부터 시작)
int m_totalPoses;            // 전체 포즈 개수
QJsonArray m_poses;          // 포즈 배열 (name, description, duration)
int m_repCount;              // 반복 횟수 카운터
```

추가된 헬퍼 메서드:
```cpp
void updatePoseDisplay();    // 현재 포즈 정보를 UI에 표시
void nextPose();             // 다음 포즈로 이동
bool isLastPose();           // 마지막 포즈인지 확인
```

### 2. 운동 종목 매핑 업데이트 (mainwindow.cpp)

T-Pose 삭제 및 9개 운동 종목 정의:
```cpp
static QMap<QString, QString> exerciseMap = {
    {"스쿼트", "squat"},                      // 구현됨
    {"플랭크", "plank"},                      // 미구현
    {"런지", "lunge"},                        // 미구현
    {"점핑잭", "jumping_jack"},               // 미구현
    {"마운틴 클라이머", "mountain_climber"},  // 미구현
    {"버피", "burpee"},                       // 미구현
    {"사용자 정의 1", "custom1"},             // 미구현
    {"사용자 정의 2", "custom2"}              // 미구현
};
```

### 3. 버튼 핸들러 수정 (mainwindow.cpp)

**구현된 운동:**
- `on_squatButton_clicked()` → 스쿼트 시작

**미구현 운동 (기타):**
- 나머지 버튼들은 "곧 출시됩니다" 메시지 표시

### 4. 모드 선택 응답 파싱 (handleQtResponse)

WatchTower로부터 `mode_selected` 응답 수신 시:
```cpp
if (data.contains("poses") && data["poses"].isArray()) {
    m_poses = data["poses"].toArray();
    m_totalPoses = m_poses.size();
    m_currentPoseIndex = 0;
    m_repCount = 0;
    updatePoseDisplay();
}
```

**예상 응답 형식:**
```json
{
  "status": "success",
  "mode": "squat",
  "message": "Mode selected: squat",
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
  ],
  "total_poses": 2
}
```

### 5. 실시간 분석 피드백 처리 (updateWorkoutFeedback)

AI 분석 결과 수신 시:

**포즈 전환 로직:**
```cpp
if (is_correct) {
    if (isLastPose()) {
        // 마지막 포즈 완료 → 1회 반복 완료
        m_repCount++;
        m_currentPoseIndex = 0;  // 첫 포즈로 리셋
    } else {
        // 다음 포즈로 이동
        nextPose();
    }
}
```

**예상 분석 응답 형식:**
```json
{
  "status": "success",
  "is_correct": true,
  "score": 95,
  "feedback": "완벽합니다!",
  "current_pose": "squat_stand",
  "pose_description": "스쿼트 준비 자세 (선 자세)"
}
```

### 6. UI 업데이트 (updatePoseDisplay)

현재 포즈 정보를 UI에 표시:
```cpp
// 운동 제목에 진행 상황 표시
ui_workout->exerciseTitleLabel->setText("운동: 스쿼트 (1/2)");

// 포즈 설명 표시
ui_workout->feedbackLabel->setText("스쿼트 준비 자세 (선 자세)");

// 반복 횟수 업데이트
ui_workout->repCountLabel->setText("반복 횟수: 3");
```

### 7. 최신 포즈 구성 참고

- 바벨 오버헤드 프레스는 AI 서버와 Qt 모두 2개 포즈(시작 `overhead_start`, 완료 `overhead_top`)만 사용하도록 단순화되었습니다. 서버에서 내려오는 포즈 배열이 즉시 UI에 반영되므로 별도 수동 설정 없이 Start → Top 순서를 반복합니다.

## 데이터 흐름

### 운동 시작 시퀀스

1. **사용자**: 스쿼트 버튼 클릭
2. **Qt**: `startWorkout("스쿼트")` 호출
3. **Qt**: `qt/command/select_mode` 토픽으로 `{"mode": "squat"}` 전송
4. **WatchTower**: AI Server에 모드 선택 요청
5. **AI Server**: 모드 설정 및 포즈 배열 반환
6. **WatchTower**: `qt/response/mode_selected` 토픽으로 응답
7. **Qt**: 포즈 배열 파싱, 첫 번째 포즈 표시

### 운동 중 시퀀스

1. **사용자**: "시작" 버튼 클릭
2. **Qt**: `qt/command/start` 토픽으로 명령 전송
3. **WatchTower**: 카메라 프레임을 AI Server로 전송 (pose_index 포함)
4. **AI Server**: 현재 포즈 분석 후 결과 반환
5. **WatchTower**: `qt/response/analysis` 토픽으로 결과 전송
6. **Qt**:
   - `is_correct == true` → 다음 포즈로 이동 또는 반복 완료
   - `is_correct == false` → 피드백 표시, 현재 포즈 유지

### 반복 완료 시퀀스

1. **Qt**: 마지막 포즈에서 `is_correct == true` 수신
2. **Qt**: `m_repCount++` (반복 횟수 증가)
3. **Qt**: `m_currentPoseIndex = 0` (첫 포즈로 리셋)
4. **Qt**: `updatePoseDisplay()` (UI 업데이트)
5. **Qt**: UI에 "반복 횟수: N" 표시

## 운동 모드별 포즈 구성

### 스쿼트 (squat)
1. **squat_stand**: 스쿼트 준비 자세 (선 자세)
   - 엉덩이-무릎-발목 각도 > 160°
2. **squat_down**: 스쿼트 자세 (무릎 90도)
   - 무릎 각도 80°~100°

## 향후 작업

### 필수 구현
1. **WatchTower 연동**: WatchTower가 Qt의 pose_index를 실시간으로 추적하도록 수정
   - 현재는 Qt에서만 pose_index 관리
   - WatchTower에서도 pose_index를 동기화해야 함

2. **Start 버튼 로직**: 운동 시작 시 WatchTower에 현재 pose_index 전송
   ```cpp
   void MainWindow::on_workout_startButton_clicked() {
       QJsonObject json;
       json["command"] = "start";
       json["mode"] = m_currentMode;
       json["pose_index"] = m_currentPoseIndex;
       json["timestamp"] = QDateTime::currentSecsSinceEpoch();
       publishMessage(m_config.topicQtCmdStart(), doc.toJson());
   }
   ```

3. **포즈 인덱스 동기화**: 포즈 전환 시 WatchTower에 알림
   ```cpp
   void MainWindow::nextPose() {
       if (m_currentPoseIndex < m_totalPoses - 1) {
           m_currentPoseIndex++;
           updatePoseDisplay();
           // WatchTower에 포즈 인덱스 변경 알림
           notifyPoseIndexChanged();
       }
   }
   ```

### 선택 구현
1. **포즈 타이머**: 각 포즈의 `duration` 사용하여 최소 유지 시간 구현
2. **음성 피드백**: 포즈 전환 시 음성 안내
3. **애니메이션**: 포즈 설명 이미지 또는 애니메이션 표시
4. **통계**: 운동 세션 통계 (평균 점수, 총 반복 횟수 등)

## 테스트 시나리오

### 스쿼트 1회 반복 시나리오
1. "스쿼트" 버튼 클릭
2. UI 확인: "운동: 스쿼트 (1/2)" / "스쿼트 준비 자세"
3. "시작" 버튼 클릭
4. 선 자세 유지 → `is_correct: true` → 자동으로 2번 포즈로 이동
5. UI 확인: "운동: 스쿼트 (2/2)" / "스쿼트 자세"
6. 무릎 90도 자세 유지 → `is_correct: true` → 반복 완료
7. UI 확인: "반복 횟수: 1" / 다시 1번 포즈로 리셋

## 파일 수정 내역

### mainwindow.h
- 포즈 시퀀스 관련 멤버 변수 추가
- 헬퍼 메서드 선언 추가
- **헤더 파일 추가**: `#include <QJsonObject>`, `#include <QJsonArray>`
  - QJsonArray 사용을 위해 필수
- `isLastPose()` 메서드를 `const`로 선언

### mainwindow.cpp
- 생성자에서 포즈 변수 초기화
- `convertExerciseNameToMode()`: T-Pose 삭제, 9개 운동 매핑
- 버튼 핸들러 수정 (스쿼트만 구현)
- `handleQtResponse()`: 포즈 배열 파싱 추가
- `updateWorkoutFeedback()`: 포즈 전환 로직 추가
- 헬퍼 메서드 구현:
  - `updatePoseDisplay()`: UI 업데이트
  - `nextPose()`: 다음 포즈 이동
  - `isLastPose() const`: 마지막 포즈 확인 (const 메서드)

## 호환성

이 구현은 다음 파일들과 호환됩니다:

- `ai_server/ai_config.py` - MODE_POSES 정의
- `ai_server/pose_analyzer.py` - 포즈 분석 및 응답 생성
- `ai_server/ai_server.py` - Flask API 엔드포인트
- `WatchTower/http_client.py` - HTTP 클라이언트 (pose_index 전송)
- `WatchTower/mqtt_controller.py` - MQTT 메시지 브리지

## 빌드 방법

### 필수 패키지 설치
```bash
sudo apt install qtmqtt5-dev qtbase5-dev qtbase5-dev-tools
```

### 빌드 명령어
```bash
cd Qt_app
qmake
make
```

### 실행
```bash
./workout_app
```

## 주의사항

1. **Qt5Mqtt 패키지 필요**: 빌드 전에 `sudo apt install qtmqtt5-dev` 실행
2. **MQTT 브로커 실행 필요**: `sudo systemctl start mosquitto`
3. **AI Server 실행 필요**: `cd ai_server && python ai_server.py`
4. **WatchTower 실행 필요**: `cd WatchTower && python watchtower_main.py`
5. **네트워크 설정**: 모든 config 파일에서 IP 주소 확인
6. **빌드 성공 확인**: `workout_app` 실행 파일이 생성되었는지 확인

## 디버그 로그

포즈 시퀀스 관련 주요 로그 메시지:

```
✓ Mode selected successfully: squat
Loaded 2 poses for mode: squat
Pose 1/2: 스쿼트 준비 자세 (선 자세)
Moving to next pose: 2/2
Pose 2/2: 스쿼트 자세 (무릎 90도)
Rep completed! Total reps: 1
Pose 1/2: 스쿼트 준비 자세 (선 자세)
```

## 문의

구현 관련 질문이나 버그 리포트는 프로젝트 이슈 트래커에 등록하세요.
