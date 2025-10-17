"""
AI 서버 설정 파일
Flask API 서버 및 YOLO Pose 분석 설정
"""


class AIServerConfig:
    """AI 서버 설정"""

    # ============================================
    # Flask 서버 설정
    # ============================================
    HOST = '0.0.0.0'               # 모든 인터페이스에서 수신
    PORT = 5000                    # Flask 서버 포트

    # ============================================
    # YOLO Pose 모델 설정
    # ============================================
    MODEL_PATH = 'yolo11s-pose.pt'  # YOLO 모델 파일 경로

    # 키포인트 신뢰도 임계값(내부 각도 계산용) — 완화
    CONFIDENCE_THRESHOLD = 0.3

    # 추론 파라미터(1차)
    YOLO_CONF = 0.25               # 객체/키포인트 탐지 confidence
    YOLO_IOU = 0.5                 # NMS IoU
    YOLO_IMGSZ = 736               # 입력 해상도 (640보다 살짝 업샘플)

    # 추론 파라미터(폴백, 1차 실패 시 재시도)
    FALLBACK_CONF = 0.15
    FALLBACK_IMGSZ = 960

    # ============================================
    # 운동 모드 설정
    # ============================================
    SUPPORTED_MODES = [
        # 카테고리 루틴 (3개)
        'bodyweight_routine',   # 맨몸 운동 루틴
        'kettlebell_routine',   # 케틀벨 운동 루틴
        'barbell_routine',      # 바벨 운동 루틴
        # 맨몸 운동
        'squat',                # 스쿼트
        'pushup',               # 푸시업
        'lunge',                # 런지(측면 뷰)
        # 케틀벨 운동
        'kettlebell_swing',     # 케틀벨 스윙
        'kettlebell_deadlift',  # 케틀벨 데드리프트
        # 바벨 운동
        'barbell_row',          # 바벨 로우
        'barbell_upright_row',  # 바벨 업라이트 로우
        'barbell_overhead_press', # 바벨 오버헤드 프레스
        'barbell_biceps_curl',  # 바벨 바이셉스 컬
        'barbell_reverse_curl', # 바벨 리버스 컬
        # 선택: 플랭크가 있다면 유지
        'plank',
    ]

    # ============================================
    # 모드별 포즈 시퀀스 정의
    # ============================================
    MODE_POSES = {
        # ============================================
        # 카테고리 루틴 (3개)
        # ============================================
        'bodyweight_routine': [
            # Squat (Side view)
            {'name': 'squat_stand', 'description': '[1/3] Squat - Ready (Side view)', 'view': 'side'},
            {'name': 'squat_down',  'description': '[1/3] Squat - Down (Side view)',  'view': 'side'},
            # Pushup (Side view)
            {'name': 'pushup_up',   'description': '[2/3] Pushup - Up (Side view)',   'view': 'side'},
            {'name': 'pushup_down', 'description': '[2/3] Pushup - Down (Side view)', 'view': 'side'},
            # Lunge (Side view) — 시작자세를 확실히 측면으로 안내
            {'name': 'lunge_center','description': '[3/3] Lunge - Ready (Side view)', 'view': 'side'},
            {'name': 'lunge_left',  'description': '[3/3] Lunge - Left (Side view)',  'view': 'side'},
            {'name': 'lunge_right', 'description': '[3/3] Lunge - Right (Side view)', 'view': 'side'},
        ],

        'kettlebell_routine': [
            # Kettlebell Swing (Side view)
            {'name': 'swing_start', 'description': '[1/3] KB Swing - Start (Side view)', 'view': 'side'},
            {'name': 'swing_up',    'description': '[1/3] KB Swing - Up (Side view)',    'view': 'side'},
            # Kettlebell Deadlift (Side view)
            {'name': 'deadlift_down','description': '[2/3] KB Deadlift - Down (Side view)', 'view': 'side'},
            {'name': 'deadlift_up',  'description': '[2/3] KB Deadlift - Up (Side view)',   'view': 'side'},
            # (사이드 런지 제거됨)
        ],

        'barbell_routine': [
            # Barbell Row (Side view)
            {'name': 'barbell_row_start','description': '[1/5] Barbell Row - Start (Side view)', 'view': 'side'},
            {'name': 'barbell_row_pull', 'description': '[1/5] Barbell Row - Pull (Side view)',  'view': 'side'},
            # Barbell Upright Row (Front view)
            {'name': 'barbell_upright_start','description': '[2/5] Upright Row - Start (Front view)', 'view': 'front'},
            {'name': 'barbell_upright_top',  'description': '[2/5] Upright Row - Top (Front view)',   'view': 'front'},
            # Barbell Overhead Press (Front view)
            {'name': 'overhead_start','description': '[3/5] Overhead Press - Start (Front view)', 'view': 'front'},
            {'name': 'overhead_top',  'description': '[3/5] Overhead Press - Top (Front view)',   'view': 'front'},
            # Barbell Biceps Curl (Side view)
            {'name': 'curl_down', 'description': '[4/5] Biceps Curl - Down (Side view)', 'view': 'side'},
            {'name': 'curl_up',   'description': '[4/5] Biceps Curl - Up (Side view)',   'view': 'side'},
            # Barbell Reverse Curl (Side view)
            {'name': 'reverse_curl_down','description': '[5/5] Reverse Curl - Down (Side view)', 'view': 'side'},
            {'name': 'reverse_curl_up',  'description': '[5/5] Reverse Curl - Up (Side view)',   'view': 'side'},
        ],

        # ============================================
        # 맨몸 운동 (개별)
        # ============================================
        'squat': [
            {'name': 'squat_stand', 'description': '스쿼트 준비 자세 (선 자세)', 'view': 'side'},
            {'name': 'squat_down',  'description': '스쿼트 자세 (무릎 90도)',  'view': 'side'},
        ],
        'pushup': [
            {'name': 'pushup_up',   'description': '푸시업 준비 자세 (팔 펴기)', 'view': 'side'},
            {'name': 'pushup_down', 'description': '푸시업 자세 (팔 굽히기)',   'view': 'side'},
        ],
        'plank': [
            {'name': 'plank_knee',  'description': '플랭크 준비 (무릎 댄 자세)', 'view': 'side'},
            {'name': 'plank_hold',  'description': '플랭크 유지 (무릎 뗀 자세)', 'view': 'side'},
        ],
        'lunge': [
            # 런지 시작자세를 "측면 선자세"로 명확히 표기
            {'name': 'lunge_center','description': '런지 준비 (측면 선자세)', 'view': 'side'},
            {'name': 'lunge_left',  'description': '왼쪽 런지 (측면)',       'view': 'side'},
            {'name': 'lunge_right', 'description': '오른쪽 런지 (측면)',     'view': 'side'},
        ],

        # ============================================
        # 케틀벨 운동 (개별)
        # ============================================
        'kettlebell_swing': [
            {'name': 'swing_start','description': '케틀벨 스윙 시작 자세 (케틀벨 아래로)', 'view': 'side'},
            {'name': 'swing_up',   'description': '케틀벨 스윙 자세 (케틀벨 어깨 높이)',   'view': 'side'},
        ],
        'kettlebell_deadlift': [
            {'name': 'deadlift_down','description': '케틀벨 데드리프트 시작 자세 (바닥)', 'view': 'side'},
            {'name': 'deadlift_up',  'description': '케틀벨 데드리프트 완료 자세 (선 자세)', 'view': 'side'},
        ],

        # ============================================
        # 바벨 운동 (개별)
        # ============================================
        'barbell_row': [
            {'name': 'barbell_row_start','description': '바벨 로우 시작 자세 (팔 아래로)', 'view': 'side'},
            {'name': 'barbell_row_pull', 'description': '바벨 로우 당기기 (팔꿈치 뒤로)',   'view': 'side'},
        ],
        'barbell_upright_row': [
            {'name': 'barbell_upright_start','description': 'Upright Row - Start (Front view)', 'view': 'front'},
            {'name': 'barbell_upright_top',  'description': 'Upright Row - Top (Front view)',   'view': 'front'},
        ],
        'barbell_overhead_press': [
            {'name': 'overhead_start','description': 'Overhead Press - Start (Front view)', 'view': 'front'},
            {'name': 'overhead_top',  'description': 'Overhead Press - Top (Front view)',   'view': 'front'},
        ],
        'barbell_biceps_curl': [
            {'name': 'curl_down','description': '바이셉스 컬 시작 (팔 아래)',     'view': 'side'},
            {'name': 'curl_up',  'description': '바이셉스 컬 완료 (팔 굽히기)',   'view': 'side'},
        ],
        'barbell_reverse_curl': [
            {'name': 'reverse_curl_down','description': '리버스 컬 시작 (팔 아래)',   'view': 'side'},
            {'name': 'reverse_curl_up',  'description': '리버스 컬 완료 (팔 굽히기)', 'view': 'side'},
        ],
    }

    # ============================================
    # Squat 판정 기준
    # ============================================
    SQUAT_STAND_HIP_KNEE_THRESHOLD = 160
    SQUAT_DOWN_KNEE_ANGLE_MIN = 80
    SQUAT_DOWN_KNEE_ANGLE_MAX = 100

    # ============================================
    # Pushup 판정 기준 (완화)
    # ============================================
    PUSHUP_UP_ELBOW_ANGLE_MIN = 135
    PUSHUP_DOWN_ELBOW_ANGLE_MIN = 20
    PUSHUP_DOWN_ELBOW_ANGLE_MAX = 120

    # ============================================
    # Kettlebell Swing 판정 기준
    # ============================================
    SWING_START_ELBOW_ANGLE_MIN = 150
    SWING_UP_ELBOW_ANGLE_MIN = 150
    SWING_UP_SHOULDER_HEIGHT_MIN = 0.6

    # ============================================
    # Kettlebell Deadlift 판정 기준
    # ============================================
    DEADLIFT_DOWN_KNEE_ANGLE_MIN = 100
    DEADLIFT_DOWN_KNEE_ANGLE_MAX = 150
    DEADLIFT_UP_KNEE_ANGLE_MIN = 150

    # ============================================
    # Barbell Row 판정 기준
    # ============================================
    ROW_START_ELBOW_ANGLE_MIN = 120
    ROW_PULL_ELBOW_ANGLE_MIN = 40
    ROW_PULL_ELBOW_ANGLE_MAX = 120
    ROW_START_HIP_ANGLE_MIN = 60
    ROW_START_HIP_ANGLE_MAX = 140

    # ============================================
    # Upright Row 판정 기준
    # ============================================
    UPRIGHT_START_ELBOW_ANGLE_MIN = 150
    UPRIGHT_TOP_ELBOW_ANGLE_MIN = 30
    UPRIGHT_TOP_ELBOW_ANGLE_MAX = 120

    # ============================================
    # Barbell Overhead Press 판정 기준
    # ============================================
    OVERHEAD_START_ELBOW_ANGLE_MIN = 20
    OVERHEAD_START_ELBOW_ANGLE_MAX = 100
    OVERHEAD_TOP_ELBOW_ANGLE_MIN = 120

    # ============================================
    # Barbell Curl 판정 기준
    # ============================================
    CURL_DOWN_ELBOW_ANGLE_MIN = 160
    CURL_UP_ELBOW_ANGLE_MIN = 40
    CURL_UP_ELBOW_ANGLE_MAX = 70

    # ============================================
    # Forward Lunge 판정 기준 (측면)
    # ============================================
    LUNGE_CENTER_KNEE_ANGLE_MIN = 155
    LUNGE_CENTER_HIP_ANGLE_MIN = 155
    LUNGE_LEFT_FRONT_KNEE_MIN = 30
    LUNGE_LEFT_FRONT_KNEE_MAX = 140
    LUNGE_LEFT_BACK_KNEE_MIN = 30
    LUNGE_LEFT_BACK_KNEE_MAX = 140
    LUNGE_RIGHT_FRONT_KNEE_MIN = 30
    LUNGE_RIGHT_FRONT_KNEE_MAX = 140
    LUNGE_RIGHT_BACK_KNEE_MIN = 30
    LUNGE_RIGHT_BACK_KNEE_MAX = 140

    # ============================================
    # 디버그 설정
    # ============================================
    DEBUG = True
    VERBOSE = False
