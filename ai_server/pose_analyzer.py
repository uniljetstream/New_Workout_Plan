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

    # ---------------------------
    # Mode / Pose index handling
    # ---------------------------
    def set_mode(self, mode):
        """Set exercise mode"""
        if mode not in AIServerConfig.SUPPORTED_MODES:
            return False
        self.current_mode = mode
        self.current_pose_index = 0
        return True

    def set_pose_index(self, pose_index):
        """Set current pose index"""
        if self.current_mode is None:
            return False
        poses = AIServerConfig.MODE_POSES.get(self.current_mode, [])
        if pose_index < 0 or pose_index >= len(poses):
            return False
        self.current_pose_index = pose_index
        return True

    def get_current_pose_info(self):
        """Get current pose information"""
        if self.current_mode is None:
            return None
        poses = AIServerConfig.MODE_POSES.get(self.current_mode, [])
        if self.current_pose_index >= len(poses):
            return None
        return poses[self.current_pose_index]

    # ---------------------------
    # Inference with fallback
    # ---------------------------
    def _infer_with_fallback(self, frame):
        """
        Run YOLO pose inference with primary params, then fallback if needed.
        Returns (result_obj, used_fallback: bool)
        """
        # 1) primary
        res = self.model(
            frame,
            conf=AIServerConfig.YOLO_CONF,
            iou=AIServerConfig.YOLO_IOU,
            imgsz=AIServerConfig.YOLO_IMGSZ,
            verbose=AIServerConfig.VERBOSE,
        )
        used_fallback = False

        # If nothing detected (no boxes or keypoints), try fallback
        need_fallback = (
            len(res) == 0
            or (res[0].keypoints is None or len(res[0].keypoints) == 0)
        )
        if need_fallback:
            res = self.model(
                frame,
                conf=AIServerConfig.FALLBACK_CONF,
                iou=AIServerConfig.YOLO_IOU,
                imgsz=AIServerConfig.FALLBACK_IMGSZ,
                augment=True,
                verbose=AIServerConfig.VERBOSE,
            )
            used_fallback = True

        return res, used_fallback

    def _extract_primary_person(self, result):
        """
        Robustly extract the 'best' person: prefer first box; if boxes empty but keypoints exist,
        pick the keypoints instance with the highest mean confidence.
        Returns (xy (17,2), conf (17,), bbox [x1,y1,x2,y2] or None)
        """
        if result.keypoints is None or len(result.keypoints) == 0:
            return None, None, None

        kps = result.keypoints  # N instances
        boxes = result.boxes

        # If boxes exist, use index 0 (sorted by confidence already)
        if boxes is not None and len(boxes) > 0:
            idx = 0
        else:
            # pick by mean keypoint conf
            means = []
            for i in range(len(kps)):
                conf_i = kps[i].conf.cpu().numpy()[0]
                means.append(float(np.mean(conf_i)))
            idx = int(np.argmax(means))

        xy = kps[idx].xy.cpu().numpy()[0]      # (17,2)
        conf = kps[idx].conf.cpu().numpy()[0]  # (17,)

        bbox = None
        if boxes is not None and len(boxes) > 0:
            try:
                box = boxes[idx].xyxy.cpu().numpy()[0]
                bbox = [float(x) for x in box]
            except Exception:
                bbox = None

        return xy, conf, bbox

    # ---------------------------
    # Main entry
    # ---------------------------
    def analyze_frame(self, frame):
        """
        Analyze frame (according to current mode and pose)
        """
        if self.current_mode is None:
            return {'status': 'error', 'message': 'No mode selected'}

        pose_info = self.get_current_pose_info()
        if pose_info is None:
            return {'status': 'error', 'message': 'Invalid pose index'}

        # YOLO Pose inference with fallback
        results, used_fallback = self._infer_with_fallback(frame)
        result0 = results[0]

        xy, conf, bbox = self._extract_primary_person(result0)

        if xy is None or conf is None:
            # 최종적으로도 사람/키포인트가 안 잡힘
            return {
                'status': 'success',
                'is_correct': False,
                'score': 0,
                'feedback': 'No person detected',
                'current_pose': pose_info['name'],
                'pose_description': pose_info['description'],
                'detector': 'fallback' if used_fallback else 'primary'
            }

        # 분석 함수 매핑 (side_lunge 관련 전부 제거됨)
        pose_name = pose_info['name']
        analysis_map = {
            # Bodyweight
            'squat_stand': self._analyze_squat_stand,
            'squat_down': self._analyze_squat_down,
            'pushup_up': self._analyze_pushup_up,
            'pushup_down': self._analyze_pushup_down,
            'lunge_center': self._analyze_lunge_center,        # 개선됨(측면 기본)
            'lunge_left': self._analyze_lunge_left_side,
            'lunge_right': self._analyze_lunge_right_side,
            # Kettlebell
            'swing_start': self._analyze_swing_start,
            'swing_up': self._analyze_swing_up,
            'deadlift_down': self._analyze_deadlift_down,
            'deadlift_up': self._analyze_deadlift_up,
            # Barbell
            'barbell_row_start': self._analyze_row_start,
            'barbell_row_pull': self._analyze_row_pull,
            'barbell_upright_start': self._analyze_upright_start,
            'barbell_upright_top': self._analyze_upright_top,
            'overhead_start': self._analyze_overhead_start,
            'overhead_top': self._analyze_overhead_top,        # 개선됨(얼굴 없이도 OK)
            'curl_down': self._analyze_curl_down,
            'curl_up': self._analyze_curl_up,
            'reverse_curl_down': self._analyze_curl_down,
            'reverse_curl_up': self._analyze_curl_up,
        }

        if pose_name not in analysis_map:
            return {'status': 'error', 'message': f'Pose {pose_name} not implemented yet'}

        result = analysis_map[pose_name](xy, conf, bbox)

        # Add pose & keypoint info
        result['current_pose'] = pose_info['name']
        result['pose_description'] = pose_info['description']
        result['keypoints'] = {'xy': xy.tolist(), 'conf': conf.tolist()}
        result['detector'] = 'fallback' if used_fallback else 'primary'
        return result

    # ---------------------------
    # Helpers
    # ---------------------------
    @staticmethod
    def _angle(p1, p2, p3):
        """Calculate angle from 3 points (p2 is the center)."""
        if p1 is None or p2 is None or p3 is None:
            return None
        v1 = np.array([p1[0] - p2[0], p1[1] - p2[1]])
        v2 = np.array([p3[0] - p2[0], p3[1] - p2[1]])
        denom = (np.linalg.norm(v1) * np.linalg.norm(v2)) + 1e-6
        cos_angle = np.dot(v1, v2) / denom
        angle = np.arccos(np.clip(cos_angle, -1.0, 1.0))
        return float(np.degrees(angle))

    @staticmethod
    def _slope(p_top, p_bottom):
        """Return absolute slope of vertical line (x diff / y diff); smaller => more vertical."""
        if p_top is None or p_bottom is None:
            return None
        dy = (p_bottom[1] - p_top[1]) + 1e-6
        dx = (p_bottom[0] - p_top[0])
        return abs(dx / dy)

    # ---------------------------
    # Analyzers
    # ---------------------------
    def _analyze_squat_stand(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Lh, Rh = xy[11] if conf[11] > threshold else None, xy[12] if conf[12] > threshold else None
        Lk, Rk = xy[13] if conf[13] > threshold else None, xy[14] if conf[14] > threshold else None
        La, Ra = xy[15] if conf[15] > threshold else None, xy[16] if conf[16] > threshold else None

        if any(p is None for p in [Lh, Rh, Lk, Rk, La, Ra]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}

        L_ang = self._angle(Lh, Lk, La)
        R_ang = self._angle(Rh, Rk, Ra)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}

        thr = AIServerConfig.SQUAT_STAND_HIP_KNEE_THRESHOLD
        L_ok, R_ok = L_ang > thr, R_ang > thr
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok: fb.append(f"Straighten left leg ({L_ang:.0f}deg)")
        if not R_ok: fb.append(f"Straighten right leg ({R_ang:.0f}deg)")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Ready position complete!" if score == 100 else ", ".join(fb)}

    def _analyze_squat_down(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Lh, Rh = xy[11] if conf[11] > threshold else None, xy[12] if conf[12] > threshold else None
        Lk, Rk = xy[13] if conf[13] > threshold else None, xy[14] if conf[14] > threshold else None
        La, Ra = xy[15] if conf[15] > threshold else None, xy[16] if conf[16] > threshold else None

        if any(p is None for p in [Lh, Rh, Lk, Rk, La, Ra]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}

        L_ang = self._angle(Lh, Lk, La)
        R_ang = self._angle(Rh, Rk, Ra)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}

        mn, mx = AIServerConfig.SQUAT_DOWN_KNEE_ANGLE_MIN, AIServerConfig.SQUAT_DOWN_KNEE_ANGLE_MAX
        L_ok, R_ok = (mn <= L_ang <= mx), (mn <= R_ang <= mx)
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok: fb.append(f"Left knee {'too deep' if L_ang < mn else 'bend more'} ({L_ang:.0f}deg)")
        if not R_ok: fb.append(f"Right knee {'too deep' if R_ang < mn else 'bend more'} ({R_ang:.0f}deg)")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Perfect squat!" if score == 100 else ", ".join(fb)}

    def _analyze_pushup_up(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None

        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}

        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}

        mn = AIServerConfig.PUSHUP_UP_ELBOW_ANGLE_MIN
        L_ok, R_ok = L_ang > mn, R_ang > mn
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok: fb.append(f"Straighten left arm ({L_ang:.0f}deg)")
        if not R_ok: fb.append(f"Straighten right arm ({R_ang:.0f}deg)")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Ready position complete!" if score == 100 else ", ".join(fb)}

    def _analyze_pushup_down(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None

        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}

        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}

        mn, mx = AIServerConfig.PUSHUP_DOWN_ELBOW_ANGLE_MIN, AIServerConfig.PUSHUP_DOWN_ELBOW_ANGLE_MAX
        if L_ang and R_ang:
            L_ok, R_ok = (mn <= L_ang <= mx), (mn <= R_ang <= mx)
        elif L_ang:
            L_ok, R_ok = (mn <= L_ang <= mx), True
        elif R_ang:
            L_ok, R_ok = True, (mn <= R_ang <= mx)
        else:
            L_ok = R_ok = False

        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok: fb.append(f"Left arm {'too deep' if L_ang < mn else 'bend more'} ({L_ang:.0f}deg)")
        if not R_ok: fb.append(f"Right arm {'too deep' if R_ang < mn else 'bend more'} ({R_ang:.0f}deg)")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Perfect pushup!" if score == 100 else ", ".join(fb)}

    def _analyze_swing_start(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None
        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn = AIServerConfig.SWING_START_ELBOW_ANGLE_MIN
        L_ok, R_ok = L_ang > mn, R_ang > mn
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok: fb.append("Straighten left arm")
        if not R_ok: fb.append("Straighten right arm")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Starting position complete!" if score == 100 else ", ".join(fb)}

    def _analyze_swing_up(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None
        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)

        avg_sh = (Ls[1] + Rs[1]) / 2
        avg_wr = (Lw[1] + Rw[1]) / 2
        height_ratio = avg_sh / (avg_wr + 1e-6)

        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}

        elbow_min = AIServerConfig.SWING_UP_ELBOW_ANGLE_MIN
        height_min = AIServerConfig.SWING_UP_SHOULDER_HEIGHT_MIN
        L_ok, R_ok = (L_ang > elbow_min), (R_ang > elbow_min)
        H_ok = height_ratio > height_min

        score = (33 if L_ok else 0) + (33 if R_ok else 0) + (34 if H_ok else 0)
        fb = []
        if not L_ok or not R_ok: fb.append("Straighten arms")
        if not H_ok: fb.append("Swing kettlebell higher")
        return {'status': 'success', 'is_correct': score >= 95, 'score': score, 'feedback': "Perfect swing position!" if score >= 95 else ", ".join(fb)}

    def _analyze_deadlift_down(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Lh, Rh = xy[11] if conf[11] > threshold else None, xy[12] if conf[12] > threshold else None
        Lk, Rk = xy[13] if conf[13] > threshold else None, xy[14] if conf[14] > threshold else None
        La, Ra = xy[15] if conf[15] > threshold else None, xy[16] if conf[16] > threshold else None
        if any(p is None for p in [Lh, Rh, Lk, Rk, La, Ra]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        L_ang = self._angle(Lh, Lk, La)
        R_ang = self._angle(Rh, Rk, Ra)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn, mx = AIServerConfig.DEADLIFT_DOWN_KNEE_ANGLE_MIN, AIServerConfig.DEADLIFT_DOWN_KNEE_ANGLE_MAX
        L_ok, R_ok = (mn <= L_ang <= mx), (mn <= R_ang <= mx)
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok or not R_ok: fb.append("Adjust knee position")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Starting position complete!" if score == 100 else ", ".join(fb)}

    def _analyze_deadlift_up(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Lh, Rh = xy[11] if conf[11] > threshold else None, xy[12] if conf[12] > threshold else None
        Lk, Rk = xy[13] if conf[13] > threshold else None, xy[14] if conf[14] > threshold else None
        La, Ra = xy[15] if conf[15] > threshold else None, xy[16] if conf[16] > threshold else None
        if any(p is None for p in [Lh, Rh, Lk, Rk, La, Ra]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Stand in front of camera (full body)'}
        L_ang = self._angle(Lh, Lk, La)
        R_ang = self._angle(Rh, Rk, Ra)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn = AIServerConfig.DEADLIFT_UP_KNEE_ANGLE_MIN
        L_ok, R_ok = (L_ang > mn), (R_ang > mn)
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok or not R_ok: fb.append("Straighten legs")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Perfect deadlift position!" if score == 100 else ", ".join(fb)}

    def _analyze_row_start(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None
        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn = AIServerConfig.ROW_START_ELBOW_ANGLE_MIN
        if L_ang and R_ang:
            L_ok, R_ok = (L_ang > mn), (R_ang > mn)
        elif L_ang:
            L_ok, R_ok = (L_ang > mn), True
        elif R_ang:
            L_ok, R_ok = True, (R_ang > mn)
        else:
            L_ok = R_ok = False
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok and not R_ok: fb.append("Straighten arms")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Starting position complete!" if score == 100 else ", ".join(fb)}

    def _analyze_row_pull(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None
        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn, mx = AIServerConfig.ROW_PULL_ELBOW_ANGLE_MIN, AIServerConfig.ROW_PULL_ELBOW_ANGLE_MAX
        L_ok, R_ok = (mn <= L_ang <= mx), (mn <= R_ang <= mx)
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok: fb.append("Adjust left arm")
        if not R_ok: fb.append("Adjust right arm")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Row pull complete!" if score == 100 else ", ".join(fb)}

    def _analyze_upright_start(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None
        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn = AIServerConfig.UPRIGHT_START_ELBOW_ANGLE_MIN
        L_ok, R_ok = (L_ang > mn), (R_ang > mn)
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok or not R_ok: fb.append("Start with arms extended")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Starting position complete!" if score == 100 else ", ".join(fb)}

    def _analyze_upright_top(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None
        Lh, Rh = xy[11] if conf[11] > threshold else None, xy[12] if conf[12] > threshold else None
        pts = [Ls, Rs, Le, Re, Lw, Rw, Lh, Rh]
        if any(p is None for p in pts):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn, mx = AIServerConfig.UPRIGHT_TOP_ELBOW_ANGLE_MIN, AIServerConfig.UPRIGHT_TOP_ELBOW_ANGLE_MAX
        avg_hip_y = (Lh[1] + Rh[1]) / 2
        avg_wr_y = (Lw[1] + Rw[1]) / 2
        height_ok = (avg_wr_y < avg_hip_y)
        L_ok, R_ok = (mn <= L_ang <= mx), (mn <= R_ang <= mx)
        score = (33 if L_ok else 0) + (33 if R_ok else 0) + (34 if height_ok else 0)
        fb = []
        if not L_ok or not R_ok: fb.append("Adjust elbow angle")
        if not height_ok: fb.append("Pull above stomach")
        return {'status': 'success', 'is_correct': score >= 95, 'score': score, 'feedback': "Perfect upright row!" if score >= 95 else ", ".join(fb)}

    def _analyze_overhead_start(self, xy, conf, bbox=None):
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None
        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view (front)'}
        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn, mx = AIServerConfig.OVERHEAD_START_ELBOW_ANGLE_MIN, AIServerConfig.OVERHEAD_START_ELBOW_ANGLE_MAX
        L_ok, R_ok = (mn <= L_ang <= mx), (mn <= R_ang <= mx)
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = [] if (L_ok and R_ok) else ["Position barbell at shoulder height"]
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Starting position complete!" if score == 100 else ", ".join(fb)}

    def _analyze_overhead_top(self, xy, conf, bbox=None):
        """
        Overhead press TOP — 얼굴/코 인식 실패해도 판정 가능하도록 보강.
        1) 기본: 팔꿈치 각도 충분 + 손목이 어깨보다 위
        2) 코(nose) 없을 때: 어깨 기준 높이만 사용
        3) 손목 누락: 팔꿈치 높이로 대체(팔꿈치가 어깨보다 위면 가산)
        """
        threshold = AIServerConfig.CONFIDENCE_THRESHOLD
        nose = xy[0] if conf[0] > threshold else None
        Ls, Rs = xy[5] if conf[5] > threshold else None, xy[6] if conf[6] > threshold else None
        Le, Re = xy[7] if conf[7] > threshold else None, xy[8] if conf[8] > threshold else None
        Lw, Rw = xy[9] if conf[9] > threshold else None, xy[10] if conf[10] > threshold else None

        # 최소 필요 포인트: 어깨·팔꿈치는 있어야 함
        if any(p is None for p in [Ls, Rs, Le, Re]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view (front)'}

        L_ang = self._angle(Ls, Le, Lw if Lw is not None else Le)
        R_ang = self._angle(Rs, Re, Rw if Rw is not None else Re)
        if L_ang is None or R_ang is None:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}

        elbow_min = AIServerConfig.OVERHEAD_TOP_ELBOW_ANGLE_MIN
        L_ext, R_ext = (L_ang > elbow_min), (R_ang > elbow_min)

        # 높이 판정
        avg_sh_y = (Ls[1] + Rs[1]) / 2.0

        # 손목 기반(정석)
        wrist_ok = False
        if Lw is not None and Rw is not None:
            avg_wr_y = (Lw[1] + Rw[1]) / 2.0
            wrist_ok = (avg_wr_y < avg_sh_y)  # 손목이 어깨보다 위
        else:
            # 손목 누락 시 팔꿈치 높이로 대체
            avg_elb_y = (Le[1] + Re[1]) / 2.0
            wrist_ok = (avg_elb_y < avg_sh_y * 0.98)  # 어깨보다 충분히 위(약간의 여유)

        # 코가 있으면 조금 더 엄격하게(코보다 위면 보너스)
        if nose is not None and (Lw is not None and Rw is not None):
            avg_wr_y = (Lw[1] + Rw[1]) / 2.0
            if avg_wr_y < nose[1]:
                wrist_ok = True  # 강한 만족

        score = (33 if L_ext else 0) + (33 if R_ext else 0) + (34 if wrist_ok else 0)
        fb = []
        if not L_ext or not R_ext: fb.append("Fully extend arms")
        if not wrist_ok: fb.append("Press higher")
        return {'status': 'success', 'is_correct': score >= 95, 'score': score, 'feedback': "Perfect overhead press!" if score >= 95 else ", ".join(fb)}

    def _analyze_lunge_center(self, xy, conf, bbox=None):
        """
        Lunge center (측면 선자세 디폴트).
        - 한쪽 다리만 보이더라도 '선자세'로 인식 (무릎 각도 충분히 펴졌는지 확인)
        - 어깨-엉덩이-발목 수직 정렬(기울기)로 보정
        """
        th = AIServerConfig.CONFIDENCE_THRESHOLD
        Lh, Rh = xy[11] if conf[11] > th else None, xy[12] if conf[12] > th else None
        Lk, Rk = xy[13] if conf[13] > th else None, xy[14] if conf[14] > th else None
        La, Ra = xy[15] if conf[15] > th else None, xy[16] if conf[16] > th else None
        Ls, Rs = xy[5] if conf[5] > th else None, xy[6] if conf[6] > th else None

        # 다리 한쪽만으로도 판정 가능
        left_ok = None
        right_ok = None
        if Lh and Lk and La:
            L_ang = self._angle(Lh, Lk, La)
            if L_ang is not None:
                left_ok = (L_ang > AIServerConfig.LUNGE_CENTER_KNEE_ANGLE_MIN - 5)  # 약간 완화
        if Rh and Rk and Ra:
            R_ang = self._angle(Rh, Rk, Ra)
            if R_ang is not None:
                right_ok = (R_ang > AIServerConfig.LUNGE_CENTER_KNEE_ANGLE_MIN - 5)

        # 수직 정렬 보정(어깨-엉덩이-발목)
        vertical_ok = False
        # 왼쪽 라인 또는 오른쪽 라인 기준
        for top, mid, bot in [(Ls, Lh, La), (Rs, Rh, Ra)]:
            s1 = self._slope(top, mid) if top and mid else None
            s2 = self._slope(mid, bot) if mid and bot else None
            # 기울기가 작을수록 수직. 0.25 이하면 충분히 수직으로 간주
            if (s1 is not None and s1 < 0.25) or (s2 is not None and s2 < 0.25):
                vertical_ok = True
                break

        # 최종 판정: (왼쪽 또는 오른쪽 다리 펴짐) 또는 수직 정렬 good
        legs_ok = (left_ok is True) or (right_ok is True)
        is_ok = legs_ok or vertical_ok

        score = 100 if is_ok else 0
        fb = [] if is_ok else ["Stand sideways with legs straight"]
        return {'status': 'success', 'is_correct': is_ok, 'score': score, 'feedback': "Ready position complete!" if is_ok else ", ".join(fb)}

    def _analyze_lunge_left_side(self, xy, conf, bbox=None):
        th = AIServerConfig.CONFIDENCE_THRESHOLD
        Lh, Lk, La = xy[11] if conf[11] > th else None, xy[13] if conf[13] > th else None, xy[15] if conf[15] > th else None
        if any(p is None for p in [Lh, Lk, La]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position left leg in view (side)'}
        ang = self._angle(Lh, Lk, La)
        if ang is None:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn, mx = AIServerConfig.LUNGE_LEFT_FRONT_KNEE_MIN, AIServerConfig.LUNGE_LEFT_FRONT_KNEE_MAX
        ok = (mn <= ang <= mx)
        return {'status': 'success', 'is_correct': ok, 'score': 100 if ok else 0, 'feedback': "Perfect left lunge!" if ok else f"Adjust left knee angle ({ang:.0f}deg)"}

    def _analyze_lunge_right_side(self, xy, conf, bbox=None):
        th = AIServerConfig.CONFIDENCE_THRESHOLD
        Rh, Rk, Ra = xy[12] if conf[12] > th else None, xy[14] if conf[14] > th else None, xy[16] if conf[16] > th else None
        if any(p is None for p in [Rh, Rk, Ra]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position right leg in view (side)'}
        ang = self._angle(Rh, Rk, Ra)
        if ang is None:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn, mx = AIServerConfig.LUNGE_RIGHT_FRONT_KNEE_MIN, AIServerConfig.LUNGE_RIGHT_FRONT_KNEE_MAX
        ok = (mn <= ang <= mx)
        return {'status': 'success', 'is_correct': ok, 'score': 100 if ok else 0, 'feedback': "Perfect right lunge!" if ok else f"Adjust right knee angle ({ang:.0f}deg)"}

    def _analyze_curl_down(self, xy, conf, bbox=None):
        th = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > th else None, xy[6] if conf[6] > th else None
        Le, Re = xy[7] if conf[7] > th else None, xy[8] if conf[8] > th else None
        Lw, Rw = xy[9] if conf[9] > th else None, xy[10] if conf[10] > th else None
        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn = AIServerConfig.CURL_DOWN_ELBOW_ANGLE_MIN
        L_ok, R_ok = (L_ang > mn), (R_ang > mn)
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok or not R_ok: fb.append("Start with arms extended")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Starting position complete!" if score == 100 else ", ".join(fb)}

    def _analyze_curl_up(self, xy, conf, bbox=None):
        th = AIServerConfig.CONFIDENCE_THRESHOLD
        Ls, Rs = xy[5] if conf[5] > th else None, xy[6] if conf[6] > th else None
        Le, Re = xy[7] if conf[7] > th else None, xy[8] if conf[8] > th else None
        Lw, Rw = xy[9] if conf[9] > th else None, xy[10] if conf[10] > th else None
        if any(p is None for p in [Ls, Rs, Le, Re, Lw, Rw]):
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Position upper body in view'}
        L_ang = self._angle(Ls, Le, Lw)
        R_ang = self._angle(Rs, Re, Rw)
        if None in [L_ang, R_ang]:
            return {'status': 'success', 'is_correct': False, 'score': 0, 'feedback': 'Angle calculation failed'}
        mn, mx = AIServerConfig.CURL_UP_ELBOW_ANGLE_MIN, AIServerConfig.CURL_UP_ELBOW_ANGLE_MAX
        L_ok, R_ok = (mn <= L_ang <= mx), (mn <= R_ang <= mx)
        score = (50 if L_ok else 0) + (50 if R_ok else 0)
        fb = []
        if not L_ok or not R_ok: fb.append("Curl elbows")
        return {'status': 'success', 'is_correct': score == 100, 'score': score, 'feedback': "Perfect curl!" if score == 100 else ", ".join(fb)}

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
