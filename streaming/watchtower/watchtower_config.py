"""
WatchTower 클라이언트 설정 파일
AI 서버 연결 및 카메라 설정
"""


class WatchTowerConfig:
    """WatchTower 클라이언트 설정"""

    # ============================================
    # AI 서버 연결 설정
    # ============================================
    AI_SERVER_HOST = '192.168.1.100'  # AI 서버 IP 주소
    AI_SERVER_PORT = 5000              # AI 서버 포트

    # API 엔드포인트
    API_MODE_SELECT = '/api/mode/select'
    API_STREAM_FRAME = '/api/stream/frame'
    API_STREAM_STOP = '/api/stream/stop'
    API_STATUS = '/api/status'

    # ============================================
    # 카메라 설정
    # ============================================
    CAMERA_ID = 0                      # USB 카메라 장치 ID
    CAMERA_WIDTH = 640                 # 프레임 너비
    CAMERA_HEIGHT = 480                # 프레임 높이
    CAMERA_FPS = 30                    # 카메라 FPS

    # ============================================
    # 스트리밍 설정
    # ============================================
    STREAM_FPS = 10                    # AI 서버로 전송할 FPS (너무 높으면 부하 증가)
    JPEG_QUALITY = 85                  # JPEG 압축 품질 (0-100)

    # ============================================
    # HTTP 요청 설정
    # ============================================
    REQUEST_TIMEOUT = 5                # HTTP 요청 타임아웃 (초)
    MAX_RETRIES = 3                    # 재시도 횟수

    # ============================================
    # 디스플레이 설정
    # ============================================
    SHOW_PREVIEW = True                # 로컬 프리뷰 창 표시 여부
    WINDOW_NAME = 'WatchTower'         # 프리뷰 창 이름

    # ============================================
    # UART 통신 설정 (STM32 팬틸트 제어)
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
