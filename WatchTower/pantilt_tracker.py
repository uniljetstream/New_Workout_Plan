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

        # 추적 활성화 상태
        self.is_tracking = False
        self.frames_without_detection = 0
        self.max_frames_without_detection = 10

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
            if 'center_x' in tracking and 'center_y' in tracking:
                return (tracking['center_x'], tracking['center_y'])

        # 2. keypoints로부터 직접 계산 (AI 서버 응답에 keypoints가 있는 경우)
        if 'keypoints' in keypoints_data:
            kp = keypoints_data['keypoints']

            # 주요 포인트: nose(0), left_shoulder(5), right_shoulder(6)
            # 얼굴과 어깨의 평균 위치를 추적
            x_points = []
            y_points = []

            # 이 부분은 AI 서버가 키포인트 좌표를 직접 제공하는 경우를 위한 것
            # 현재 구현에서는 각도만 제공되므로, tracking 데이터 추가가 필요
            pass

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
            self.frames_without_detection += 1
            if self.frames_without_detection > self.max_frames_without_detection:
                self.is_tracking = False
            return False

        # tracking 데이터 확인
        target_pos = None

        if 'tracking' in analysis_result:
            tracking = analysis_result['tracking']
            if 'center_x' in tracking and 'center_y' in tracking:
                target_pos = (tracking['center_x'], tracking['center_y'])
            elif 'bbox' in tracking:
                target_pos = self.calculate_target_from_bbox(tracking['bbox'])

        if target_pos:
            self.target_x, self.target_y = target_pos

            # 스무딩을 위한 버퍼에 추가
            self.position_buffer_x.append(self.target_x)
            self.position_buffer_y.append(self.target_y)

            # 추적 활성화
            self.is_tracking = True
            self.frames_without_detection = 0
            return True
        else:
            self.frames_without_detection += 1
            if self.frames_without_detection > self.max_frames_without_detection:
                self.is_tracking = False
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

        # 프레임 중앙과의 차이 계산
        error_x = target_x - self.frame_center_x
        error_y = target_y - self.frame_center_y

        # Dead zone 체크 (중앙 근처는 움직이지 않음)
        if abs(error_x) < self.dead_zone_x:
            error_x = 0
        if abs(error_y) < self.dead_zone_y:
            error_y = 0

        # 비례 제어로 각도 변화량 계산
        # 프레임 절반 거리 = TRACKING_SPEED 도 변화
        pan_delta = -(error_x / (self.frame_width / 2)) * WatchTowerConfig.TRACKING_SPEED
        tilt_delta = (error_y / (self.frame_height / 2)) * WatchTowerConfig.TRACKING_SPEED

        # 각도 변화량 제한 (급격한 움직임 방지)
        max_delta = WatchTowerConfig.TRACKING_MAX_DELTA
        pan_delta = np.clip(pan_delta, -max_delta, max_delta)
        tilt_delta = np.clip(tilt_delta, -max_delta, max_delta)

        # 새로운 각도 계산
        new_pan = self.current_pan + pan_delta
        new_tilt = self.current_tilt + tilt_delta

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
            'frames_without_detection': self.frames_without_detection
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
