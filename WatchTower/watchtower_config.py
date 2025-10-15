"""
WatchTower 통합 시스템 설정 파일
사용자가 환경에 맞게 수정해야 하는 모든 설정값
"""


class WatchTowerConfig:
    """WatchTower 메인 설정"""

    # ============================================
    # AI 서버 설정 (수정 필요!)
    # ============================================
    AI_SERVER_HOST = '192.168.1.100'  # AI 서버 IP 주소
    AI_SERVER_PORT = 5000              # AI 서버 포트
    REQUEST_TIMEOUT = 5                # HTTP 요청 타임아웃 (초)

    # API 엔드포인트
    API_MODE_SELECT = '/api/mode/select'
    API_STREAM_FRAME = '/api/stream/frame'
    API_STREAM_STOP = '/api/stream/stop'
    API_HEALTH = '/api/health'
    API_STATUS = '/api/status'

    # ============================================
    # MQTT 브로커 설정 (WatchTower가 브로커 역할)
    # ============================================
    MQTT_BROKER_HOST = '10.10.16.111'  # MQTT 브로커 주소 (Jetson Nano의 IP)
    MQTT_BROKER_PORT = 1883            # MQTT 브로커 포트
    MQTT_KEEPALIVE = 60                # Keep-alive 시간 (초)
    MQTT_QOS = 1                       # QoS 레벨 (0, 1, 2)

    # MQTT 인증 (필요시 설정)
    MQTT_USERNAME = ""                 # 브로커 인증 사용자명 (옵션)
    MQTT_PASSWORD = ""                 # 브로커 인증 비밀번호 (옵션)

    # ============================================
    # MQTT 토픽 정의
    # ============================================
    # Qt Command topics (Qt -> WatchTower)
    TOPIC_QT_SELECT_MODE = "qt/command/select_mode"
    TOPIC_QT_START = "qt/command/start"
    TOPIC_QT_STOP = "qt/command/stop"

    # Qt Response topics (WatchTower -> Qt)
    TOPIC_QT_RESPONSE_MODE = "qt/response/mode_selected"
    TOPIC_QT_RESPONSE_ERROR = "qt/response/error"
    TOPIC_QT_RESPONSE_STATUS = "qt/response/status"
    TOPIC_QT_RESPONSE_ANALYSIS = "qt/response/analysis"
    TOPIC_QT_RESPONSE_FRAME = "qt/response/frame"
    TOPIC_QT_RESPONSE_JOYSTICK = "qt/response/joystick"
    TOPIC_QT_RESPONSE_WATCH = "qt/response/watch"
    TOPIC_QT_POSE_UPDATE = "qt/command/pose_index"
    TOPIC_QT_REQUEST_ANALYSIS = "qt/command/request_analysis"

    # Command topics (WatchTower -> Devices)
    TOPIC_CMD_JOYSTICK = "watchtower/command/joystick"
    TOPIC_CMD_WATCH = "watchtower/command/watch"

    # Sensor data topics (Devices -> WatchTower)
    TOPIC_JOYSTICK_DATA = "joystick/sensor/data"
    TOPIC_WATCH_HEARTRATE = "watch/sensor/heartrate"

    # Status topics (양방향)
    TOPIC_JOYSTICK_STATUS = "joystick/status"
    TOPIC_WATCH_STATUS = "watch/status"

    # ============================================
    # 카메라 설정 (수정 필요!)
    # ============================================
    CAMERA_ID = 0                      # USB 카메라 장치 ID (/dev/video0)
    CAMERA_WIDTH = 640                 # 프레임 너비
    CAMERA_HEIGHT = 480                # 프레임 높이
    CAMERA_FPS = 30                    # 카메라 FPS

    # ============================================
    # 스트리밍 설정
    # ============================================
    STREAM_FPS = 10                    # AI 서버로 전송할 FPS (너무 높으면 부하 증가)
    JPEG_QUALITY = 85                  # JPEG 압축 품질 (0-100)

    # ============================================
    # 디스플레이 설정
    # ============================================
    SHOW_PREVIEW = True                # 로컬 프리뷰 창 표시 여부
    WINDOW_NAME = 'WatchTower'         # 프리뷰 창 이름

    # ============================================
    # UART 통신 설정 (STM32 팬틸트 제어) (수정 필요!)
    # ============================================
    UART_PORT = '/dev/ttyUSB0'         # UART 포트 (Jetson: /dev/ttyUSB0 또는 /dev/ttyACM0)
    UART_BAUDRATE = 115200             # 통신 속도
    UART_TIMEOUT = 1.0                 # UART 타임아웃 (초)

    # ============================================
    # 팬틸트 서보 설정 (MG996R: -60~60도 범위)
    # ============================================
    PANTILT_ENABLED = True             # 팬틸트 제어 활성화 여부
    PANTILT_VERBOSE = False            # 팬틸트 UART 전송 로그 상세 출력
    PAN_MIN = -60                      # Pan 최소 각도
    PAN_MAX = 60                       # Pan 최대 각도
    PAN_CENTER = 0                     # Pan 중앙 각도
    TILT_MIN = -60                     # Tilt 최소 각도
    TILT_MAX = 60                      # Tilt 최대 각도
    TILT_CENTER = 0                    # Tilt 중앙 각도

    # ============================================
    # 추적 알고리즘 설정
    # ============================================
    TRACKING_SPEED = 10                # 각도 변화 속도 (도/프레임, 값이 클수록 빠름)
    TRACKING_MAX_DELTA = 15            # 한 프레임당 최대 각도 변화량 (도)
    TRACKING_SMOOTH_FRAMES = 3         # 스무딩을 위한 프레임 수
    TRACKING_DEAD_ZONE_X = 30          # X축 Dead Zone (픽셀, 중앙 근처는 움직이지 않음)
    TRACKING_DEAD_ZONE_Y = 30          # Y축 Dead Zone (픽셀)

    # ============================================
    # 센서 데이터 로깅 설정
    # ============================================
    LOG_SENSOR_DATA = True             # 센서 데이터 로깅 활성화
    LOG_FILE_PATH = './sensor_logs'    # 로그 파일 저장 경로
    LOG_MAX_SIZE_MB = 100              # 최대 로그 파일 크기 (MB)

    # ============================================
    # 운동 모드 설정
    # ============================================
    SUPPORTED_MODES = ['squat', 'pushup']  # 지원하는 운동 모드
    DEFAULT_MODE = 'squat'            # 기본 운동 모드

    # ============================================
    # 디바이스 타임아웃 설정
    # ============================================
    DEVICE_TIMEOUT = 10.0              # 디바이스 응답 대기 시간 (초)
    HEARTBEAT_INTERVAL = 5.0           # 디바이스 상태 확인 주기 (초)
