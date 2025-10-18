"""
YOLO Pose 분석 모듈
운동 자세를 분석하고 피드백을 제공하는 클래스
"""

import numpy as np
from ultralytics import YOLO
from ai_config import AIServerConfig


class PoseAnalyzer:
    """YOLO Pose 기반 자세 분석기"""

    def __init__(self, model_path=None):
        """
        초기화

        Args:
            model_path: YOLO 모델 파일 경로 (None이면 config에서 가져옴)
        """
        model_path = model_path or AIServerConfig.MODEL_PATH
        self.model = YOLO(model_path)
        self.current_mode = None
        self.current_pose_index = 0  # 현재 포즈 인덱스

    def set_mode(self, mode):
        """
        운동 모드 설정

        Args:
            mode: 운동 모드 ('squat', 'pushup', etc.)

        Returns:
            bool: 설정 성공 여부
        """
        if mode not in AIServerConfig.SUPPORTED_MODES:
            return False
        self.current_mode = mode
        self.current_pose_index = 0  # 모드 변경 시 포즈 인덱스 초기화
        return True

    def set_pose_index(self, pose_index):
        """
        현재 포즈 인덱스 설정

        Args:
            pose_index: 포즈 인덱스 (0부터 시작)

        Returns:
            bool: 설정 성공 여부
        """
        if self.current_mode is None:
            return False

        poses = AIServerConfig.MODE_POSES.get(self.current_mode, [])
        if pose_index < 0 or pose_index >= len(poses):
            return False

        self.current_pose_index = pose_index
        return True

    def get_current_pose_info(self):
        """
        현재 포즈 정보 가져오기

        Returns:
            dict: 포즈 정보 또는 None
        """
        if self.current_mode is None:
            return None

        poses = AIServerConfig.MODE_POSES.get(self.current_mode, [])
        if self.current_pose_index >= len(poses):
            return None

        return poses[self.current_pose_index]

    def analyze_frame(self, frame):
        """
        프레임 분석 (현재 모드 및 포즈에 따라)

        Args:
            frame: OpenCV 이미지 (numpy 배열)

        Returns:
            dict: 분석 결과
        """
        if self.current_mode is None:
            return {
                'status': 'error',
                'message': 'No mode selected'
            }

        # 현재 포즈 정보 가져오기
        pose_info = self.get_current_pose_info()
        if pose_info is None:
            return {
                'status': 'error',
                'message': 'Invalid pose index'
            }

        # YOLO Pose 추론
        results = self.model(frame, verbose=AIServerConfig.VERBOSE)

        if results[0].keypoints is None or len(results[0].keypoints) == 0:
            return {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': '사람이 감지되지 않았습니다',
                'current_pose': pose_info['name'],
                'pose_description': pose_info['description']
            }

        # 키포인트 추출
        keypoints = results[0].keypoints[0]
        xy = keypoints.xy.cpu().numpy()[0]  # (17, 2)
        conf = keypoints.conf.cpu().numpy()[0]  # (17,)

        # 바운딩 박스 추출 (추적용)
        boxes = results[0].boxes
        bbox = None
        if boxes is not None and len(boxes) > 0:
            box = boxes[0].xyxy.cpu().numpy()[0]  # [x1, y1, x2, y2]
            bbox = [float(x) for x in box]

        # 포즈 이름에 따라 분석
        pose_name = pose_info['name']

        # 분석 함수 매핑
        analysis_map = {
            # 맨몸 운동
            'squat_stand': self._analyze_squat_stand,
            'squat_down': self._analyze_squat_down,
            'pushup_up': self._analyze_pushup_up,
            'pushup_down': self._analyze_pushup_down,
            'plank_knee': self._analyze_plank_knee,
            'plank_hold': self._analyze_plank_hold,
            'lunge_center': self._analyze_lunge_center,
            'lunge_left': self._analyze_lunge_left_forward,
            'lunge_right': self._analyze_lunge_right_forward,
            # 케틀벨 운동
            'swing_start': self._analyze_swing_start,
            'swing_up': self._analyze_swing_up,
            'deadlift_down': self._analyze_deadlift_down,
            'deadlift_up': self._analyze_deadlift_up,
            # 바벨 운동 (row 함수 재사용)
            'barbell_row_start': self._analyze_row_start,
            'barbell_row_pull': self._analyze_row_pull,
            'barbell_row_hold': self._analyze_row_hold,
            'barbell_upright_start': self._analyze_upright_start,
            'barbell_upright_mid': self._analyze_upright_mid,
            'barbell_upright_top': self._analyze_upright_top,
            'overhead_start': self._analyze_overhead_start,
            'overhead_mid': self._analyze_overhead_mid,
            'overhead_top': self._analyze_overhead_top,
            'curl_down': self._analyze_curl_down,
            'curl_up': self._analyze_curl_up,
            'reverse_curl_down': self._analyze_curl_down,  # 동일 함수 사용
            'reverse_curl_up': self._analyze_curl_up,      # 동일 함수 사용
            # 기타 운동
            'bridge_down': self._analyze_bridge_down,
            'bridge_up': self._analyze_bridge_up,
            'knee_start': self._analyze_knee_start,
            'knee_left': self._analyze_knee_left,
            'knee_right': self._analyze_knee_right,
        }

        if pose_name in analysis_map:
            result = analysis_map[pose_name](xy, conf, bbox)
        else:
            return {
                'status': 'error',
                'message': f'Pose {pose_name} not implemented yet'
            }

        # 포즈 정보 추가
        result['current_pose'] = pose_info['name']
        result['pose_description'] = pose_info['description']

        # Attach keypoint info for downstream visualization
        result['keypoints'] = {
            'xy': xy.tolist(),
            'conf': conf.tolist()
        }

        # Convert numpy types to native Python types for JSON serialization
        if 'is_correct' in result:
            result['is_correct'] = bool(result['is_correct'])
        if 'score' in result:
            result['score'] = int(result['score']) if isinstance(result['score'], (np.integer, np.floating)) else result['score']
        if 'tracking' in result:
            if 'center_x' in result['tracking']:
                result['tracking']['center_x'] = float(result['tracking']['center_x'])
            if 'center_y' in result['tracking']:
                result['tracking']['center_y'] = float(result['tracking']['center_y'])
            if 'bbox' in result['tracking']:
                result['tracking']['bbox'] = [float(x) for x in result['tracking']['bbox']]

        return result

    def _analyze_squat_stand(self, xy, conf, bbox=None):
        """스쿼트 준비 자세 (선 자세) 분석"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_leg_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_leg_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_leg_angle, right_leg_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                stand_threshold = AIServerConfig.SQUAT_STAND_HIP_KNEE_THRESHOLD
                left_leg_ok = left_leg_angle > stand_threshold
                right_leg_ok = right_leg_angle > stand_threshold
                score = (50 if left_leg_ok else 0) + (50 if right_leg_ok else 0)
                feedback = []
                if not left_leg_ok: feedback.append(f"왼쪽 다리 펴기 ({left_leg_angle:.0f}°)")
                if not right_leg_ok: feedback.append(f"오른쪽 다리 펴기 ({right_leg_angle:.0f}°)")
                is_correct = score == 100
                message = "준비 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_squat_down(self, xy, conf, bbox=None):
        """스쿼트 자세 (무릎 90도) 분석"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.SQUAT_DOWN_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.SQUAT_DOWN_KNEE_ANGLE_MAX
                left_knee_ok = min_angle <= left_knee_angle <= max_angle
                right_knee_ok = min_angle <= right_knee_angle <= max_angle
                score = (50 if left_knee_ok else 0) + (50 if right_knee_ok else 0)
                feedback = []
                if not left_knee_ok:
                    feedback.append(f"왼쪽 무릎 {'너무 깊음' if left_knee_angle < min_angle else '더 굽히기'} ({left_knee_angle:.0f}°)")
                if not right_knee_ok:
                    feedback.append(f"오른쪽 무릎 {'너무 깊음' if right_knee_angle < min_angle else '더 굽히기'} ({right_knee_angle:.0f}°)")
                is_correct = score == 100
                message = "완벽한 스쿼트 자세!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_pushup_up(self, xy, conf, bbox=None):
        """푸시업 준비 자세 (팔 펴기) 분석"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 전체적으로 보이도록 카메라 앞에 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.PUSHUP_UP_ELBOW_ANGLE_MIN
                left_arm_ok = left_elbow_angle > min_angle
                right_arm_ok = right_elbow_angle > min_angle
                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok: feedback.append(f"왼팔 더 펴기 ({left_elbow_angle:.0f}°)")
                if not right_arm_ok: feedback.append(f"오른팔 더 펴기 ({right_elbow_angle:.0f}°)")
                is_correct = score == 100
                message = "준비 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_pushup_down(self, xy, conf, bbox=None):
        """푸시업 자세 (팔 굽히기) 분석"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 전체적으로 보이도록 카메라 앞에 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.PUSHUP_DOWN_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.PUSHUP_DOWN_ELBOW_ANGLE_MAX
                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle
                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok:
                    feedback.append(f"왼팔 {'너무 깊이 굽힘' if left_elbow_angle < min_angle else '더 굽히기'} ({left_elbow_angle:.0f}°)")
                if not right_arm_ok:
                    feedback.append(f"오른팔 {'너무 깊이 굽힘' if right_elbow_angle < min_angle else '더 굽히기'} ({right_elbow_angle:.0f}°)")
                is_correct = score == 100
                message = "완벽한 푸시업 자세!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_swing_start(self, xy, conf, bbox=None):
        """케틀벨 스윙 시작 자세"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None

        points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist, left_hip, right_hip, left_knee, right_knee]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                elbow_min = AIServerConfig.SWING_START_ELBOW_ANGLE_MIN
                hip_min = AIServerConfig.SWING_START_HIP_ANGLE_MIN
                hip_max = AIServerConfig.SWING_START_HIP_ANGLE_MAX

                left_arm_ok = left_elbow_angle > elbow_min
                right_arm_ok = right_elbow_angle > elbow_min
                left_hip_ok = hip_min <= left_hip_angle <= hip_max
                right_hip_ok = hip_min <= right_hip_angle <= hip_max

                score = sum([left_arm_ok, right_arm_ok, left_hip_ok, right_hip_ok]) * 25
                feedback = []
                if not left_arm_ok: feedback.append(f"왼팔 펴기")
                if not right_arm_ok: feedback.append(f"오른팔 펴기")
                if not left_hip_ok or not right_hip_ok: feedback.append(f"엉덩이 자세 조정")
                is_correct = score == 100
                message = "시작 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_swing_up(self, xy, conf, bbox=None):
        """케틀벨 스윙업 자세"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None

        points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist, left_hip, right_hip, left_knee, right_knee]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            
            avg_shoulder_y = (left_shoulder[1] + right_shoulder[1]) / 2
            avg_wrist_y = (left_wrist[1] + right_wrist[1]) / 2
            wrist_height_ratio = avg_shoulder_y / (avg_wrist_y + 1e-6)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                elbow_min = AIServerConfig.SWING_UP_ELBOW_ANGLE_MIN
                hip_min = AIServerConfig.SWING_UP_HIP_ANGLE_MIN
                height_min = AIServerConfig.SWING_UP_SHOULDER_HEIGHT_MIN

                left_arm_ok = left_elbow_angle > elbow_min
                right_arm_ok = right_elbow_angle > elbow_min
                left_hip_ok = left_hip_angle > hip_min
                right_hip_ok = right_hip_angle > hip_min
                height_ok = wrist_height_ratio > height_min

                score = sum([left_arm_ok, right_arm_ok, left_hip_ok, right_hip_ok, height_ok]) * 20
                feedback = []
                if not left_arm_ok or not right_arm_ok: feedback.append(f"팔 펴기")
                if not left_hip_ok or not right_hip_ok: feedback.append(f"엉덩이 펴기")
                if not height_ok: feedback.append(f"케틀벨 더 높이 올리기")
                is_correct = score == 100
                message = "완벽한 스윙 자세!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_deadlift_down(self, xy, conf, bbox=None):
        """케틀벨 데드리프트 시작 자세"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_shoulder, right_shoulder, left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_hip_angle, right_hip_angle, left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                hip_min = AIServerConfig.DEADLIFT_DOWN_HIP_ANGLE_MIN
                hip_max = AIServerConfig.DEADLIFT_DOWN_HIP_ANGLE_MAX
                knee_min = AIServerConfig.DEADLIFT_DOWN_KNEE_ANGLE_MIN
                knee_max = AIServerConfig.DEADLIFT_DOWN_KNEE_ANGLE_MAX

                left_hip_ok = hip_min <= left_hip_angle <= hip_max
                right_hip_ok = hip_min <= right_hip_angle <= hip_max
                left_knee_ok = knee_min <= left_knee_angle <= knee_max
                right_knee_ok = knee_min <= right_knee_angle <= knee_max

                score = sum([left_hip_ok, right_hip_ok, left_knee_ok, right_knee_ok]) * 25
                feedback = []
                if not left_hip_ok or not right_hip_ok: feedback.append(f"엉덩이 자세 조정")
                if not left_knee_ok or not right_knee_ok: feedback.append(f"무릎 자세 조정")
                is_correct = score == 100
                message = "시작 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_deadlift_up(self, xy, conf, bbox=None):
        """케틀벨 데드리프트 완료 자세"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_shoulder, right_shoulder, left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_hip_angle, right_hip_angle, left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                hip_min = AIServerConfig.DEADLIFT_UP_HIP_ANGLE_MIN
                knee_min = AIServerConfig.DEADLIFT_UP_KNEE_ANGLE_MIN

                left_hip_ok = left_hip_angle > hip_min
                right_hip_ok = right_hip_angle > hip_min
                left_knee_ok = left_knee_angle > knee_min
                right_knee_ok = right_knee_angle > knee_min

                score = sum([left_hip_ok, right_hip_ok, left_knee_ok, right_knee_ok]) * 25
                feedback = []
                if not left_hip_ok or not right_hip_ok: feedback.append(f"엉덩이 펴기")
                if not left_knee_ok or not right_knee_ok: feedback.append(f"다리 펴기")
                is_correct = score == 100
                message = "완벽한 데드리프트 자세!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_row_start(self, xy, conf, bbox=None):
        """바벨/케틀벨 로우 시작 자세"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None

        points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist, left_hip, right_hip, left_knee, right_knee]

        if any(p is None for p in points):
            missing_parts = []
            if not (left_shoulder and right_shoulder): missing_parts.append("어깨")
            if not (left_elbow and right_elbow and left_wrist and right_wrist): missing_parts.append("팔")
            if not (left_hip and right_hip): missing_parts.append("엉덩이")
            if not (left_knee and right_knee): missing_parts.append("무릎")
            message = f"{', '.join(missing_parts) if missing_parts else '신체'}를 카메라에 보이도록 위치 조정 (측면)" if missing_parts else "측면으로 카메라 앞에 서주세요"
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': message}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '측면 각도로 위치 조정'}
            else:
                elbow_min = AIServerConfig.ROW_START_ELBOW_ANGLE_MIN
                hip_min = AIServerConfig.ROW_START_HIP_ANGLE_MIN
                hip_max = AIServerConfig.ROW_START_HIP_ANGLE_MAX

                left_arm_ok = left_elbow_angle > elbow_min
                right_arm_ok = right_elbow_angle > elbow_min
                left_hip_ok = hip_min <= left_hip_angle <= hip_max
                right_hip_ok = hip_min <= right_hip_angle <= hip_max

                score = sum([left_arm_ok, right_arm_ok, left_hip_ok, right_hip_ok]) * 25
                feedback = []
                if not left_arm_ok or not right_arm_ok:
                    avg_elbow = (left_elbow_angle + right_elbow_angle) / 2
                    feedback.append(f"팔 펴기 (현재: {avg_elbow:.0f}도, 목표: >{elbow_min}도)")
                if not left_hip_ok or not right_hip_ok:
                    avg_hip = (left_hip_angle + right_hip_angle) / 2
                    if avg_hip < hip_min:
                        feedback.append(f"상체를 더 숙이기 (현재: {avg_hip:.0f}도, 목표: {hip_min}-{hip_max}도)")
                    else:
                        feedback.append(f"상체를 덜 숙이기 (현재: {avg_hip:.0f}도, 목표: {hip_min}-{hip_max}도)")
                is_correct = score == 100
                message = "시작 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_row_pull(self, xy, conf, bbox=None):
        """바벨/케틀벨 로우 당기기"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            missing_parts = []
            if not (left_shoulder and right_shoulder): missing_parts.append("어깨")
            if not (left_elbow and right_elbow and left_wrist and right_wrist): missing_parts.append("팔")
            message = f"{', '.join(missing_parts) if missing_parts else '상체'}를 카메라에 보이도록 위치 조정 (측면)"
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': message}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '측면 각도로 위치 조정'}
            else:
                min_angle = AIServerConfig.ROW_PULL_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.ROW_PULL_ELBOW_ANGLE_MAX

                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle

                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok:
                    if left_elbow_angle < min_angle:
                        feedback.append(f"팔꿈치를 더 당기기 (현재: {left_elbow_angle:.0f}도, 목표: {min_angle}-{max_angle}도)")
                    else:
                        feedback.append(f"팔꿈치를 덜 당기기 (현재: {left_elbow_angle:.0f}도, 목표: {min_angle}-{max_angle}도)")
                if not right_arm_ok:
                    if right_elbow_angle < min_angle:
                        feedback.append(f"팔꿈치를 더 당기기 (현재: {right_elbow_angle:.0f}도, 목표: {min_angle}-{max_angle}도)")
                    else:
                        feedback.append(f"팔꿈치를 덜 당기기 (현재: {right_elbow_angle:.0f}도, 목표: {min_angle}-{max_angle}도)")
                is_correct = score == 100
                message = "로우 당기기 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_row_hold(self, xy, conf, bbox=None):
        """바벨/케틀벨 로우 홀드"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            missing_parts = []
            if not (left_shoulder and right_shoulder): missing_parts.append("어깨")
            if not (left_elbow and right_elbow and left_wrist and right_wrist): missing_parts.append("팔")
            message = f"{', '.join(missing_parts) if missing_parts else '상체'}를 카메라에 보이도록 위치 조정 (측면)"
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': message}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '측면 각도로 위치 조정'}
            else:
                min_angle = AIServerConfig.ROW_HOLD_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.ROW_HOLD_ELBOW_ANGLE_MAX

                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle

                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok:
                    if left_elbow_angle < min_angle:
                        feedback.append(f"홀드 자세, 조금 더 당기기 (현재: {left_elbow_angle:.0f}도, 목표: {min_angle}-{max_angle}도)")
                    else:
                        feedback.append(f"홀드 자세, 조금 풀기 (현재: {left_elbow_angle:.0f}도, 목표: {min_angle}-{max_angle}도)")
                if not right_arm_ok:
                    if right_elbow_angle < min_angle:
                        feedback.append(f"홀드 자세, 조금 더 당기기 (현재: {right_elbow_angle:.0f}도, 목표: {min_angle}-{max_angle}도)")
                    else:
                        feedback.append(f"홀드 자세, 조금 풀기 (현재: {right_elbow_angle:.0f}도, 목표: {min_angle}-{max_angle}도)")
                is_correct = score == 100
                message = "완벽한 홀드 자세!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_upright_start(self, xy, conf, bbox=None):
        """업라이트 로우 시작"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                elbow_min = AIServerConfig.UPRIGHT_START_ELBOW_ANGLE_MIN
                left_arm_ok = left_elbow_angle > elbow_min
                right_arm_ok = right_elbow_angle > elbow_min

                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok or not right_arm_ok: feedback.append(f"팔 펴고 시작")
                is_correct = score == 100
                message = "시작 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_upright_mid(self, xy, conf, bbox=None):
        """업라이트 로우 중간"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            avg_shoulder_y = (left_shoulder[1] + right_shoulder[1]) / 2
            avg_wrist_y = (left_wrist[1] + right_wrist[1]) / 2
            wrist_height_ratio = (avg_shoulder_y - avg_wrist_y) / (avg_shoulder_y + 1e-6)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.UPRIGHT_MID_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.UPRIGHT_MID_ELBOW_ANGLE_MAX
                height_min = AIServerConfig.UPRIGHT_MID_WRIST_HEIGHT_MIN
                height_max = AIServerConfig.UPRIGHT_MID_WRIST_HEIGHT_MAX

                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle
                height_ok = height_min <= wrist_height_ratio <= height_max

                score = (33 if left_arm_ok else 0) + (33 if right_arm_ok else 0) + (34 if height_ok else 0)
                feedback = []
                if not left_arm_ok or not right_arm_ok: feedback.append(f"팔꿈치 조정")
                if not height_ok: feedback.append(f"높이 조정")
                is_correct = score >= 95
                message = "중간 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_upright_top(self, xy, conf, bbox=None):
        """업라이트 로우 최상단"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            avg_shoulder_y = (left_shoulder[1] + right_shoulder[1]) / 2
            avg_wrist_y = (left_wrist[1] + right_wrist[1]) / 2
            wrist_height_ratio = (avg_shoulder_y - avg_wrist_y) / (avg_shoulder_y + 1e-6)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.UPRIGHT_TOP_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.UPRIGHT_TOP_ELBOW_ANGLE_MAX
                height_min = AIServerConfig.UPRIGHT_TOP_WRIST_HEIGHT_MIN

                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle
                height_ok = wrist_height_ratio > height_min

                score = (33 if left_arm_ok else 0) + (33 if right_arm_ok else 0) + (34 if height_ok else 0)
                feedback = []
                if not left_arm_ok or not right_arm_ok: feedback.append(f"팔꿈치 각도 조정")
                if not height_ok: feedback.append(f"더 높이 올리기")
                is_correct = score >= 95
                message = "완벽한 업라이트 로우!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_center(self, xy, conf, bbox=None):
        """사이드 런지 중앙"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                knee_min = AIServerConfig.LUNGE_CENTER_KNEE_ANGLE_MIN

                left_knee_ok = left_knee_angle > knee_min
                right_knee_ok = right_knee_angle > knee_min

                score = (50 if left_knee_ok else 0) + (50 if right_knee_ok else 0)
                feedback = []
                if not left_knee_ok or not right_knee_ok: feedback.append(f"다리 펴고 서기")
                is_correct = score == 100
                message = "준비 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_plank_knee(self, xy, conf, bbox=None):
        """플랭크 준비 (무릎 댄 자세)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None

        points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist, left_hip, right_hip, left_knee, right_knee]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                elbow_min = AIServerConfig.PLANK_KNEE_ELBOW_ANGLE_MIN
                elbow_max = AIServerConfig.PLANK_KNEE_ELBOW_ANGLE_MAX
                hip_min = AIServerConfig.PLANK_KNEE_HIP_ANGLE_MIN
                hip_max = AIServerConfig.PLANK_KNEE_HIP_ANGLE_MAX

                left_elbow_ok = elbow_min <= left_elbow_angle <= elbow_max
                right_elbow_ok = elbow_min <= right_elbow_angle <= elbow_max
                left_hip_ok = hip_min <= left_hip_angle <= hip_max
                right_hip_ok = hip_min <= right_hip_angle <= hip_max

                score = sum([left_elbow_ok, right_elbow_ok, left_hip_ok, right_hip_ok]) * 25
                feedback = []
                if not left_elbow_ok or not right_elbow_ok: feedback.append(f"팔꿈치 각도 조정")
                if not left_hip_ok or not right_hip_ok: feedback.append(f"엉덩이 자세 조정")
                is_correct = score == 100
                message = "준비 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_plank_hold(self, xy, conf, bbox=None):
        """플랭크 유지 (무릎 뗀 자세)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist, left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle, left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                elbow_min = AIServerConfig.PLANK_HOLD_ELBOW_ANGLE_MIN
                elbow_max = AIServerConfig.PLANK_HOLD_ELBOW_ANGLE_MAX
                hip_min = AIServerConfig.PLANK_HOLD_HIP_ANGLE_MIN
                knee_min = AIServerConfig.PLANK_HOLD_KNEE_ANGLE_MIN

                left_elbow_ok = elbow_min <= left_elbow_angle <= elbow_max
                right_elbow_ok = elbow_min <= right_elbow_angle <= elbow_max
                left_hip_ok = left_hip_angle > hip_min
                right_hip_ok = right_hip_angle > hip_min
                left_knee_ok = left_knee_angle > knee_min
                right_knee_ok = right_knee_angle > knee_min

                score = sum([left_elbow_ok, right_elbow_ok, left_hip_ok, right_hip_ok, left_knee_ok, right_knee_ok]) * 17
                feedback = []
                if not left_elbow_ok or not right_elbow_ok: feedback.append(f"팔꿈치 유지")
                if not left_hip_ok or not right_hip_ok: feedback.append(f"엉덩이 일직선 유지")
                if not left_knee_ok or not right_knee_ok: feedback.append(f"다리 펴기")
                is_correct = score >= 95
                message = "완벽한 플랭크!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_left_forward(self, xy, conf, bbox=None):
        """왼쪽 런지 (앞으로)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                front_min = AIServerConfig.LUNGE_LEFT_FRONT_KNEE_MIN
                front_max = AIServerConfig.LUNGE_LEFT_FRONT_KNEE_MAX
                back_min = AIServerConfig.LUNGE_LEFT_BACK_KNEE_MIN
                back_max = AIServerConfig.LUNGE_LEFT_BACK_KNEE_MAX

                # 왼쪽 앞다리 굽히고 오른쪽 뒷다리 굽히기
                left_ok = front_min <= left_knee_angle <= front_max
                right_ok = back_min <= right_knee_angle <= back_max

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok: feedback.append(f"왼쪽 무릎 조정")
                if not right_ok: feedback.append(f"오른쪽 무릎 조정")
                is_correct = score == 100
                message = "완벽한 왼쪽 런지!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_right_forward(self, xy, conf, bbox=None):
        """오른쪽 런지 (앞으로)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                front_min = AIServerConfig.LUNGE_RIGHT_FRONT_KNEE_MIN
                front_max = AIServerConfig.LUNGE_RIGHT_FRONT_KNEE_MAX
                back_min = AIServerConfig.LUNGE_RIGHT_BACK_KNEE_MIN
                back_max = AIServerConfig.LUNGE_RIGHT_BACK_KNEE_MAX

                # 오른쪽 앞다리 굽히고 왼쪽 뒷다리 굽히기
                right_ok = front_min <= right_knee_angle <= front_max
                left_ok = back_min <= left_knee_angle <= back_max

                score = (50 if right_ok else 0) + (50 if left_ok else 0)
                feedback = []
                if not right_ok: feedback.append(f"오른쪽 무릎 조정")
                if not left_ok: feedback.append(f"왼쪽 무릎 조정")
                is_correct = score == 100
                message = "완벽한 오른쪽 런지!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_overhead_start(self, xy, conf, bbox=None):
        """오버헤드 프레스 시작 (어깨 높이)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.OVERHEAD_START_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.OVERHEAD_START_ELBOW_ANGLE_MAX

                left_ok = min_angle <= left_elbow_angle <= max_angle
                right_ok = min_angle <= right_elbow_angle <= max_angle

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok or not right_ok: feedback.append(f"어깨 높이로 바벨 위치")
                is_correct = score == 100
                message = "시작 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_overhead_mid(self, xy, conf, bbox=None):
        """오버헤드 프레스 중간"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.OVERHEAD_MID_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.OVERHEAD_MID_ELBOW_ANGLE_MAX

                left_ok = min_angle <= left_elbow_angle <= max_angle
                right_ok = min_angle <= right_elbow_angle <= max_angle

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok or not right_ok: feedback.append(f"바벨 올리는 중")
                is_correct = score == 100
                message = "중간 단계 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_overhead_top(self, xy, conf, bbox=None):
        """오버헤드 프레스 완료 (머리 위)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                elbow_min = AIServerConfig.OVERHEAD_TOP_ELBOW_ANGLE_MIN

                left_ok = left_elbow_angle > elbow_min
                right_ok = right_elbow_angle > elbow_min

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok or not right_ok: feedback.append(f"팔 완전히 펴기")
                is_correct = score == 100
                message = "완벽한 오버헤드 프레스!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_curl_down(self, xy, conf, bbox=None):
        """컬 시작 (팔 아래)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                elbow_min = AIServerConfig.CURL_DOWN_ELBOW_ANGLE_MIN

                left_ok = left_elbow_angle > elbow_min
                right_ok = right_elbow_angle > elbow_min

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok or not right_ok: feedback.append(f"팔 펴고 시작")
                is_correct = score == 100
                message = "시작 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_curl_up(self, xy, conf, bbox=None):
        """컬 완료 (팔 굽히기)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '상체가 보이도록 위치해주세요'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.CURL_UP_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.CURL_UP_ELBOW_ANGLE_MAX

                left_ok = min_angle <= left_elbow_angle <= max_angle
                right_ok = min_angle <= right_elbow_angle <= max_angle

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok or not right_ok: feedback.append(f"팔꿈치 굽히기")
                is_correct = score == 100
                message = "완벽한 컬!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_left(self, xy, conf, bbox=None):
        """왼쪽 사이드 런지"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.LUNGE_DOWN_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.LUNGE_DOWN_KNEE_ANGLE_MAX
                straight_min = AIServerConfig.LUNGE_DOWN_STRAIGHT_KNEE_MIN

                # 왼쪽 무릎은 굽히고 오른쪽은 펴야 함
                left_bent = min_angle <= left_knee_angle <= max_angle
                right_straight = right_knee_angle > straight_min

                lunge_ok = left_bent and right_straight

                score = 100 if lunge_ok else 0
                feedback = []
                if not left_bent: feedback.append(f"왼쪽 무릎 더 굽히기")
                if not right_straight: feedback.append(f"오른쪽 다리 펴기")
                is_correct = score == 100
                message = "완벽한 왼쪽 런지!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_right(self, xy, conf, bbox=None):
        """오른쪽 사이드 런지"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.LUNGE_DOWN_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.LUNGE_DOWN_KNEE_ANGLE_MAX
                straight_min = AIServerConfig.LUNGE_DOWN_STRAIGHT_KNEE_MIN

                # 오른쪽 무릎은 굽히고 왼쪽은 펴야 함
                right_bent = min_angle <= right_knee_angle <= max_angle
                left_straight = left_knee_angle > straight_min

                lunge_ok = right_bent and left_straight

                score = 100 if lunge_ok else 0
                feedback = []
                if not right_bent: feedback.append(f"오른쪽 무릎 더 굽히기")
                if not left_straight: feedback.append(f"왼쪽 다리 펴기")
                is_correct = score == 100
                message = "완벽한 오른쪽 런지!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_bridge_down(self, xy, conf, bbox=None):
        """브릿지 시작 (바닥)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None

        points = [left_shoulder, right_shoulder, left_hip, right_hip, left_knee, right_knee]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 위치해주세요'}
        else:
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)

            if None in [left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                hip_min = AIServerConfig.BRIDGE_DOWN_HIP_ANGLE_MIN
                hip_max = AIServerConfig.BRIDGE_DOWN_HIP_ANGLE_MAX

                left_hip_ok = hip_min <= left_hip_angle <= hip_max
                right_hip_ok = hip_min <= right_hip_angle <= hip_max

                score = (50 if left_hip_ok else 0) + (50 if right_hip_ok else 0)
                feedback = []
                if not left_hip_ok or not right_hip_ok: feedback.append(f"바닥에 편하게 누우기")
                is_correct = score == 100
                message = "시작 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_bridge_up(self, xy, conf, bbox=None):
        """브릿지 (엉덩이 들기)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_shoulder, right_shoulder, left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 위치해주세요'}
        else:
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_hip_angle, right_hip_angle, left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                hip_min = AIServerConfig.BRIDGE_UP_HIP_ANGLE_MIN
                knee_min = AIServerConfig.BRIDGE_UP_KNEE_ANGLE_MIN
                knee_max = AIServerConfig.BRIDGE_UP_KNEE_ANGLE_MAX

                left_hip_ok = left_hip_angle > hip_min
                right_hip_ok = right_hip_angle > hip_min
                left_knee_ok = knee_min <= left_knee_angle <= knee_max
                right_knee_ok = knee_min <= right_knee_angle <= knee_max

                score = sum([left_hip_ok, right_hip_ok, left_knee_ok, right_knee_ok]) * 25
                feedback = []
                if not left_hip_ok or not right_hip_ok: feedback.append(f"엉덩이 더 높이")
                if not left_knee_ok or not right_knee_ok: feedback.append(f"무릎 각도 조정")
                is_correct = score == 100
                message = "완벽한 브릿지!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_knee_start(self, xy, conf, bbox=None):
        """니 드라이브 준비"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '전신이 보이도록 카메라 앞에 서주세요'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                knee_min = AIServerConfig.KNEE_START_KNEE_ANGLE_MIN

                left_knee_ok = left_knee_angle > knee_min
                right_knee_ok = right_knee_angle > knee_min

                score = (50 if left_knee_ok else 0) + (50 if right_knee_ok else 0)
                feedback = []
                if not left_knee_ok or not right_knee_ok: feedback.append(f"똑바로 서기")
                is_correct = score == 100
                message = "준비 자세 완료!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_knee_left(self, xy, conf, bbox=None):
        """왼쪽 무릎 들기"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None

        points = [left_hip, left_knee, left_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '왼쪽 다리가 보이도록 위치해주세요'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)

            if left_knee_angle is None:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.KNEE_LIFT_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.KNEE_LIFT_KNEE_ANGLE_MAX

                knee_lifted = min_angle <= left_knee_angle <= max_angle

                score = 100 if knee_lifted else 0
                feedback = []
                if not knee_lifted: feedback.append(f"왼쪽 무릎 더 들어올리기")
                is_correct = score == 100
                message = "완벽한 왼쪽 니 드라이브!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_knee_right(self, xy, conf, bbox=None):
        """오른쪽 무릎 들기"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        right_hip = xy[12] if conf[12] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [right_hip, right_knee, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '오른쪽 다리가 보이도록 위치해주세요'}
        else:
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if right_knee_angle is None:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': '각도 계산 실패'}
            else:
                min_angle = AIServerConfig.KNEE_LIFT_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.KNEE_LIFT_KNEE_ANGLE_MAX

                knee_lifted = min_angle <= right_knee_angle <= max_angle

                score = 100 if knee_lifted else 0
                feedback = []
                if not knee_lifted: feedback.append(f"오른쪽 무릎 더 들어올리기")
                is_correct = score == 100
                message = "완벽한 오른쪽 니 드라이브!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    @staticmethod
    def _calculate_angle(p1, p2, p3):
        """
        3개 점으로 각도 계산 (p2가 중심점)

        Args:
            p1, p2, p3: (x, y) 좌표

        Returns:
            float: 각도 (0-180도) 또는 None
        """
        if p1 is None or p2 is None or p3 is None:
            return None

        v1 = np.array([p1[0] - p2[0], p1[1] - p2[1]])
        v2 = np.array([p3[0] - p2[0], p3[1] - p2[1]])

        cos_angle = np.dot(v1, v2) / (np.linalg.norm(v1) * np.linalg.norm(v2) + 1e-6)
        angle = np.arccos(np.clip(cos_angle, -1.0, 1.0))
        return np.degrees(angle)