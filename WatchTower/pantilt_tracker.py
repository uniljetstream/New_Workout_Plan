#!/usr/bin/env python3
"""
팬틸트 추적 모듈
YOLO 키포인트 기반으로 사람을 추적하여 팬틸트 각도를 계산
"""

import numpy as np
from collections import deque
from watchtower_config import WatchTowerConfig


class PanTiltTracker:
    """YOLO 키포인트 기반 팬틸트 추적 클래스"""

    def __init__(self, frame_width=640, frame_height=480):
        """
        초기화

        Args:
            frame_width: 프레임 너비
            frame_height: 프레임 높이
        """
        self.frame_width = frame_width
        self.frame_height = frame_height
        self.frame_center_x = frame_width / 2
        self.frame_center_y = frame_height / 2

        # 현재 팬틸트 각도 (중앙 위치로 초기화)
        self.current_pan = WatchTowerConfig.PAN_CENTER
        self.current_tilt = WatchTowerConfig.TILT_CENTER

        # 추적 대상 위치 (프레임 좌표계)
        self.target_x = self.frame_center_x
        self.target_y = self.frame_center_y

        # 스무딩을 위한 이동 평균 버퍼
        self.position_buffer_size = WatchTowerConfig.TRACKING_SMOOTH_FRAMES
        self.position_buffer_x = deque(maxlen=self.position_buffer_size)
        self.position_buffer_y = deque(maxlen=self.position_buffer_size)

        # Dead zone (중앙 근처는 움직이지 않음)
        self.dead_zone_x = WatchTowerConfig.TRACKING_DEAD_ZONE_X
        self.dead_zone_y = WatchTowerConfig.TRACKING_DEAD_ZONE_Y
        self.keypoint_min_conf = WatchTowerConfig.TRACKING_KEYPOINT_MIN_CONF

        # Y축 추적 오프셋 (전신 운동 시 상체를 더 위로 추적)
        self.y_offset = WatchTowerConfig.TRACKING_Y_OFFSET

        # Tilt 추적 활성화 여부 (False면 센터 고정)
        self.tilt_enabled = WatchTowerConfig.TRACKING_TILT_ENABLED

        # 추적 활성화 상태
        self.is_tracking = False
        self.frames_without_detection = 0
        self.max_frames_without_detection = WatchTowerConfig.TRACKING_RETRY_LIMIT
        self.retries_exhausted = False

    def calculate_target_position(self, keypoints_data):
        """
        키포인트 데이터에서 추적 대상 위치 계산

        Args:
            keypoints_data: AI 서버로부터 받은 keypoints 또는 tracking 데이터
                - tracking 데이터가 있으면 사용 (center_x, center_y)
                - 없으면 keypoints로부터 얼굴/상체 중심 계산

        Returns:
            tuple: (target_x, target_y) 또는 None
        """
        # 1. tracking 데이터가 있는 경우 (AI 서버에서 제공)
        if 'tracking' in keypoints_data:
            tracking = keypoints_data['tracking']
            if isinstance(tracking, dict):
                if 'center_x' in tracking and 'center_y' in tracking:
                    return (tracking['center_x'], tracking['center_y'])
                if 'bbox' in tracking:
                    target_from_bbox = self.calculate_target_from_bbox(tracking['bbox'])
                    if target_from_bbox:
                        return target_from_bbox

        # 2. keypoints로부터 직접 계산 (AI 서버 응답에 keypoints가 있는 경우)
        if 'keypoints' in keypoints_data:
            kp = keypoints_data['keypoints']
            if isinstance(kp, dict):
                xy = kp.get('xy')
                conf = kp.get('conf')
                if xy is not None and conf is not None:
                    xy = np.array(xy)
                    conf = np.array(conf)
                    torso_center = self._calculate_torso_center_from_keypoints(xy, conf)
                    if torso_center is not None:
                        return torso_center

        return None

    def calculate_target_from_bbox(self, bbox):
        """
        바운딩 박스로부터 중심점 계산

        Args:
            bbox: [x1, y1, x2, y2] 형식의 바운딩 박스

        Returns:
            tuple: (center_x, center_y)
        """
        if bbox and len(bbox) == 4:
            x1, y1, x2, y2 = bbox
            center_x = (x1 + x2) / 2
            center_y = (y1 + y2) / 2
            return (center_x, center_y)
        return None

    def update(self, analysis_result):
        """
        분석 결과를 기반으로 추적 상태 업데이트

        Args:
            analysis_result: AI 서버로부터 받은 분석 결과
                {
                    'status': 'success',
                    'tracking': {
                        'center_x': 320,
                        'center_y': 240,
                        'bbox': [100, 50, 540, 430]
                    }
                }

        Returns:
            bool: 추적 대상이 감지되었는지 여부
        """
        if not analysis_result or analysis_result.get('status') != 'success':
            self._handle_detection_miss()
            return False

        target_pos = self.calculate_target_position(analysis_result)

        if target_pos:
            self.target_x, self.target_y = target_pos

            # 스무딩을 위한 버퍼에 추가
            self.position_buffer_x.append(self.target_x)
            self.position_buffer_y.append(self.target_y)

            # 추적 활성화
            self.is_tracking = True
            self.frames_without_detection = 0
            self.retries_exhausted = False
            return True
        else:
            self._handle_detection_miss()
            return False

    def get_smoothed_target(self):
        """
        스무딩된 추적 대상 위치 반환

        Returns:
            tuple: (smoothed_x, smoothed_y)
        """
        if len(self.position_buffer_x) == 0:
            return (self.frame_center_x, self.frame_center_y)

        smoothed_x = np.mean(self.position_buffer_x)
        smoothed_y = np.mean(self.position_buffer_y)

        return (smoothed_x, smoothed_y)

    def calculate_pan_tilt_angles(self):
        """
        현재 추적 대상 위치를 기반으로 팬틸트 각도 계산

        Returns:
            tuple: (pan_angle, tilt_angle)
        """
        if not self.is_tracking:
            # 추적 중이 아니면 현재 각도 유지
            return (self.current_pan, self.current_tilt)

        # 스무딩된 대상 위치
        target_x, target_y = self.get_smoothed_target()

        # 프레임 중앙과의 차이 계산 (Pan만)
        error_x = target_x - self.frame_center_x

        # Dead zone 체크 (Pan만)
        if abs(error_x) < self.dead_zone_x:
            error_x = 0

        # 비례 제어로 각도 변화량 계산 (Pan만)
        pan_delta = -(error_x / (self.frame_width / 2)) * WatchTowerConfig.TRACKING_SPEED

        # 각도 변화량 제한 (Pan만)
        max_delta = WatchTowerConfig.TRACKING_MAX_DELTA
        pan_delta = np.clip(pan_delta, -max_delta, max_delta)

        # 새로운 각도 계산
        new_pan = self.current_pan + pan_delta

        # Tilt 처리: 활성화 여부에 따라 분기
        if self.tilt_enabled:
            # Tilt 추적 활성화: 기존 로직 사용
            # Y축 오프셋 적용
            adjusted_target_y = target_y + self.y_offset
            error_y = adjusted_target_y - self.frame_center_y

            # Dead zone 체크
            if abs(error_y) < self.dead_zone_y:
                error_y = 0

            # 비례 제어로 각도 변화량 계산
            tilt_delta = (error_y / (self.frame_height / 2)) * WatchTowerConfig.TRACKING_SPEED
            tilt_delta = np.clip(tilt_delta, -max_delta, max_delta)

            new_tilt = self.current_tilt + tilt_delta
        else:
            # Tilt 추적 비활성화: 센터 고정
            new_tilt = WatchTowerConfig.TILT_CENTER

        # 각도 범위 제한
        new_pan = np.clip(new_pan, WatchTowerConfig.PAN_MIN, WatchTowerConfig.PAN_MAX)
        new_tilt = np.clip(new_tilt, WatchTowerConfig.TILT_MIN, WatchTowerConfig.TILT_MAX)

        # 현재 각도 업데이트
        self.current_pan = new_pan
        self.current_tilt = new_tilt

        return (new_pan, new_tilt)

    def get_current_angles(self):
        """
        현재 팬틸트 각도 반환

        Returns:
            tuple: (pan_angle, tilt_angle)
        """
        return (self.current_pan, self.current_tilt)

    def reset(self):
        """추적 상태 초기화"""
        self.current_pan = WatchTowerConfig.PAN_CENTER
        self.current_tilt = WatchTowerConfig.TILT_CENTER
        self.target_x = self.frame_center_x
        self.target_y = self.frame_center_y
        self.position_buffer_x.clear()
        self.position_buffer_y.clear()
        self.is_tracking = False
        self.frames_without_detection = 0
        self.retries_exhausted = False

    def has_retries_remaining(self):
        """재탐지 시도 가능 여부"""
        return self.frames_without_detection < self.max_frames_without_detection

    def has_exhausted_retries(self):
        """재탐지 시도 소진 여부"""
        return self.retries_exhausted

    def get_retry_count(self):
        """현재까지 재탐지 시도 횟수 반환"""
        return self.frames_without_detection

    def _handle_detection_miss(self):
        """감지 실패 시 상태 업데이트"""
        self.frames_without_detection += 1
        self.is_tracking = False

        # 사람이 감지되지 않으면 버퍼 클리어 (각도 유지)
        # 버퍼에 이전 데이터가 남아있으면 Y축 오프셋이 계속 적용되어 위로 움직임
        self.position_buffer_x.clear()
        self.position_buffer_y.clear()

        if self.frames_without_detection >= self.max_frames_without_detection:
            self.retries_exhausted = True

    def _calculate_torso_center_from_keypoints(self, xy, conf):
        """
        키포인트에서 상체 상단(코/목/어깨) 중심을 사용하여 추적 대상 계산
        전신 운동 시 상체 상단을 추적하여 틸트가 아래로 쏠리는 문제 방지

        Args:
            xy: (17, 2) 키포인트 좌표 배열
            conf: (17,) 키포인트 confidence 배열

        Returns:
            tuple: (center_x, center_y) 또는 None
        """
        if xy.shape[0] < 17 or conf.shape[0] < 17:
            return None

        # 상체 상단 키포인트 수집 (우선순위: 코 > 목 > 어깨)
        upper_body_points = []

        # 1순위: 코 (0번 키포인트) - 가장 위쪽
        if conf[0] >= self.keypoint_min_conf:
            upper_body_points.append(xy[0])

        # 2순위: 목 (목은 어깨 중간점으로 근사)
        valid_shoulders = []
        for idx in (5, 6):  # 왼쪽 어깨(5), 오른쪽 어깨(6)
            if conf[idx] >= self.keypoint_min_conf:
                valid_shoulders.append(xy[idx])

        if valid_shoulders:
            shoulder_center = np.mean(valid_shoulders, axis=0)
            upper_body_points.append(shoulder_center)

        # 상체 상단 키포인트가 없으면 None 반환
        if not upper_body_points:
            return None

        # 상체 상단 중심 계산 (코 + 어깨 중심의 평균)
        upper_body_center = np.mean(upper_body_points, axis=0)
        return (float(upper_body_center[0]), float(upper_body_center[1]))

    def get_tracking_info(self):
        """
        디버깅용 추적 정보 반환

        Returns:
            dict: 추적 상태 정보
        """
        smoothed_x, smoothed_y = self.get_smoothed_target()

        return {
            'is_tracking': self.is_tracking,
            'target_x': int(self.target_x),
            'target_y': int(self.target_y),
            'smoothed_x': int(smoothed_x),
            'smoothed_y': int(smoothed_y),
            'current_pan': int(self.current_pan),
            'current_tilt': int(self.current_tilt),
            'error_x': int(self.target_x - self.frame_center_x),
            'error_y': int(self.target_y - self.frame_center_y),
            'frames_without_detection': self.frames_without_detection,
            'retry_limit': self.max_frames_without_detection,
            'retries_exhausted': self.retries_exhausted
        }


def main():
    """테스트용 메인 함수"""
    print("=" * 50)
    print("  팬틸트 추적 테스트")
    print("=" * 50)

    # 추적기 초기화
    tracker = PanTiltTracker(frame_width=640, frame_height=480)

    # 테스트 시나리오: 사람이 오른쪽으로 이동
    test_positions = [
        {'tracking': {'center_x': 320, 'center_y': 240}},  # 중앙
        {'tracking': {'center_x': 400, 'center_y': 240}},  # 오른쪽으로
        {'tracking': {'center_x': 480, 'center_y': 240}},  # 더 오른쪽으로
        {'tracking': {'center_x': 500, 'center_y': 200}},  # 오른쪽 위로
        {'tracking': {'center_x': 500, 'center_y': 200}},  # 유지
        {'tracking': {'center_x': 320, 'center_y': 240}},  # 중앙 복귀
    ]

    print("\n추적 시뮬레이션:")
    for i, result in enumerate(test_positions):
        result['status'] = 'success'
        tracker.update(result)
        pan, tilt = tracker.calculate_pan_tilt_angles()

        info = tracker.get_tracking_info()
        print(f"\n[프레임 {i+1}]")
        print(f"  대상 위치: ({info['target_x']}, {info['target_y']})")
        print(f"  스무딩 위치: ({info['smoothed_x']}, {info['smoothed_y']})")
        print(f"  오차: ({info['error_x']}, {info['error_y']})")
        print(f"  팬틸트 각도: Pan={pan:.1f}°, Tilt={tilt:.1f}°")
        print(f"  추적 상태: {'활성' if info['is_tracking'] else '비활성'}")

    print("\n✓ 테스트 완료!")


if __name__ == "__main__":
    main()
