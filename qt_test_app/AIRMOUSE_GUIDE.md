# AirMouse 테스트 가이드

통합 테스트 앱에 **AirMouse Test** 탭이 추가되었습니다. ESP32 조이스틱의 AirMouse 기능을 시각적으로 테스트할 수 있습니다.

## 기능

### 가상 커서 캔버스
- **800x600 픽셀 캔버스**에서 가상 커서가 움직입니다
- **격자 및 중심선** 표시로 위치 확인
- **커서 트레일** - 움직임 경로를 시각화 (50개 포인트)
- **실시간 좌표 표시** - 캔버스 왼쪽 위에 표시

### 커서 디자인
- **빨간색 원형 커서** (그림자 효과 포함)
- **십자 조준선** - 정확한 위치 표시
- **트레일 효과** - 반투명 파란색 궤적

### 컨트롤
- **Enable AirMouse Mode**: ESP32를 AirMouse 모드로 전환
- **Sensor Mode**: 센서 모드로 전환
- **Calibrate**: 조이스틱 캘리브레이션
- **Reset Cursor**: 커서를 중앙으로 리셋

### 설정
- **Sensitivity 슬라이더**: 0.1x ~ 5.0x (기본 1.0x)
- **Enable Smoothing**: 움직임 평균화로 떨림 감소
- **Show Trail**: 커서 궤적 표시/숨김

### 데이터 표시
- Mouse X/Y: 실시간 마우스 델타 값
- Scroll: 스크롤 델타 값

## 사용 방법

### 1. MQTT 연결
```bash
# 브로커 시작
sudo systemctl start mosquitto

# 앱 실행
./qt_test_app

# Connect 버튼 클릭
```

### 2. ESP32 조이스틱 연결
ESP32가 실행 중이어야 합니다:
```bash
cd mpu6050_mqtt
get_idf
idf.py -p /dev/ttyUSB0 flash monitor
```

### 3. AirMouse 모드 활성화
1. **AirMouse Test** 탭 선택
2. **Enable AirMouse Mode** 버튼 클릭
3. ESP32가 AirMouse 모드로 전환됨
4. 조이스틱을 기울이면 캔버스의 커서가 움직입니다!

### 4. 감도 조절
- 너무 느리면: **Sensitivity 슬라이더**를 오른쪽으로 (2.0x ~ 3.0x)
- 너무 빠르면: **Sensitivity 슬라이더**를 왼쪽으로 (0.5x ~ 0.8x)
- 떨림이 심하면: **Enable Smoothing** 체크

### 5. 커서 리셋
**Reset Cursor** 버튼을 클릭하면 커서가 중앙으로 돌아갑니다.

## 동작 원리

### 데이터 플로우
```
ESP32 (MPU6050)
  → MQTT (airmouse 데이터)
    → Qt App
      → CursorCanvas 위젯
        → 가상 커서 움직임!
```

### MQTT 메시지 형식
```json
{
  "mode": "airmouse",
  "mouse_x": 10.5,
  "mouse_y": -5.2,
  "scroll_delta": 1,
  "timestamp": 1234567890
}
```

### 커서 움직임 계산
1. **수신**: MQTT에서 mouse_x, mouse_y 수신
2. **감도 적용**: 델타 값 × 감도
3. **스무싱** (옵션): 최근 3개 값의 평균
4. **이동**: 커서 위치 += 조정된 델타
5. **경계 검사**: 캔버스 범위 내로 제한
6. **트레일 추가**: 최근 50개 위치 저장
7. **화면 업데이트**: Qt paintEvent()

## 캔버스 기능

### 격자 및 가이드
- **50픽셀 격자**: 위치 파악 용이
- **중심 십자선**: 캔버스 중심 표시 (회색 점선)
- **좌표 표시**: 왼쪽 위에 현재 커서 위치

### 커서 트레일
- **최대 50개 포인트** 저장
- **그라데이션 색상**: 오래된 경로일수록 투명
- **파란색 계열**: 구분하기 쉬운 색상
- **Show Trail** 체크박스로 on/off

### 마우스 클릭
캔버스를 클릭하면 커서가 클릭한 위치로 즉시 이동합니다.

## 예제 테스트 시나리오

### 시나리오 1: 기본 테스트
1. AirMouse 모드 활성화
2. 조이스틱을 **앞으로 기울이기** → 커서가 위로 이동
3. 조이스틱을 **왼쪽으로 기울이기** → 커서가 왼쪽으로 이동
4. 조이스틱을 **회전** → 스크롤 (데이터만 표시)

### 시나리오 2: 감도 테스트
1. Sensitivity를 **0.5x**로 설정
2. 조이스틱을 크게 기울여도 천천히 움직임
3. Sensitivity를 **3.0x**로 설정
4. 조이스틱을 조금만 기울여도 빠르게 움직임

### 시나리오 3: 스무싱 효과
1. **Enable Smoothing** 체크 해제
2. 조이스틱 움직임 → 커서가 떨리며 움직임
3. **Enable Smoothing** 체크
4. 조이스틱 움직임 → 커서가 부드럽게 움직임

### 시나리오 4: 트레일 효과
1. **Show Trail** 체크
2. 커서를 원형으로 크게 움직이기
3. 파란색 궤적이 남음
4. **Show Trail** 체크 해제
5. 궤적이 사라짐

### 시나리오 5: 정확도 테스트
1. 캔버스의 특정 격자 교차점을 목표로 설정
2. 조이스틱을 조심스럽게 움직여 목표 도달
3. Sensitivity를 조절하여 최적값 찾기

## 문제 해결

### 커서가 움직이지 않음
1. MQTT 연결 상태 확인 (상태: Connected)
2. **AirMouse Test** 탭이 선택되어 있는지 확인
3. **Enable AirMouse Mode** 버튼 클릭 했는지 확인
4. Log에서 메시지 수신 확인 (`Received from joystick/sensor/data`)
5. "Mouse X", "Mouse Y" 값이 변하는지 확인

### 커서가 너무 빠르거나 느림
- **Sensitivity 슬라이더** 조절
- 권장: 0.8x ~ 1.5x 범위

### 커서가 떨림
- **Enable Smoothing** 체크
- Sensitivity를 약간 낮추기

### 커서가 캔버스 밖으로 나감
- 자동으로 경계 내로 제한됨
- **Reset Cursor** 버튼으로 중앙 리셋

### ESP32 응답 없음
```bash
# ESP32 시리얼 모니터 확인
idf.py -p /dev/ttyUSB0 monitor

# MQTT 연결 상태 확인
# WiFi 연결 확인
# 브로커 IP 확인
```

## 다른 탭 기능

### Sensor Data 탭
- 조이스틱 원시 센서 데이터 (Accel X/Y/Z, Gyro X/Y/Z)
- 스마트 워치 심박수 데이터
- 센서 모드에서 데이터 확인

### WatchTower Commands 탭
- 운동 모드 선택 (T Pose, Squat, Pushup)
- Start/Stop Workout 명령
- WatchTower 시스템 응답 확인

## 코드 구조

### CursorCanvas 클래스 (`cursorcanvas.h/cpp`)
- **paintEvent()**: 캔버스, 격자, 커서, 트레일 그리기
- **moveCursor()**: 커서 이동 및 스무싱
- **resetCursor()**: 커서 리셋
- **setSensitivity()**: 감도 설정
- **setSmoothing()**: 스무싱 on/off
- **setShowTrail()**: 트레일 표시 on/off

### MainWindow 통합 (`mainwindow.h/cpp`)
- **setupCursorCanvas()**: 캔버스 초기화 및 레이아웃 설정
- **updateAirMouseData()**: MQTT 데이터를 커서 움직임으로 변환
- **on_resetCursorButton_clicked()**: 커서 리셋 버튼 슬롯
- **on_cursorSensitivitySlider_valueChanged()**: 감도 슬라이더 슬롯
- **on_cursorSmoothingCheckBox_toggled()**: 스무싱 체크박스 슬롯
- **on_showTrailCheckBox_toggled()**: 트레일 체크박스 슬롯

## 향후 개선 사항

### 추가 가능한 기능
- [ ] 클릭 이벤트 시각화 (버튼 클릭 시 애니메이션)
- [ ] 스크롤 시각화 (스크롤 바 또는 애니메이션)
- [ ] 커서 속도 미터 (현재 속도 표시)
- [ ] 히트맵 (자주 방문한 영역 표시)
- [ ] 녹화 및 재생 기능
- [ ] 커서 스타일 변경 옵션
- [ ] 다중 커서 지원 (여러 조이스틱)

## 팁

### 최적 사용 환경
- **Sensitivity**: 1.0x ~ 1.5x
- **Smoothing**: ON
- **Trail**: ON (시각적 피드백)
- **조명**: ESP32가 안정적으로 작동하는 환경

### 캘리브레이션
조이스틱이 가만히 있을 때도 커서가 움직이면:
1. **Calibrate** 버튼 클릭
2. 조이스틱을 평평한 곳에 놓고 5초 대기
3. ESP32가 영점을 재설정함

### 데모 시연
1. **Show Trail** 켜기
2. Sensitivity를 1.5x로 설정
3. 조이스틱으로 이름이나 도형 그리기
4. 파란색 트레일로 경로 확인!

## 관련 파일
- [cursorcanvas.h](cursorcanvas.h) - 커서 캔버스 헤더
- [cursorcanvas.cpp](cursorcanvas.cpp) - 커서 캔버스 구현
- [mainwindow.ui](mainwindow.ui) - UI 디자인 (탭 포함)
- [mainwindow.h](mainwindow.h) - 메인 윈도우 헤더
- [mainwindow.cpp](mainwindow.cpp) - 메인 윈도우 구현
- [README.md](README.md) - 전체 프로젝트 문서
