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
                {
                    'status': 'success' | 'error',
                    'is_correct': bool,
                    'score': int (0-100),
                    'feedback': str,
                    'current_pose': str,  # 현재 확인하는 포즈 이름
                    'pose_description': str  # 포즈 설명
                }
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
            # 첫 번째 감지된 사람의 바운딩 박스
            box = boxes[0].xyxy.cpu().numpy()[0]  # [x1, y1, x2, y2]
            bbox = [float(x) for x in box]

        # 포즈 이름에 따라 분석
        pose_name = pose_info['name']

        # 분석 결과 가져오기
        if pose_name == 'squat_stand':
            result = self._analyze_squat_stand(xy, conf, bbox)
        elif pose_name == 'squat_down':
            result = self._analyze_squat_down(xy, conf, bbox)
        elif pose_name == 'pushup_up':
            result = self._analyze_pushup_up(xy, conf, bbox)
        elif pose_name == 'pushup_down':
            result = self._analyze_pushup_down(xy, conf, bbox)
        else:
            return {
                'status': 'error',
                'message': f'Pose {pose_name} not implemented yet'
            }

        # 포즈 정보 추가
        result['current_pose'] = pose_info['name']
        result['pose_description'] = pose_info['description']

        return result

    def _analyze_squat_stand(self, xy, conf, bbox=None):
        """스쿼트 준비 자세 (선 자세) 분석"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD

        # 필요한 키포인트 추출 (엉덩이, 무릎, 발목)
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': '전신이 보이도록 카메라 앞에 서주세요'
            }
        else:
            # 각도 계산 (엉덩이-무릎-발목)
            left_leg_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_leg_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_leg_angle, right_leg_angle]:
                result = {
                    'status': 'success',
                    'is_correct': False,
                    'score': 0,
                    'feedback': '각도 계산 실패'
                }
            else:
                # 선 자세 판정 (다리가 펴져 있어야 함)
                stand_threshold = AIServerConfig.SQUAT_STAND_HIP_KNEE_THRESHOLD

                left_leg_ok = left_leg_angle > stand_threshold
                right_leg_ok = right_leg_angle > stand_threshold

                # 점수 계산
                score = 0
                if left_leg_ok:
                    score += 50
                if right_leg_ok:
                    score += 50

                # 피드백 생성
                feedback = []
                if not left_leg_ok:
                    feedback.append(f"왼쪽 다리 펴기 ({left_leg_angle:.0f}°)")
                if not right_leg_ok:
                    feedback.append(f"오른쪽 다리 펴기 ({right_leg_angle:.0f}°)")

                is_correct = score == 100
                message = "준비 자세 완료!" if is_correct else ", ".join(feedback)

                result = {
                    'status': 'success',
                    'is_correct': is_correct,
                    'score': score,
                    'feedback': message
                }

        # 추적 정보 추가
        if bbox:
            x1, y1, x2, y2 = bbox
            center_x = (x1 + x2) / 2
            center_y = (y1 + y2) / 2
            result['tracking'] = {
                'center_x': float(center_x),
                'center_y': float(center_y),
                'bbox': bbox
            }

        return result

    def _analyze_squat_down(self, xy, conf, bbox=None):
        """스쿼트 자세 (무릎 90도) 분석"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD

        # 필요한 키포인트 추출
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': '전신이 보이도록 카메라 앞에 서주세요'
            }
        else:
            # 각도 계산
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {
                    'status': 'success',
                    'is_correct': False,
                    'score': 0,
                    'feedback': '각도 계산 실패'
                }
            else:
                # 스쿼트 자세 판정 (무릎 각도가 80~100도 사이)
                min_angle = AIServerConfig.SQUAT_DOWN_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.SQUAT_DOWN_KNEE_ANGLE_MAX

                left_knee_ok = min_angle <= left_knee_angle <= max_angle
                right_knee_ok = min_angle <= right_knee_angle <= max_angle

                # 점수 계산
                score = 0
                if left_knee_ok:
                    score += 50
                if right_knee_ok:
                    score += 50

                # 피드백 생성
                feedback = []
                if not left_knee_ok:
                    if left_knee_angle < min_angle:
                        feedback.append(f"왼쪽 무릎 너무 깊음 ({left_knee_angle:.0f}°)")
                    else:
                        feedback.append(f"왼쪽 무릎 더 굽히기 ({left_knee_angle:.0f}°)")
                if not right_knee_ok:
                    if right_knee_angle < min_angle:
                        feedback.append(f"오른쪽 무릎 너무 깊음 ({right_knee_angle:.0f}°)")
                    else:
                        feedback.append(f"오른쪽 무릎 더 굽히기 ({right_knee_angle:.0f}°)")

                is_correct = score == 100
                message = "완벽한 스쿼트 자세!" if is_correct else ", ".join(feedback)

                result = {
                    'status': 'success',
                    'is_correct': is_correct,
                    'score': score,
                    'feedback': message
                }

        # 추적 정보 추가
        if bbox:
            x1, y1, x2, y2 = bbox
            center_x = (x1 + x2) / 2
            center_y = (y1 + y2) / 2
            result['tracking'] = {
                'center_x': float(center_x),
                'center_y': float(center_y),
                'bbox': bbox
            }

        return result

    def _analyze_pushup_up(self, xy, conf, bbox=None):
        """푸시업 준비 자세 (팔 펴기) 분석"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD

        # 필요한 키포인트 추출 (어깨, 팔꿈치, 손목, 엉덩이, 발목)
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]
        body_points = [left_shoulder, right_shoulder, left_hip, right_hip, left_ankle, right_ankle]

        if any(p is None for p in arm_points):
            result = {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': '상체가 전체적으로 보이도록 카메라 앞에 위치해주세요'
            }
        else:
            # 팔꿈치 각도 계산 (어깨-팔꿈치-손목)
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {
                    'status': 'success',
                    'is_correct': False,
                    'score': 0,
                    'feedback': '각도 계산 실패'
                }
            else:
                # 팔 펴기 판정 (팔꿈치가 펴져 있어야 함)
                min_angle = AIServerConfig.PUSHUP_UP_ELBOW_ANGLE_MIN

                left_arm_ok = left_elbow_angle > min_angle
                right_arm_ok = right_elbow_angle > min_angle

                # 점수 계산
                score = 0
                if left_arm_ok:
                    score += 50
                if right_arm_ok:
                    score += 50

                # 피드백 생성
                feedback = []
                if not left_arm_ok:
                    feedback.append(f"왼팔 더 펴기 ({left_elbow_angle:.0f}°)")
                if not right_arm_ok:
                    feedback.append(f"오른팔 더 펴기 ({right_elbow_angle:.0f}°)")

                is_correct = score == 100
                message = "준비 자세 완료!" if is_correct else ", ".join(feedback)

                result = {
                    'status': 'success',
                    'is_correct': is_correct,
                    'score': score,
                    'feedback': message
                }

        # 추적 정보 추가
        if bbox:
            x1, y1, x2, y2 = bbox
            center_x = (x1 + x2) / 2
            center_y = (y1 + y2) / 2
            result['tracking'] = {
                'center_x': float(center_x),
                'center_y': float(center_y),
                'bbox': bbox
            }

        return result

    def _analyze_pushup_down(self, xy, conf, bbox=None):
        """푸시업 자세 (팔 굽히기) 분석"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD

        # 필요한 키포인트 추출
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': '상체가 전체적으로 보이도록 카메라 앞에 위치해주세요'
            }
        else:
            # 팔꿈치 각도 계산
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {
                    'status': 'success',
                    'is_correct': False,
                    'score': 0,
                    'feedback': '각도 계산 실패'
                }
            else:
                # 팔 굽히기 판정 (팔꿈치 각도가 70~110도 사이)
                min_angle = AIServerConfig.PUSHUP_DOWN_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.PUSHUP_DOWN_ELBOW_ANGLE_MAX

                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle

                # 점수 계산
                score = 0
                if left_arm_ok:
                    score += 50
                if right_arm_ok:
                    score += 50

                # 피드백 생성
                feedback = []
                if not left_arm_ok:
                    if left_elbow_angle < min_angle:
                        feedback.append(f"왼팔 너무 깊이 굽힘 ({left_elbow_angle:.0f}°)")
                    else:
                        feedback.append(f"왼팔 더 굽히기 ({left_elbow_angle:.0f}°)")
                if not right_arm_ok:
                    if right_elbow_angle < min_angle:
                        feedback.append(f"오른팔 너무 깊이 굽힘 ({right_elbow_angle:.0f}°)")
                    else:
                        feedback.append(f"오른팔 더 굽히기 ({right_elbow_angle:.0f}°)")

                is_correct = score == 100
                message = "완벽한 푸시업 자세!" if is_correct else ", ".join(feedback)

                result = {
                    'status': 'success',
                    'is_correct': is_correct,
                    'score': score,
                    'feedback': message
                }

        # 추적 정보 추가
        if bbox:
            x1, y1, x2, y2 = bbox
            center_x = (x1 + x2) / 2
            center_y = (y1 + y2) / 2
            result['tracking'] = {
                'center_x': float(center_x),
                'center_y': float(center_y),
                'bbox': bbox
            }

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
