"""
MQTT 테스트 전용 설정 파일
조이스틱(ESP32)과 Watch(ESP32) MQTT 통신 테스트를 위한 설정
"""


class MQTTTestConfig:
    """MQTT 테스트 전용 설정"""

    # ============================================
    # MQTT 브로커 설정
    # ============================================
    MQTT_BROKER_HOST = 'localhost'     # MQTT 브로커 주소 (Jetson Nano에서 실행시 localhost)
    MQTT_BROKER_PORT = 1883            # MQTT 브로커 포트
    MQTT_KEEPALIVE = 60                # Keep-alive 시간 (초)
    MQTT_QOS = 1                       # QoS 레벨 (0, 1, 2)

    # MQTT 인증 (필요시 설정)
    MQTT_USERNAME = ""                 # 브로커 인증 사용자명 (옵션)
    MQTT_PASSWORD = ""                 # 브로커 인증 비밀번호 (옵션)

    # ============================================
    # MQTT 토픽 정의
    # ============================================
    # Command topics (WatchTower -> Devices)
    TOPIC_CMD_JOYSTICK = "watchtower/command/joystick"
    TOPIC_CMD_WATCH = "watchtower/command/watch"

    # Sensor data topics (Devices -> WatchTower)
    TOPIC_JOYSTICK_DATA = "joystick/sensor/data"
    TOPIC_WATCH_HEARTRATE = "watch/sensor/heartrate"

    # Status topics (Devices -> WatchTower)
    TOPIC_JOYSTICK_STATUS = "joystick/status"
    TOPIC_WATCH_STATUS = "watch/status"

    # ============================================
    # 운동 모드 설정
    # ============================================
    SUPPORTED_MODES = ['t_pose', 'squat', 'pushup']  # 지원하는 운동 모드
    DEFAULT_MODE = 't_pose'            # 기본 운동 모드

    # ============================================
    # 디스플레이 설정
    # ============================================
    SHOW_TIMESTAMP = True              # 타임스탬프 출력 여부
    SHOW_RAW_JSON = False              # Raw JSON 메시지 출력 여부 (디버깅용)
    SENSOR_DATA_PRECISION = 2          # 센서 데이터 소수점 자릿수

    # ============================================
    # 통계 설정
    # ============================================
    ENABLE_STATISTICS = True           # 통계 수집 활성화
    STATS_UPDATE_INTERVAL = 10.0       # 통계 자동 출력 주기 (초, 0이면 수동만)
