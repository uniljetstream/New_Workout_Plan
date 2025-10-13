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
    CONFIDENCE_THRESHOLD = 0.5      # 키포인트 신뢰도 임계값

    # ============================================
    # 운동 모드 설정
    # ============================================
    SUPPORTED_MODES = [
        'squat',       # 스쿼트
        'pushup',      # 푸시업
    ]

    # ============================================
    # 모드별 포즈 시퀀스 정의
    # ============================================
    MODE_POSES = {
        'squat': [
            {
                'name': 'squat_stand',
                'description': '스쿼트 준비 자세 (선 자세)',
                'duration': 1.0
            },
            {
                'name': 'squat_down',
                'description': '스쿼트 자세 (무릎 90도)',
                'duration': 2.0
            }
        ],
        'pushup': [
            {
                'name': 'pushup_up',
                'description': '푸시업 준비 자세 (팔 펴기)',
                'duration': 1.0
            },
            {
                'name': 'pushup_down',
                'description': '푸시업 자세 (팔 굽히기)',
                'duration': 1.0
            }
        ]
    }

    # ============================================
    # Squat 판정 기준
    # ============================================
    SQUAT_STAND_HIP_KNEE_THRESHOLD = 160  # 선 자세: 엉덩이-무릎-발목 각도
    SQUAT_DOWN_KNEE_ANGLE_MIN = 80        # 앉은 자세: 무릎 최소 각도
    SQUAT_DOWN_KNEE_ANGLE_MAX = 100       # 앉은 자세: 무릎 최대 각도

    # ============================================
    # Pushup 판정 기준
    # ============================================
    PUSHUP_UP_ELBOW_ANGLE_MIN = 160       # 팔 펴기: 팔꿈치 최소 각도
    PUSHUP_DOWN_ELBOW_ANGLE_MIN = 70      # 팔 굽히기: 팔꿈치 최소 각도
    PUSHUP_DOWN_ELBOW_ANGLE_MAX = 110     # 팔 굽히기: 팔꿈치 최대 각도
    PUSHUP_BODY_ALIGNMENT_THRESHOLD = 20  # 몸통 정렬: 어깨-엉덩이-발목 직선 허용 오차 (도)

    # ============================================
    # 디버그 설정
    # ============================================
    DEBUG = True                   # Flask 디버그 모드
    VERBOSE = False                # YOLO 출력 상세 모드
