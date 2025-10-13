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

    def set_mode(self, mode):
        """
        운동 모드 설정

        Args:
            mode: 운동 모드 ('t_pose', 'squat', etc.)

        Returns:
            bool: 설정 성공 여부
        """
        if mode not in AIServerConfig.SUPPORTED_MODES:
            return False
        self.current_mode = mode
        return True

    def analyze_frame(self, frame):
        """
        프레임 분석 (현재 모드에 따라)

        Args:
            frame: OpenCV 이미지 (numpy 배열)

        Returns:
            dict: 분석 결과
                {
                    'status': 'success' | 'error',
                    'is_correct': bool,
                    'score': int (0-100),
                    'feedback': str,
                    'keypoints': dict (선택사항)
                }
        """
        if self.current_mode is None:
            return {
                'status': 'error',
                'message': 'No mode selected'
            }

        # YOLO Pose 추론
        results = self.model(frame, verbose=AIServerConfig.VERBOSE)

        if results[0].keypoints is None or len(results[0].keypoints) == 0:
            return {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': '사람이 감지되지 않았습니다'
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

        # 모드에 따라 분석
        if self.current_mode == 't_pose':
            return self._analyze_t_pose(xy, conf, bbox)
        elif self.current_mode == 'squat':
            return self._analyze_squat(xy, conf, bbox)
        else:
            return {
                'status': 'error',
                'message': f'Mode {self.current_mode} not implemented yet'
            }

    def _analyze_t_pose(self, xy, conf, bbox=None):
        """T자 서기 자세 분석"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD

        # 필요한 키포인트 추출
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        points = [left_shoulder, right_shoulder, left_elbow,
                  right_elbow, left_wrist, right_wrist]

        if any(p is None for p in points):
            return {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': '팔 전체가 보이도록 카메라 앞에 서주세요'
            }

        # 각도 계산
        left_arm_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
        right_arm_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
        left_horizontal = self._calculate_horizontal_angle(left_shoulder, left_wrist)
        right_horizontal = self._calculate_horizontal_angle(right_shoulder, right_wrist)

        if None in [left_arm_angle, right_arm_angle, left_horizontal, right_horizontal]:
            return {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': '각도 계산 실패'
            }

        # T자 판정
        arm_threshold = AIServerConfig.T_POSE_ARM_STRAIGHT_THRESHOLD
        horizontal_threshold = AIServerConfig.T_POSE_HORIZONTAL_THRESHOLD

        left_arm_ok = left_arm_angle > arm_threshold
        right_arm_ok = right_arm_angle > arm_threshold
        left_horizontal_ok = left_horizontal < horizontal_threshold
        right_horizontal_ok = right_horizontal < horizontal_threshold

        # 점수 계산
        score = 0
        if left_arm_ok:
            score += 25
        if right_arm_ok:
            score += 25
        if left_horizontal_ok:
            score += 25
        if right_horizontal_ok:
            score += 25

        # 피드백 생성
        feedback = []
        if not left_arm_ok:
            feedback.append(f"왼팔 펴기 ({left_arm_angle:.0f}°)")
        if not right_arm_ok:
            feedback.append(f"오른팔 펴기 ({right_arm_angle:.0f}°)")
        if not left_horizontal_ok:
            feedback.append(f"왼팔 수평 ({left_horizontal:.0f}°)")
        if not right_horizontal_ok:
            feedback.append(f"오른팔 수평 ({right_horizontal:.0f}°)")

        is_correct = score == 100
        message = "완벽한 T자 자세!" if is_correct else ", ".join(feedback)

        result = {
            'status': 'success',
            'is_correct': is_correct,
            'score': score,
            'feedback': message,
            'keypoints': {
                'left_arm_angle': float(left_arm_angle),
                'right_arm_angle': float(right_arm_angle),
                'left_horizontal': float(left_horizontal),
                'right_horizontal': float(right_horizontal)
            }
        }

        # 추적 정보 추가 (바운딩 박스가 있는 경우)
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

    def _analyze_squat(self, xy, conf, bbox=None):
        """스쿼트 자세 분석 (향후 구현)"""
        return {
            'status': 'error',
            'message': 'Squat mode not implemented yet'
        }

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

    @staticmethod
    def _calculate_horizontal_angle(p1, p2):
        """
        두 점이 수평선과 이루는 각도 계산

        Args:
            p1, p2: (x, y) 좌표

        Returns:
            float: 각도 (0도 = 완전 수평) 또는 None
        """
        if p1 is None or p2 is None:
            return None

        dx = p2[0] - p1[0]
        dy = p2[1] - p1[1]
        angle = np.degrees(np.arctan2(dy, dx))
        return abs(angle)
