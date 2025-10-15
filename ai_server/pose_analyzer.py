"""
YOLO Pose Analysis Module
Analyzes exercise posture and provides feedback
"""

import numpy as np
from ultralytics import YOLO
from ai_config import AIServerConfig


class PoseAnalyzer:
    """YOLO Pose-based posture analyzer"""

    def __init__(self, model_path=None):
        """
        Initialize

        Args:
            model_path: YOLO model file path (get from config if None)
        """
        model_path = model_path or AIServerConfig.MODEL_PATH
        self.model = YOLO(model_path)
        self.current_mode = None
        self.current_pose_index = 0  # Current pose index

    def set_mode(self, mode):
        """
        Set exercise mode

        Args:
            mode: Exercise mode ('squat', 'pushup', etc.)

        Returns:
            bool: Whether setting was successful
        """
        if mode not in AIServerConfig.SUPPORTED_MODES:
            return False
        self.current_mode = mode
        self.current_pose_index = 0  # Reset pose index when mode changes
        return True

    def set_pose_index(self, pose_index):
        """
        Set current pose index

        Args:
            pose_index: Pose index (starting from 0)

        Returns:
            bool: Whether setting was successful
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
        Get current pose information

        Returns:
            dict: Pose information or None
        """
        if self.current_mode is None:
            return None

        poses = AIServerConfig.MODE_POSES.get(self.current_mode, [])
        if self.current_pose_index >= len(poses):
            return None

        return poses[self.current_pose_index]

    def analyze_frame(self, frame):
        """
        Analyze frame (according to current mode and pose)

        Args:
            frame: OpenCV image (numpy array)

        Returns:
            dict: Analysis result
        """
        if self.current_mode is None:
            return {
                'status': 'error',
                'message': 'No mode selected'
            }

        # Get current pose info
        pose_info = self.get_current_pose_info()
        if pose_info is None:
            return {
                'status': 'error',
                'message': 'Invalid pose index'
            }

        # YOLO Pose inference
        results = self.model(frame, verbose=AIServerConfig.VERBOSE)

        if results[0].keypoints is None or len(results[0].keypoints) == 0:
            return {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': 'No person detected',
                'current_pose': pose_info['name'],
                'pose_description': pose_info['description']
            }

        # Extract keypoints
        keypoints = results[0].keypoints[0]
        xy = keypoints.xy.cpu().numpy()[0]  # (17, 2)
        conf = keypoints.conf.cpu().numpy()[0]  # (17,)

        # Extract bounding box (for tracking)
        boxes = results[0].boxes
        bbox = None
        if boxes is not None and len(boxes) > 0:
            box = boxes[0].xyxy.cpu().numpy()[0]  # [x1, y1, x2, y2]
            bbox = [float(x) for x in box]

        # Analyze according to pose name
        pose_name = pose_info['name']

        # Analysis function mapping
        analysis_map = {
            # Bodyweight exercises
            'squat_stand': self._analyze_squat_stand,
            'squat_down': self._analyze_squat_down,
            'pushup_up': self._analyze_pushup_up,
            'pushup_down': self._analyze_pushup_down,
            'plank_knee': self._analyze_plank_knee,
            'plank_hold': self._analyze_plank_hold,
            'lunge_center': self._analyze_lunge_center,
            'lunge_left': self._analyze_lunge_left_forward,
            'lunge_right': self._analyze_lunge_right_forward,
            # Kettlebell exercises
            'swing_start': self._analyze_swing_start,
            'swing_up': self._analyze_swing_up,
            'deadlift_down': self._analyze_deadlift_down,
            'deadlift_up': self._analyze_deadlift_up,
            # Barbell exercises (reuse row functions)
            'barbell_row_start': self._analyze_row_start,
            'barbell_row_pull': self._analyze_row_pull,
            'barbell_row_hold': self._analyze_row_hold,
            'barbell_upright_start': self._analyze_upright_start,
            'barbell_upright_top': self._analyze_upright_top,
            'overhead_start': self._analyze_overhead_start,
            'overhead_top': self._analyze_overhead_top,
            'curl_down': self._analyze_curl_down,
            'curl_up': self._analyze_curl_up,
            'reverse_curl_down': self._analyze_curl_down,  # Use same function
            'reverse_curl_up': self._analyze_curl_up,      # Use same function
            # Other exercises
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

        # Add pose info
        result['current_pose'] = pose_info['name']
        result['pose_description'] = pose_info['description']

        # Attach keypoint info for downstream visualization
        result['keypoints'] = {
            'xy': xy.tolist(),
            'conf': conf.tolist()
        }

        return result

    def _analyze_squat_stand(self, xy, conf, bbox=None):
        """Squat ready position (standing) analysis"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_leg_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_leg_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_leg_angle, right_leg_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                stand_threshold = AIServerConfig.SQUAT_STAND_HIP_KNEE_THRESHOLD
                left_leg_ok = left_leg_angle > stand_threshold
                right_leg_ok = right_leg_angle > stand_threshold
                score = (50 if left_leg_ok else 0) + (50 if right_leg_ok else 0)
                feedback = []
                if not left_leg_ok: feedback.append(f"Straighten left leg ({left_leg_angle:.0f}deg)")
                if not right_leg_ok: feedback.append(f"Straighten right leg ({right_leg_angle:.0f}deg)")
                is_correct = score == 100
                message = "Ready position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_squat_down(self, xy, conf, bbox=None):
        """Squat position (90-degree knee) analysis"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.SQUAT_DOWN_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.SQUAT_DOWN_KNEE_ANGLE_MAX
                left_knee_ok = min_angle <= left_knee_angle <= max_angle
                right_knee_ok = min_angle <= right_knee_angle <= max_angle
                score = (50 if left_knee_ok else 0) + (50 if right_knee_ok else 0)
                feedback = []
                if not left_knee_ok:
                    feedback.append(f"Left knee {'too deep' if left_knee_angle < min_angle else 'bend more'} ({left_knee_angle:.0f}deg)")
                if not right_knee_ok:
                    feedback.append(f"Right knee {'too deep' if right_knee_angle < min_angle else 'bend more'} ({right_knee_angle:.0f}deg)")
                is_correct = score == 100
                message = "Perfect squat!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_pushup_up(self, xy, conf, bbox=None):
        """Pushup ready position (arms extended) analysis"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.PUSHUP_UP_ELBOW_ANGLE_MIN
                left_arm_ok = left_elbow_angle > min_angle
                right_arm_ok = right_elbow_angle > min_angle
                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok: feedback.append(f"Straighten left arm ({left_elbow_angle:.0f}deg)")
                if not right_arm_ok: feedback.append(f"Straighten right arm ({right_elbow_angle:.0f}deg)")
                is_correct = score == 100
                message = "Ready position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_pushup_down(self, xy, conf, bbox=None):
        """Pushup position (arms bent) analysis"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.PUSHUP_DOWN_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.PUSHUP_DOWN_ELBOW_ANGLE_MAX
                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle
                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok:
                    feedback.append(f"Left arm {'bent too much' if left_elbow_angle < min_angle else 'bend more'} ({left_elbow_angle:.0f}deg)")
                if not right_arm_ok:
                    feedback.append(f"Right arm {'bent too much' if right_elbow_angle < min_angle else 'bend more'} ({right_elbow_angle:.0f}deg)")
                is_correct = score == 100
                message = "Perfect pushup!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_swing_start(self, xy, conf, bbox=None):
        """Kettlebell swing starting position"""
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
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
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
                if not left_arm_ok: feedback.append(f"Straighten left arm")
                if not right_arm_ok: feedback.append(f"Straighten right arm")
                if not left_hip_ok or not right_hip_ok: feedback.append(f"Adjust hip position")
                is_correct = score == 100
                message = "Starting position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_swing_up(self, xy, conf, bbox=None):
        """Kettlebell swing up position"""
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
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            
            avg_shoulder_y = (left_shoulder[1] + right_shoulder[1]) / 2
            avg_wrist_y = (left_wrist[1] + right_wrist[1]) / 2
            wrist_height_ratio = avg_shoulder_y / (avg_wrist_y + 1e-6)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
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
                if not left_arm_ok or not right_arm_ok: feedback.append(f"Straighten arms")
                if not left_hip_ok or not right_hip_ok: feedback.append(f"Extend hips")
                if not height_ok: feedback.append(f"Swing kettlebell higher")
                is_correct = score == 100
                message = "Perfect swing position!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_deadlift_down(self, xy, conf, bbox=None):
        """Kettlebell deadlift starting position"""
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
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_hip_angle, right_hip_angle, left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
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
                if not left_hip_ok or not right_hip_ok: feedback.append(f"Adjust hip position")
                if not left_knee_ok or not right_knee_ok: feedback.append(f"Adjust knee position")
                is_correct = score == 100
                message = "Starting position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_deadlift_up(self, xy, conf, bbox=None):
        """Kettlebell deadlift complete position"""
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
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_hip_angle, right_hip_angle, left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                hip_min = AIServerConfig.DEADLIFT_UP_HIP_ANGLE_MIN
                knee_min = AIServerConfig.DEADLIFT_UP_KNEE_ANGLE_MIN

                left_hip_ok = left_hip_angle > hip_min
                right_hip_ok = right_hip_angle > hip_min
                left_knee_ok = left_knee_angle > knee_min
                right_knee_ok = right_knee_angle > knee_min

                score = sum([left_hip_ok, right_hip_ok, left_knee_ok, right_knee_ok]) * 25
                feedback = []
                if not left_hip_ok or not right_hip_ok: feedback.append(f"Extend hips")
                if not left_knee_ok or not right_knee_ok: feedback.append(f"Straighten legs")
                is_correct = score == 100
                message = "Perfect deadlift position!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_row_start(self, xy, conf, bbox=None):
        """Kettlebell row starting position"""
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
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
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
                if not left_arm_ok or not right_arm_ok: feedback.append(f"Straighten arms")
                if not left_hip_ok or not right_hip_ok: feedback.append(f"Bend forward at hips")
                is_correct = score == 100
                message = "Starting position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_row_pull(self, xy, conf, bbox=None):
        """Kettlebell row pull"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.ROW_PULL_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.ROW_PULL_ELBOW_ANGLE_MAX

                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle

                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok: feedback.append(f"Adjust left arm")
                if not right_arm_ok: feedback.append(f"Adjust right arm")
                is_correct = score == 100
                message = "Row pull complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_row_hold(self, xy, conf, bbox=None):
        """Kettlebell row hold"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.ROW_HOLD_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.ROW_HOLD_ELBOW_ANGLE_MAX

                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle

                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok: feedback.append(f"Maintain left arm hold")
                if not right_arm_ok: feedback.append(f"Maintain right arm hold")
                is_correct = score == 100
                message = "Perfect hold position!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_upright_start(self, xy, conf, bbox=None):
        """Upright row start"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                elbow_min = AIServerConfig.UPRIGHT_START_ELBOW_ANGLE_MIN
                left_arm_ok = left_elbow_angle > elbow_min
                right_arm_ok = right_elbow_angle > elbow_min

                score = (50 if left_arm_ok else 0) + (50 if right_arm_ok else 0)
                feedback = []
                if not left_arm_ok or not right_arm_ok: feedback.append(f"Start with arms extended")
                is_correct = score == 100
                message = "Starting position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_upright_top(self, xy, conf, bbox=None):
        """Upright row top"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            avg_shoulder_y = (left_shoulder[1] + right_shoulder[1]) / 2
            avg_wrist_y = (left_wrist[1] + right_wrist[1]) / 2
            wrist_height_ratio = (avg_shoulder_y - avg_wrist_y) / (avg_shoulder_y + 1e-6)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.UPRIGHT_TOP_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.UPRIGHT_TOP_ELBOW_ANGLE_MAX
                height_min = AIServerConfig.UPRIGHT_TOP_WRIST_HEIGHT_MIN

                left_arm_ok = min_angle <= left_elbow_angle <= max_angle
                right_arm_ok = min_angle <= right_elbow_angle <= max_angle
                height_ok = wrist_height_ratio > height_min

                score = (33 if left_arm_ok else 0) + (33 if right_arm_ok else 0) + (34 if height_ok else 0)
                feedback = []
                if not left_arm_ok or not right_arm_ok: feedback.append(f"Adjust elbow angle")
                if not height_ok: feedback.append(f"Pull higher")
                is_correct = score >= 95
                message = "Perfect upright row!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_center(self, xy, conf, bbox=None):
        """Side lunge center"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                knee_min = AIServerConfig.LUNGE_CENTER_KNEE_ANGLE_MIN

                left_knee_ok = left_knee_angle > knee_min
                right_knee_ok = right_knee_angle > knee_min

                score = (50 if left_knee_ok else 0) + (50 if right_knee_ok else 0)
                feedback = []
                if not left_knee_ok or not right_knee_ok: feedback.append(f"Stand with legs straight")
                is_correct = score == 100
                message = "Ready position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_plank_knee(self, xy, conf, bbox=None):
        """Plank ready (knees on ground)"""
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
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position full body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
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
                if not left_elbow_ok or not right_elbow_ok: feedback.append(f"Adjust elbow angle")
                if not left_hip_ok or not right_hip_ok: feedback.append(f"Adjust hip position")
                is_correct = score == 100
                message = "Ready position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_plank_hold(self, xy, conf, bbox=None):
        """Plank hold (knees lifted)"""
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
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position full body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_elbow_angle, right_elbow_angle, left_hip_angle, right_hip_angle, left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
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
                if not left_elbow_ok or not right_elbow_ok: feedback.append(f"Maintain elbow position")
                if not left_hip_ok or not right_hip_ok: feedback.append(f"Keep hips in line")
                if not left_knee_ok or not right_knee_ok: feedback.append(f"Straighten legs")
                is_correct = score >= 95
                message = "Perfect plank!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_left_forward(self, xy, conf, bbox=None):
        """Left lunge (forward)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                front_min = AIServerConfig.LUNGE_LEFT_FRONT_KNEE_MIN
                front_max = AIServerConfig.LUNGE_LEFT_FRONT_KNEE_MAX
                back_min = AIServerConfig.LUNGE_LEFT_BACK_KNEE_MIN
                back_max = AIServerConfig.LUNGE_LEFT_BACK_KNEE_MAX

                # Left front leg bent, right back leg bent
                left_ok = front_min <= left_knee_angle <= front_max
                right_ok = back_min <= right_knee_angle <= back_max

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok: feedback.append(f"Adjust left knee")
                if not right_ok: feedback.append(f"Adjust right knee")
                is_correct = score == 100
                message = "Perfect left lunge!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_right_forward(self, xy, conf, bbox=None):
        """Right lunge (forward)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                front_min = AIServerConfig.LUNGE_RIGHT_FRONT_KNEE_MIN
                front_max = AIServerConfig.LUNGE_RIGHT_FRONT_KNEE_MAX
                back_min = AIServerConfig.LUNGE_RIGHT_BACK_KNEE_MIN
                back_max = AIServerConfig.LUNGE_RIGHT_BACK_KNEE_MAX

                # Right front leg bent, left back leg bent
                right_ok = front_min <= right_knee_angle <= front_max
                left_ok = back_min <= left_knee_angle <= back_max

                score = (50 if right_ok else 0) + (50 if left_ok else 0)
                feedback = []
                if not right_ok: feedback.append(f"Adjust right knee")
                if not left_ok: feedback.append(f"Adjust left knee")
                is_correct = score == 100
                message = "Perfect right lunge!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_overhead_start(self, xy, conf, bbox=None):
        """Overhead press start (shoulder height)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.OVERHEAD_START_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.OVERHEAD_START_ELBOW_ANGLE_MAX

                left_ok = min_angle <= left_elbow_angle <= max_angle
                right_ok = min_angle <= right_elbow_angle <= max_angle

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok or not right_ok: feedback.append(f"Position barbell at shoulder height")
                is_correct = score == 100
                message = "Starting position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_overhead_top(self, xy, conf, bbox=None):
        """Overhead press complete (overhead)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                elbow_min = AIServerConfig.OVERHEAD_TOP_ELBOW_ANGLE_MIN

                left_ok = left_elbow_angle > elbow_min
                right_ok = right_elbow_angle > elbow_min

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok or not right_ok: feedback.append(f"Fully extend arms")
                is_correct = score == 100
                message = "Perfect overhead press!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_curl_down(self, xy, conf, bbox=None):
        """Curl start (arms down)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                elbow_min = AIServerConfig.CURL_DOWN_ELBOW_ANGLE_MIN

                left_ok = left_elbow_angle > elbow_min
                right_ok = right_elbow_angle > elbow_min

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok or not right_ok: feedback.append(f"Start with arms extended")
                is_correct = score == 100
                message = "Starting position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_curl_up(self, xy, conf, bbox=None):
        """Curl complete (arms bent)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_elbow = xy[7] if conf[7] > threshold else None
        right_elbow = xy[8] if conf[8] > threshold else None
        left_wrist = xy[9] if conf[9] > threshold else None
        right_wrist = xy[10] if conf[10] > threshold else None

        arm_points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]

        if any(p is None for p in arm_points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        else:
            left_elbow_angle = self._calculate_angle(left_shoulder, left_elbow, left_wrist)
            right_elbow_angle = self._calculate_angle(right_shoulder, right_elbow, right_wrist)

            if None in [left_elbow_angle, right_elbow_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.CURL_UP_ELBOW_ANGLE_MIN
                max_angle = AIServerConfig.CURL_UP_ELBOW_ANGLE_MAX

                left_ok = min_angle <= left_elbow_angle <= max_angle
                right_ok = min_angle <= right_elbow_angle <= max_angle

                score = (50 if left_ok else 0) + (50 if right_ok else 0)
                feedback = []
                if not left_ok or not right_ok: feedback.append(f"Curl elbows")
                is_correct = score == 100
                message = "Perfect curl!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_left(self, xy, conf, bbox=None):
        """Left side lunge"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.LUNGE_DOWN_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.LUNGE_DOWN_KNEE_ANGLE_MAX
                straight_min = AIServerConfig.LUNGE_DOWN_STRAIGHT_KNEE_MIN

                # Left knee bent, right leg straight
                left_bent = min_angle <= left_knee_angle <= max_angle
                right_straight = right_knee_angle > straight_min

                lunge_ok = left_bent and right_straight

                score = 100 if lunge_ok else 0
                feedback = []
                if not left_bent: feedback.append(f"Bend left knee more")
                if not right_straight: feedback.append(f"Straighten right leg")
                is_correct = score == 100
                message = "Perfect left lunge!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_lunge_right(self, xy, conf, bbox=None):
        """Right side lunge"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.LUNGE_DOWN_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.LUNGE_DOWN_KNEE_ANGLE_MAX
                straight_min = AIServerConfig.LUNGE_DOWN_STRAIGHT_KNEE_MIN

                # Right knee bent, left leg straight
                right_bent = min_angle <= right_knee_angle <= max_angle
                left_straight = left_knee_angle > straight_min

                lunge_ok = right_bent and left_straight

                score = 100 if lunge_ok else 0
                feedback = []
                if not right_bent: feedback.append(f"Bend right knee more")
                if not left_straight: feedback.append(f"Straighten left leg")
                is_correct = score == 100
                message = "Perfect right lunge!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_bridge_down(self, xy, conf, bbox=None):
        """Bridge start (on ground)"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_shoulder = xy[5] if conf[5] > threshold else None
        right_shoulder = xy[6] if conf[6] > threshold else None
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None

        points = [left_shoulder, right_shoulder, left_hip, right_hip, left_knee, right_knee]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position full body in view'}
        else:
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)

            if None in [left_hip_angle, right_hip_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                hip_min = AIServerConfig.BRIDGE_DOWN_HIP_ANGLE_MIN
                hip_max = AIServerConfig.BRIDGE_DOWN_HIP_ANGLE_MAX

                left_hip_ok = hip_min <= left_hip_angle <= hip_max
                right_hip_ok = hip_min <= right_hip_angle <= hip_max

                score = (50 if left_hip_ok else 0) + (50 if right_hip_ok else 0)
                feedback = []
                if not left_hip_ok or not right_hip_ok: feedback.append(f"Lie comfortably on ground")
                is_correct = score == 100
                message = "Starting position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_bridge_up(self, xy, conf, bbox=None):
        """Bridge (hips lifted)"""
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
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position full body in view'}
        else:
            left_hip_angle = self._calculate_angle(left_shoulder, left_hip, left_knee)
            right_hip_angle = self._calculate_angle(right_shoulder, right_hip, right_knee)
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_hip_angle, right_hip_angle, left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
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
                if not left_hip_ok or not right_hip_ok: feedback.append(f"Lift hips higher")
                if not left_knee_ok or not right_knee_ok: feedback.append(f"Adjust knee angle")
                is_correct = score == 100
                message = "Perfect bridge!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_knee_start(self, xy, conf, bbox=None):
        """Knee drive ready"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        right_hip = xy[12] if conf[12] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [left_hip, right_hip, left_knee, right_knee, left_ankle, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if None in [left_knee_angle, right_knee_angle]:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                knee_min = AIServerConfig.KNEE_START_KNEE_ANGLE_MIN

                left_knee_ok = left_knee_angle > knee_min
                right_knee_ok = right_knee_angle > knee_min

                score = (50 if left_knee_ok else 0) + (50 if right_knee_ok else 0)
                feedback = []
                if not left_knee_ok or not right_knee_ok: feedback.append(f"Stand straight")
                is_correct = score == 100
                message = "Ready position complete!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_knee_left(self, xy, conf, bbox=None):
        """Left knee lift"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        left_hip = xy[11] if conf[11] > threshold else None
        left_knee = xy[13] if conf[13] > threshold else None
        left_ankle = xy[15] if conf[15] > threshold else None

        points = [left_hip, left_knee, left_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position left leg in view'}
        else:
            left_knee_angle = self._calculate_angle(left_hip, left_knee, left_ankle)

            if left_knee_angle is None:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.KNEE_LIFT_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.KNEE_LIFT_KNEE_ANGLE_MAX

                knee_lifted = min_angle <= left_knee_angle <= max_angle

                score = 100 if knee_lifted else 0
                feedback = []
                if not knee_lifted: feedback.append(f"Lift left knee higher")
                is_correct = score == 100
                message = "Perfect left knee drive!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    def _analyze_knee_right(self, xy, conf, bbox=None):
        """Right knee lift"""
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        right_hip = xy[12] if conf[12] > threshold else None
        right_knee = xy[14] if conf[14] > threshold else None
        right_ankle = xy[16] if conf[16] > threshold else None

        points = [right_hip, right_knee, right_ankle]

        if any(p is None for p in points):
            result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position right leg in view'}
        else:
            right_knee_angle = self._calculate_angle(right_hip, right_knee, right_ankle)

            if right_knee_angle is None:
                result = {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
            else:
                min_angle = AIServerConfig.KNEE_LIFT_KNEE_ANGLE_MIN
                max_angle = AIServerConfig.KNEE_LIFT_KNEE_ANGLE_MAX

                knee_lifted = min_angle <= right_knee_angle <= max_angle

                score = 100 if knee_lifted else 0
                feedback = []
                if not knee_lifted: feedback.append(f"Lift right knee higher")
                is_correct = score == 100
                message = "Perfect right knee drive!" if is_correct else ", ".join(feedback)
                result = {'status': 'success', 'is_correct': is_correct, 'score': score, 'feedback': message}

        if bbox:
            x1, y1, x2, y2 = bbox
            result['tracking'] = {'center_x': float((x1 + x2) / 2), 'center_y': float((y1 + y2) / 2), 'bbox': bbox}
        return result

    @staticmethod
    def _calculate_angle(p1, p2, p3):
        """
        Calculate angle from 3 points (p2 is the center point)

        Args:
            p1, p2, p3: (x, y) coordinates

        Returns:
            float: Angle (0-180 degrees) or None
        """
        if p1 is None or p2 is None or p3 is None:
            return None

        v1 = np.array([p1[0] - p2[0], p1[1] - p2[1]])
        v2 = np.array([p3[0] - p2[0], p3[1] - p2[1]])

        cos_angle = np.dot(v1, v2) / (np.linalg.norm(v1) * np.linalg.norm(v2) + 1e-6)
        angle = np.arccos(np.clip(cos_angle, -1.0, 1.0))
        return np.degrees(angle)
