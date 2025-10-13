# WatchTower Qt 테스트 클라이언트

이 간단한 Qt 위젯 애플리케이션은 MQTT를 통해 WatchTower 시스템을 테스트하기 위한 도구입니다.  
운동 모드를 선택해 `qt/command/select_mode` 토픽으로 전송하고, 정지 명령을 `qt/command/stop`으로 발행합니다.  
또한 `qt/response/#`, `joystick/#`, `watch/#` 토픽을 구독하여 들어오는 메시지를 로그로 보여줍니다.

## 빌드 방법

Qt 5.12 이상(또는 Qt 6)과 Qt MQTT 모듈이 설치되어 있어야 합니다.

```bash
cd qt_client
cmake -B build -S . -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build build
```

빌드가 완료되면 `build/watchtower_qt_client` 실행 파일을 실행하세요.

```bash
./build/watchtower_qt_client
```

### 설정 변경

실행 파일과 같은 디렉터리에 있는 `config.json`에서 브로커 주소, 포트, MQTT 토픽, 운동 모드 목록을 수정할 수 있습니다.  
값을 바꾼 뒤 앱을 다시 실행하면 변경 사항이 적용됩니다.

## 사용 방법

1. 브로커 주소와 포트를 입력하고 `연결` 버튼을 누릅니다.
2. 연결되면 드롭다운에서 운동 모드를 고르고 `모드 선택 전송`을 눌러 JSON 메시지를 발행합니다.
3. 운동 종료를 테스트하려면 `정지 전송` 버튼을 사용합니다.
4. 하단 로그 창에서 WatchTower가 발행하는 응답과 조이스틱/워치 센서 데이터를 확인할 수 있습니다.
