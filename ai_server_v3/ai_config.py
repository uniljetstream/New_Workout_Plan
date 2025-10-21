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
        # 카테고리 루틴 (3개)
        'bodyweight_routine',   # 맨몸 운동 루틴
        'kettlebell_routine',   # 케틀벨 운동 루틴
        'barbell_routine',      # 바벨 운동 루틴
        # 맨몸 운동 (2개)
        'squat',                # 스쿼트
        'lunge',                # 런지
        # 케틀벨 운동 (2개)
        'kettlebell_swing',     # 케틀벨 스윙
        'kettlebell_deadlift',  # 케틀벨 데드리프트
        # 바벨 운동 (5개)
        'barbell_row',          # 바벨 로우
        'barbell_upright_row',  # 바벨 업라이트 로우
        'barbell_overhead_press', # 바벨 오버헤드 프레스
        'barbell_biceps_curl',  # 바벨 바이셉스 컬
        'barbell_reverse_curl', # 바벨 리버스 컬
    ]

    # ============================================
    # 모드별 포즈 시퀀스 정의
    # ============================================
    MODE_POSES = {
        # ============================================
        # 카테고리 루틴 (3개)
        # ============================================
        'bodyweight_routine': [
            # Squat
            {'name': 'squat_stand', 'description': '[1/2] 스쿼트 - 준비 자세'},
            {'name': 'squat_down',  'description': '[1/2] 스쿼트 - 앉은 자세'},
            # Lunge (개별 런지와 동일한 단계)
            {
                'name': 'lunge_center',
                'description': '런지 준비 (중앙)',
                'duration': 0.5
            },
            {
                'name': 'lunge_left',
                'description': '왼쪽 런지',
                'duration': 2.0
            },
            {
                'name': 'lunge_center',
                'description': '런지 준비 (중앙)',
                'duration': 0.5
            },
            {
                'name': 'lunge_right',
                'description': '오른쪽 런지',
                'duration': 2.0
            },
        ],
        'kettlebell_routine': [
            # Kettlebell Swing
            {'name': 'swing_start', 'description': '[1/2] 케틀벨 스윙 - 시작'},
            {'name': 'swing_up',    'description': '[1/2] 케틀벨 스윙 - 들기'},
            # Kettlebell Deadlift
            {'name': 'deadlift_down','description': '[2/2] 케틀벨 데드리프트 - 바닥'},
            {'name': 'deadlift_up',  'description': '[2/2] 케틀벨 데드리프트 - 서기'},
        ],
        'barbell_routine': [
            # Barbell Row
            {'name': 'barbell_row_start', 'description': '[1/5] 바벨 로우 - 시작'},
            {'name': 'barbell_row_pull',  'description': '[1/5] 바벨 로우 - 당기기'},
            # Barbell Upright Row
            {'name': 'barbell_upright_start', 'description': '[2/5] 업라이트 로우 - 시작'},
            {'name': 'barbell_upright_mid',   'description': '[2/5] 업라이트 로우 - 당기기'},
            # Barbell Overhead Press
            {'name': 'overhead_start', 'description': '[3/5] 오버헤드 프레스 - 시작'},
            {'name': 'overhead_top',   'description': '[3/5] 오버헤드 프레스 - 완료'},
            # Barbell Biceps Curl
            {'name': 'curl_down', 'description': '[4/5] 바이셉스 컬 - 시작'},
            {'name': 'curl_up',   'description': '[4/5] 바이셉스 컬 - 완료'},
            # Barbell Reverse Curl
            {'name': 'reverse_curl_down', 'description': '[5/5] 리버스 컬 - 시작'},
            {'name': 'reverse_curl_up',   'description': '[5/5] 리버스 컬 - 완료'},
        ],

        # ============================================
        # 맨몸 운동 (개별)
        # ============================================
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
        'lunge': [
            {
                'name': 'lunge_center',
                'description': '런지 준비 (중앙)',
                'duration': 0.5
            },
            {
                'name': 'lunge_left',
                'description': '왼쪽 런지',
                'duration': 2.0
            },
            {
                'name': 'lunge_center',  # ⭐ 추가
                'description': '런지 준비 (중앙)',  # ⭐ 추가
                'duration': 0.5  # ⭐ 추가
            },  # ⭐ 추가
            {
                'name': 'lunge_right',
                'description': '오른쪽 런지',
                'duration': 2.0
            }
        ],

        # ============================================
        # 케틀벨 운동 (개별)
        # ============================================
        'kettlebell_swing': [
            {
                'name': 'swing_start',
                'description': '케틀벨 스윙 시작 자세 (케틀벨 아래로)',
                'duration': 1.0
            },
            {
                'name': 'swing_up',
                'description': '케틀벨 스윙 자세 (케틀벨 어깨 높이)',
                'duration': 1.0
            }
        ],
        'kettlebell_deadlift': [
            {
                'name': 'deadlift_down',
                'description': '케틀벨 데드리프트 시작 자세 (바닥)',
                'duration': 1.5
            },
            {
                'name': 'deadlift_up',
                'description': '케틀벨 데드리프트 완료 자세 (선 자세)',
                'duration': 1.5
            }
        ],

        # ============================================
        # 바벨 운동 (개별)
        # ============================================
        'barbell_row': [
            {
                'name': 'barbell_row_start',
                'description': '바벨 로우 시작 자세 (팔 아래로)',
                'duration': 1.0
            },
            {
                'name': 'barbell_row_pull',
                'description': '바벨 로우 당기기 (팔꿈치 뒤로)',
                'duration': 1.5
            },
        ],
        'barbell_upright_row': [
            {
                'name': 'barbell_upright_start',
                'description': '바벨 업라이트 로우 시작 (허벅지 앞)',
                'duration': 1.0
            },
            {
                'name': 'barbell_upright_mid',
                'description': '바벨 업라이트 로우 당기기 (가슴 높이)',
                'duration': 1.0
            },
        ],
        'barbell_overhead_press': [
            {
                'name': 'overhead_start',
                'description': '오버헤드 프레스 시작 (어깨 높이)',
                'duration': 1.0
            },
            {
                'name': 'overhead_top',
                'description': '오버헤드 프레스 완료 (머리 위)',
                'duration': 1.5
            }
        ],
        'barbell_biceps_curl': [
            {
                'name': 'curl_down',
                'description': '바이셉스 컬 시작 (팔 아래)',
                'duration': 1.0
            },
            {
                'name': 'curl_up',
                'description': '바이셉스 컬 완료 (팔 굽히기)',
                'duration': 1.5
            }
        ],
        'barbell_reverse_curl': [
            {
                'name': 'reverse_curl_down',
                'description': '리버스 컬 시작 (팔 아래)',
                'duration': 1.0
            },
            {
                'name': 'reverse_curl_up',
                'description': '리버스 컬 완료 (팔 굽히기)',
                'duration': 1.5
            }
        ]
    }

    # ============================================
    # Squat 판정 기준
    # ============================================
    SQUAT_STAND_HIP_KNEE_THRESHOLD = 160  # 선 자세: 엉덩이-무릎-발목 각도
    SQUAT_DOWN_KNEE_ANGLE_MIN = 50        # 앉은 자세: 무릎 최소 각도
    SQUAT_DOWN_KNEE_ANGLE_MAX = 140       # 앉은 자세: 무릎 최대 각도

    # ============================================
    # Kettlebell Swing 판정 기준
    # ============================================
    SWING_START_ELBOW_ANGLE_MIN = 150     # 시작 자세: 팔꿈치 각도
    SWING_START_HIP_ANGLE_MIN = 60        # 시작 자세: 엉덩이 굽힘 최소
    SWING_START_HIP_ANGLE_MAX = 130       # 시작 자세: 엉덩이 굽힘 최대
    SWING_UP_ELBOW_ANGLE_MIN = 150        # 스윙업: 팔꿈치 각도
    SWING_UP_HIP_ANGLE_MIN = 140          # 스윙업: 엉덩이 각도 (거의 펴짐)

    # ============================================
    # Kettlebell Deadlift 판정 기준
    # ============================================
    DEADLIFT_DOWN_HIP_ANGLE_MIN = 40      # 시작: 엉덩이 각도 최소
    DEADLIFT_DOWN_HIP_ANGLE_MAX = 100     # 시작: 엉덩이 각도 최대
    DEADLIFT_DOWN_KNEE_ANGLE_MIN = 100    # 시작: 무릎 각도 최소
    DEADLIFT_DOWN_KNEE_ANGLE_MAX = 150    # 시작: 무릎 각도 최대
    DEADLIFT_UP_HIP_ANGLE_MIN = 150       # 완료: 엉덩이 각도
    DEADLIFT_UP_KNEE_ANGLE_MIN = 150      # 완료: 무릎 각도

    # ============================================
    # Barbell Row 판정 기준
    # ============================================
    ROW_START_ELBOW_ANGLE_MIN = 120       # 시작: 팔꿈치 각도 (팔 펴짐)
    ROW_START_HIP_ANGLE_MIN = 60          # 시작: 엉덩이 굽힘
    ROW_START_HIP_ANGLE_MAX = 140         # 시작: 엉덩이 굽힘 최대

    ROW_PULL_ELBOW_ANGLE_MIN = 40         # 당기기: 팔꿈치 최소
    ROW_PULL_ELBOW_ANGLE_MAX = 120        # 당기기: 팔꿈치 최대

    # ============================================
    # Upright Row 판정 기준
    # ============================================
    UPRIGHT_START_ELBOW_ANGLE_MIN = 150   # 시작: 팔 펴짐
    UPRIGHT_MID_ELBOW_ANGLE_MIN = 40      # 당기기: 팔꿈치 각도 최소
    UPRIGHT_MID_ELBOW_ANGLE_MAX = 120     # 당기기: 팔꿈치 각도 최대

    # ============================================
    # Barbell Overhead Press 판정 기준
    # ============================================
    OVERHEAD_START_ELBOW_ANGLE_MIN = 20   # 시작: 팔꿈치 각도 (어깨 높이)
    OVERHEAD_START_ELBOW_ANGLE_MAX = 100  # 시작: 팔꿈치 각도
    OVERHEAD_TOP_ELBOW_ANGLE_MIN = 120    # 완료: 팔꿈치 각도 (팔 펴짐)

    # ============================================
    # Barbell Curl 판정 기준 - Biceps & Reverse 공통
    # ============================================
    CURL_DOWN_ELBOW_ANGLE_MIN = 160       # 시작: 팔꿈치 각도 (팔 펴짐)
    CURL_UP_ELBOW_ANGLE_MIN = 0          # 완료: 팔꿈치 각도
    CURL_UP_ELBOW_ANGLE_MAX = 50          # 완료: 팔꿈치 각도

    # ============================================
    # Forward Lunge 판정 기준 (측면)
    # ============================================
    LUNGE_CENTER_KNEE_ANGLE_MIN = 155     # 중앙: 무릎 각도
    LUNGE_CENTER_HIP_ANGLE_MIN = 155      # 중앙: 엉덩이 각도
    LUNGE_LEFT_FRONT_KNEE_MIN = 30        # 왼쪽 런지: 앞 무릎 최소
    LUNGE_LEFT_FRONT_KNEE_MAX = 140       # 왼쪽 런지: 앞 무릎 최대
    LUNGE_LEFT_BACK_KNEE_MIN = 30         # 왼쪽 런지: 뒤 무릎 최소
    LUNGE_LEFT_BACK_KNEE_MAX = 140        # 왼쪽 런지: 뒤 무릎 최대
    LUNGE_RIGHT_FRONT_KNEE_MIN = 30       # 오른쪽 런지: 앞 무릎 최소
    LUNGE_RIGHT_FRONT_KNEE_MAX = 140      # 오른쪽 런지: 앞 무릎 최대
    LUNGE_RIGHT_BACK_KNEE_MIN = 30        # 오른쪽 런지: 뒤 무릎 최소
    LUNGE_RIGHT_BACK_KNEE_MAX = 140       # 오른쪽 런지: 뒤 무릎 최대
    LUNGE_DOWN_KNEE_ANGLE_MIN = 30        # 사이드 런지: 굽힌 무릎 최소
    LUNGE_DOWN_KNEE_ANGLE_MAX = 140       # 사이드 런지: 굽힌 무릎 최대
    LUNGE_DOWN_STRAIGHT_KNEE_MIN = 150    # 사이드 런지: 펴야 하는 무릎 각도

    # ============================================
    # 디버그 설정
    # ============================================
    DEBUG = True                   # Flask 디버그 모드
    VERBOSE = False                # YOLO 출력 상세 모드
