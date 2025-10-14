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
        # 맨몸 운동 (4개)
        'squat',                # 스쿼트
        'pushup',               # 푸시업
        'plank',                # 플랭크
        'lunge',                # 런지
        # 케틀벨 운동 (5개)
        'kettlebell_swing',     # 케틀벨 스윙
        'kettlebell_deadlift',  # 케틀벨 데드리프트
        'side_lunge',           # 사이드 런지
        'bridge',               # 브릿지
        'knee_drive',           # 니 드라이브
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
        # 카테고리 루틴 (3개) - 모든 운동을 순차적으로 실행
        # ============================================
        'bodyweight_routine': [
            # Squat
            {
                'name': 'squat_stand',
                'description': '[맨몸 1/4] 스쿼트 준비 자세',
                'duration': 1.0
            },
            {
                'name': 'squat_down',
                'description': '[맨몸 1/4] 스쿼트 자세',
                'duration': 2.0
            },
            # Pushup
            {
                'name': 'pushup_up',
                'description': '[맨몸 2/4] 푸시업 준비 자세',
                'duration': 1.0
            },
            {
                'name': 'pushup_down',
                'description': '[맨몸 2/4] 푸시업 자세',
                'duration': 1.0
            },
            # Plank
            {
                'name': 'plank_knee',
                'description': '[맨몸 3/4] 플랭크 준비',
                'duration': 1.0
            },
            {
                'name': 'plank_hold',
                'description': '[맨몸 3/4] 플랭크 유지 (30초)',
                'duration': 30.0
            },
            # Lunge
            {
                'name': 'lunge_center',
                'description': '[맨몸 4/4] 런지 준비',
                'duration': 0.5
            },
            {
                'name': 'lunge_left',
                'description': '[맨몸 4/4] 왼쪽 런지',
                'duration': 2.0
            },
            {
                'name': 'lunge_right',
                'description': '[맨몸 4/4] 오른쪽 런지',
                'duration': 2.0
            }
        ],
        'kettlebell_routine': [
            # Kettlebell Swing
            {
                'name': 'swing_start',
                'description': '[케틀벨 1/5] 스윙 시작 자세',
                'duration': 1.0
            },
            {
                'name': 'swing_up',
                'description': '[케틀벨 1/5] 스윙 자세',
                'duration': 1.0
            },
            # Kettlebell Deadlift
            {
                'name': 'deadlift_down',
                'description': '[케틀벨 2/5] 데드리프트 시작 자세',
                'duration': 1.5
            },
            {
                'name': 'deadlift_up',
                'description': '[케틀벨 2/5] 데드리프트 완료 자세',
                'duration': 1.5
            },
            # Side Lunge
            {
                'name': 'lunge_center',
                'description': '[케틀벨 3/5] 사이드 런지 준비',
                'duration': 0.5
            },
            {
                'name': 'lunge_left',
                'description': '[케틀벨 3/5] 왼쪽 사이드 런지',
                'duration': 2.0
            },
            {
                'name': 'lunge_right',
                'description': '[케틀벨 3/5] 오른쪽 사이드 런지',
                'duration': 2.0
            },
            # Bridge
            {
                'name': 'bridge_down',
                'description': '[케틀벨 4/5] 브릿지 시작',
                'duration': 1.0
            },
            {
                'name': 'bridge_up',
                'description': '[케틀벨 4/5] 브릿지',
                'duration': 2.0
            },
            # Knee Drive
            {
                'name': 'knee_start',
                'description': '[케틀벨 5/5] 니 드라이브 준비',
                'duration': 0.5
            },
            {
                'name': 'knee_left',
                'description': '[케틀벨 5/5] 왼쪽 무릎 들기',
                'duration': 1.5
            },
            {
                'name': 'knee_right',
                'description': '[케틀벨 5/5] 오른쪽 무릎 들기',
                'duration': 1.5
            }
        ],
        'barbell_routine': [
            # Barbell Row
            {
                'name': 'barbell_row_start',
                'description': '[바벨 1/5] 로우 시작 자세',
                'duration': 1.0
            },
            {
                'name': 'barbell_row_pull',
                'description': '[바벨 1/5] 로우 당기기',
                'duration': 1.5
            },
            {
                'name': 'barbell_row_hold',
                'description': '[바벨 1/5] 로우 홀드',
                'duration': 1.0
            },
            # Barbell Upright Row
            {
                'name': 'barbell_upright_start',
                'description': '[바벨 2/5] 업라이트 로우 시작',
                'duration': 1.0
            },
            {
                'name': 'barbell_upright_mid',
                'description': '[바벨 2/5] 업라이트 로우 중간',
                'duration': 1.0
            },
            {
                'name': 'barbell_upright_top',
                'description': '[바벨 2/5] 업라이트 로우 최상단',
                'duration': 1.5
            },
            # Barbell Overhead Press
            {
                'name': 'overhead_start',
                'description': '[바벨 3/5] 오버헤드 프레스 시작',
                'duration': 1.0
            },
            {
                'name': 'overhead_mid',
                'description': '[바벨 3/5] 오버헤드 프레스 중간',
                'duration': 1.0
            },
            {
                'name': 'overhead_top',
                'description': '[바벨 3/5] 오버헤드 프레스 완료',
                'duration': 1.5
            },
            # Barbell Biceps Curl
            {
                'name': 'curl_down',
                'description': '[바벨 4/5] 바이셉스 컬 시작',
                'duration': 1.0
            },
            {
                'name': 'curl_up',
                'description': '[바벨 4/5] 바이셉스 컬 완료',
                'duration': 1.5
            },
            # Barbell Reverse Curl
            {
                'name': 'reverse_curl_down',
                'description': '[바벨 5/5] 리버스 컬 시작',
                'duration': 1.0
            },
            {
                'name': 'reverse_curl_up',
                'description': '[바벨 5/5] 리버스 컬 완료',
                'duration': 1.5
            }
        ],
        # ============================================
        # 맨몸 운동 (4개)
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
        ],
        'plank': [
            {
                'name': 'plank_knee',
                'description': '플랭크 준비 (무릎 댄 자세)',
                'duration': 1.0
            },
            {
                'name': 'plank_hold',
                'description': '플랭크 유지 (무릎 뗀 자세)',
                'duration': 30.0
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
                'name': 'lunge_right',
                'description': '오른쪽 런지',
                'duration': 2.0
            }
        ],
        # ============================================
        # 케틀벨 운동 (5개)
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
        'side_lunge': [
            {
                'name': 'lunge_center',
                'description': '사이드 런지 준비 (중앙)',
                'duration': 0.5
            },
            {
                'name': 'lunge_left',
                'description': '왼쪽 사이드 런지',
                'duration': 2.0
            },
            {
                'name': 'lunge_right',
                'description': '오른쪽 사이드 런지',
                'duration': 2.0
            }
        ],
        'bridge': [
            {
                'name': 'bridge_down',
                'description': '브릿지 시작 (바닥)',
                'duration': 1.0
            },
            {
                'name': 'bridge_up',
                'description': '브릿지 (엉덩이 들기)',
                'duration': 2.0
            }
        ],
        'knee_drive': [
            {
                'name': 'knee_start',
                'description': '니 드라이브 준비',
                'duration': 0.5
            },
            {
                'name': 'knee_left',
                'description': '왼쪽 무릎 들기',
                'duration': 1.5
            },
            {
                'name': 'knee_right',
                'description': '오른쪽 무릎 들기',
                'duration': 1.5
            }
        ],
        # ============================================
        # 바벨 운동 (5개)
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
            {
                'name': 'barbell_row_hold',
                'description': '바벨 로우 홀드 (상단 유지)',
                'duration': 1.0
            }
        ],
        'barbell_upright_row': [
            {
                'name': 'barbell_upright_start',
                'description': '바벨 업라이트 로우 시작 (허벅지 앞)',
                'duration': 1.0
            },
            {
                'name': 'barbell_upright_mid',
                'description': '바벨 업라이트 로우 중간 (가슴 높이)',
                'duration': 1.0
            },
            {
                'name': 'barbell_upright_top',
                'description': '바벨 업라이트 로우 최상단 (턱 높이)',
                'duration': 1.5
            }
        ],
        'barbell_overhead_press': [
            {
                'name': 'overhead_start',
                'description': '오버헤드 프레스 시작 (어깨 높이)',
                'duration': 1.0
            },
            {
                'name': 'overhead_mid',
                'description': '오버헤드 프레스 중간',
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
    SQUAT_DOWN_KNEE_ANGLE_MIN = 80        # 앉은 자세: 무릎 최소 각도
    SQUAT_DOWN_KNEE_ANGLE_MAX = 100       # 앉은 자세: 무릎 최대 각도

    # ============================================
    # Pushup 판정 기준
    # ============================================
    PUSHUP_UP_ELBOW_ANGLE_MIN = 160       # 팔 펴기: 팔꿈치 최소 각도
    PUSHUP_DOWN_ELBOW_ANGLE_MIN = 70      # 팔 굽히기: 팔꿈치 최소 각도
    PUSHUP_DOWN_ELBOW_ANGLE_MAX = 110     # 팔 굽히기: 팔꿈치 최대 각도

    # ============================================
    # Kettlebell Swing 판정 기준 (너그럽게)
    # ============================================
    SWING_START_ELBOW_ANGLE_MIN = 150     # 시작 자세: 팔꿈치 각도
    SWING_START_HIP_ANGLE_MIN = 110       # 시작 자세: 엉덩이 각도
    SWING_START_HIP_ANGLE_MAX = 170       # 시작 자세: 엉덩이 각도 최대
    
    SWING_UP_ELBOW_ANGLE_MIN = 150        # 스윙업: 팔꿈치 각도
    SWING_UP_SHOULDER_HEIGHT_MIN = 0.6    # 스윙업: 손목 높이 비율
    SWING_UP_HIP_ANGLE_MIN = 150          # 스윙업: 엉덩이 각도

    # ============================================
    # Kettlebell Deadlift 판정 기준 (너그럽게)
    # ============================================
    DEADLIFT_DOWN_HIP_ANGLE_MIN = 50      # 시작: 엉덩이 각도 최소
    DEADLIFT_DOWN_HIP_ANGLE_MAX = 110     # 시작: 엉덩이 각도 최대
    DEADLIFT_DOWN_KNEE_ANGLE_MIN = 100    # 시작: 무릎 각도 최소
    DEADLIFT_DOWN_KNEE_ANGLE_MAX = 150    # 시작: 무릎 각도 최대
    
    DEADLIFT_UP_HIP_ANGLE_MIN = 150       # 완료: 엉덩이 각도
    DEADLIFT_UP_KNEE_ANGLE_MIN = 150      # 완료: 무릎 각도

    # ============================================
    # Kettlebell Row 판정 기준 (너그럽게)
    # ============================================
    ROW_START_ELBOW_ANGLE_MIN = 150       # 시작: 팔꿈치 각도 (팔 펴짐)
    ROW_START_HIP_ANGLE_MIN = 60          # 시작: 엉덩이 굽힘
    ROW_START_HIP_ANGLE_MAX = 110         # 시작: 엉덩이 굽힘 최대
    
    ROW_PULL_ELBOW_ANGLE_MIN = 60         # 당기기: 팔꿈치 최소
    ROW_PULL_ELBOW_ANGLE_MAX = 100        # 당기기: 팔꿈치 최대
    
    ROW_HOLD_ELBOW_ANGLE_MIN = 50         # 홀드: 팔꿈치 최소
    ROW_HOLD_ELBOW_ANGLE_MAX = 90         # 홀드: 팔꿈치 최대

    # ============================================
    # Upright Row 판정 기준 (너그럽게)
    # ============================================
    UPRIGHT_START_ELBOW_ANGLE_MIN = 150   # 시작: 팔 펴짐
    UPRIGHT_START_WRIST_HIP_RATIO_MAX = 0.3  # 시작: 손목이 엉덩이 아래
    
    UPRIGHT_MID_ELBOW_ANGLE_MIN = 70      # 중간: 팔꿈치 각도
    UPRIGHT_MID_ELBOW_ANGLE_MAX = 110     # 중간: 팔꿈치 각도
    UPRIGHT_MID_WRIST_HEIGHT_MIN = 0.4    # 중간: 손목 높이 (어깨 대비)
    UPRIGHT_MID_WRIST_HEIGHT_MAX = 0.7    # 중간: 손목 높이 최대
    
    UPRIGHT_TOP_ELBOW_ANGLE_MIN = 60      # 최상단: 팔꿈치 각도
    UPRIGHT_TOP_ELBOW_ANGLE_MAX = 100     # 최상단: 팔꿈치 각도
    UPRIGHT_TOP_WRIST_HEIGHT_MIN = 0.8    # 최상단: 손목이 어깨 높이 이상

    # ============================================
    # Barbell Overhead Press 판정 기준 (너그럽게)
    # ============================================
    OVERHEAD_START_ELBOW_ANGLE_MIN = 60   # 시작: 팔꿈치 각도 (어깨 높이)
    OVERHEAD_START_ELBOW_ANGLE_MAX = 100  # 시작: 팔꿈치 각도
    
    OVERHEAD_MID_ELBOW_ANGLE_MIN = 100    # 중간: 팔꿈치 각도
    OVERHEAD_MID_ELBOW_ANGLE_MAX = 140    # 중간: 팔꿈치 각도
    
    OVERHEAD_TOP_ELBOW_ANGLE_MIN = 160    # 완료: 팔꿈치 각도 (팔 펴짐)
    OVERHEAD_TOP_WRIST_HEIGHT_MIN = 0.3   # 완료: 손목이 머리 위

    # ============================================
    # Barbell Curl 판정 기준 (너그럽게) - Biceps & Reverse 공통
    # ============================================
    CURL_DOWN_ELBOW_ANGLE_MIN = 160       # 시작: 팔꿈치 각도 (팔 펴짐)
    CURL_UP_ELBOW_ANGLE_MIN = 40          # 완료: 팔꿈치 각도
    CURL_UP_ELBOW_ANGLE_MAX = 70          # 완료: 팔꿈치 각도

    # ============================================
    # Side Lunge 판정 기준 (너그럽게)
    # ============================================
    LUNGE_CENTER_KNEE_ANGLE_MIN = 155     # 중앙: 무릎 각도
    LUNGE_CENTER_HIP_ANGLE_MIN = 155      # 중앙: 엉덩이 각도
    
    LUNGE_DOWN_KNEE_ANGLE_MIN = 70        # 굽힌 무릎 최소
    LUNGE_DOWN_KNEE_ANGLE_MAX = 110       # 굽힌 무릎 최대
    LUNGE_DOWN_STRAIGHT_KNEE_MIN = 150    # 펴진 무릎

    # ============================================
    # Bridge 판정 기준 (너그럽게)
    # ============================================
    BRIDGE_DOWN_HIP_ANGLE_MIN = 40        # 시작: 엉덩이 바닥
    BRIDGE_DOWN_HIP_ANGLE_MAX = 90        # 시작: 엉덩이 바닥
    
    BRIDGE_UP_HIP_ANGLE_MIN = 150         # 완료: 엉덩이 각도
    BRIDGE_UP_KNEE_ANGLE_MIN = 80         # 완료: 무릎 각도
    BRIDGE_UP_KNEE_ANGLE_MAX = 110        # 완료: 무릎 각도

    # ============================================
    # Knee Drive 판정 기준 (너그럽게)
    # ============================================
    KNEE_START_HIP_ANGLE_MIN = 155        # 시작: 엉덩이 각도
    KNEE_START_KNEE_ANGLE_MIN = 155       # 시작: 무릎 각도
    
    KNEE_LIFT_HIP_ANGLE_MIN = 50          # 들기: 엉덩이 각도
    KNEE_LIFT_HIP_ANGLE_MAX = 90          # 들기: 엉덩이 각도
    KNEE_LIFT_KNEE_ANGLE_MIN = 50         # 들기: 무릎 각도
    KNEE_LIFT_KNEE_ANGLE_MAX = 90         # 들기: 무릎 각도

    # ============================================
    # 디버그 설정
    # ============================================
    DEBUG = True                   # Flask 디버그 모드
    VERBOSE = False                # YOLO 출력 상세 모드