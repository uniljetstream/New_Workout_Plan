#!/usr/bin/env python3
"""
HTTP Client for WatchTower
AI 서버와 HTTP 통신 담당
"""

import requests
import base64
import time
from watchtower_config import WatchTowerConfig


class HTTPClient:
    """AI 서버 HTTP 통신 클라이언트"""

    def __init__(self):
        """초기화"""
        self.config = WatchTowerConfig
        self.server_url = f"http://{self.config.AI_SERVER_HOST}:{self.config.AI_SERVER_PORT}"
        self.current_mode = None
        self.current_pose_index = 0  # 현재 포즈 인덱스
        self.poses = []  # 현재 모드의 포즈 리스트
        self.total_poses = 0  # 총 포즈 개수

    def check_server(self):
        """
        AI 서버 연결 확인

        Returns:
            bool: 서버 연결 성공 여부
        """
        try:
            url = f"{self.server_url}{self.config.API_HEALTH}"
            response = requests.get(url, timeout=self.config.REQUEST_TIMEOUT)

            if response.status_code == 200:
                print(f"✓ AI 서버 연결 성공: {self.server_url}")
                return True
            else:
                print(f"✗ AI 서버 응답 오류: {response.status_code}")
                return False

        except requests.exceptions.RequestException as e:
            print(f"✗ AI 서버 연결 실패: {e}")
            return False

    def select_mode(self, mode):
        """
        운동 모드 선택

        Args:
            mode: 운동 모드 ('squat', 'pushup', etc.)

        Returns:
            dict: 성공 시 {'success': True, 'message': '...'}, 실패 시 {'success': False, 'error': '...'}
        """
        try:
            url = f"{self.server_url}{self.config.API_MODE_SELECT}"
            payload = {"mode": mode}

            print(f"→ AI 서버에 운동 모드 전송: {mode}")
            response = requests.post(url, json=payload, timeout=self.config.REQUEST_TIMEOUT)

            if response.status_code == 200:
                result = response.json()
                print(f"✓ AI 서버 응답: {result.get('message', 'OK')}")

                # 모드 및 포즈 정보 저장
                self.current_mode = mode
                self.current_pose_index = 0
                self.poses = result.get('poses', [])
                self.total_poses = result.get('total_poses', 0)

                print(f"  총 포즈 개수: {self.total_poses}")
                for i, pose in enumerate(self.poses):
                    print(f"  [{i}] {pose.get('description', 'N/A')}")

                return {
                    'success': True,
                    'message': result.get('message', 'Mode selected successfully'),
                    'mode': mode,
                    'poses': self.poses,
                    'total_poses': self.total_poses
                }
            else:
                error = response.json() if response.content else {}
                error_msg = error.get('message', f'HTTP {response.status_code}')
                print(f"✗ 모드 선택 실패: {error_msg}")
                return {
                    'success': False,
                    'error': error_msg
                }

        except requests.exceptions.Timeout:
            error_msg = "서버 응답 타임아웃"
            print(f"✗ {error_msg}")
            return {
                'success': False,
                'error': error_msg
            }
        except requests.exceptions.ConnectionError:
            error_msg = "서버 연결 실패"
            print(f"✗ {error_msg}")
            return {
                'success': False,
                'error': error_msg
            }
        except Exception as e:
            error_msg = f"모드 선택 오류: {str(e)}"
            print(f"✗ {error_msg}")
            return {
                'success': False,
                'error': error_msg
            }

    def send_frame(self, frame=None, pose_index=None, frame_base64=None):
        """
        프레임을 AI 서버로 전송하고 분석 결과 수신

        Args:
            frame: OpenCV 프레임 (numpy 배열)
            pose_index: 포즈 인덱스 (None이면 현재 인덱스 사용)
            frame_base64: 이미 인코딩된 프레임(Base64). 제공되면 frame 인코딩을 건너뜀.

        Returns:
            dict: 분석 결과 또는 None
        """
        try:
            if frame_base64 is None:
                if frame is None:
                    print("✗ 프레임 데이터가 없습니다 (frame 또는 frame_base64 필요)")
                    return None

                import cv2

                # JPEG 인코딩
                encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), self.config.JPEG_QUALITY]
                result, encoded_frame = cv2.imencode('.jpg', frame, encode_param)

                if not result:
                    print("✗ 프레임 인코딩 실패")
                    return None

                # Base64 인코딩
                frame_base64 = base64.b64encode(encoded_frame).decode('utf-8')

            # 포즈 인덱스 결정
            if pose_index is None:
                pose_index = self.current_pose_index

            # API 요청
            url = f"{self.server_url}{self.config.API_STREAM_FRAME}"
            payload = {
                "frame": frame_base64,
                "pose_index": pose_index,
                "timestamp": int(time.time() * 1000)
            }

            response = requests.post(url, json=payload, timeout=self.config.REQUEST_TIMEOUT)

            if response.status_code == 200:
                return response.json()
            else:
                print(f"✗ 프레임 분석 실패: {response.status_code}")
                return None

        except requests.exceptions.Timeout:
            print("✗ 프레임 전송 타임아웃")
            return None
        except Exception as e:
            print(f"✗ 프레임 전송 오류: {e}")
            return None

    def set_pose_index(self, pose_index):
        """
        현재 포즈 인덱스 설정

        Args:
            pose_index: 포즈 인덱스 (0부터 시작)

        Returns:
            bool: 설정 성공 여부
        """
        if pose_index < 0 or pose_index >= self.total_poses:
            print(f"✗ 잘못된 포즈 인덱스: {pose_index} (범위: 0-{self.total_poses-1})")
            return False

        self.current_pose_index = pose_index
        if pose_index < len(self.poses):
            pose = self.poses[pose_index]
            print(f"✓ 포즈 변경: [{pose_index}] {pose.get('description', 'N/A')}")
        return True

    def get_current_pose_info(self):
        """
        현재 포즈 정보 가져오기

        Returns:
            dict: 포즈 정보 또는 None
        """
        if self.current_pose_index < len(self.poses):
            return self.poses[self.current_pose_index]
        return None

    def stop_stream(self):
        """
        스트리밍 중단 요청

        Returns:
            bool: 중단 성공 여부
        """
        try:
            url = f"{self.server_url}{self.config.API_STREAM_STOP}"
            payload = {"mode": self.current_mode}

            print(f"→ 스트리밍 중단 요청")
            response = requests.post(url, json=payload, timeout=self.config.REQUEST_TIMEOUT)

            if response.status_code == 200:
                result = response.json()
                print(f"✓ 서버 응답: {result.get('message', 'OK')}")
                return True
            else:
                print(f"✗ 스트리밍 중단 실패: {response.status_code}")
                return False

        except Exception as e:
            print(f"✗ 스트리밍 중단 오류: {e}")
            return False

    def get_server_status(self):
        """
        서버 상태 조회

        Returns:
            dict: 서버 상태 정보 또는 None
        """
        try:
            url = f"{self.server_url}{self.config.API_STATUS}"
            response = requests.get(url, timeout=self.config.REQUEST_TIMEOUT)

            if response.status_code == 200:
                return response.json()
            else:
                return None

        except Exception as e:
            print(f"✗ 서버 상태 조회 오류: {e}")
            return None


def main():
    """테스트용 메인 함수"""
    print("="*50)
    print("  HTTP Client 테스트")
    print("="*50)

    client = HTTPClient()

    # 서버 연결 확인
    print("\n[1] AI 서버 연결 확인")
    if client.check_server():
        print("✓ 연결 테스트 성공\n")
    else:
        print("✗ 연결 테스트 실패\n")
        return

    # 모드 선택 테스트
    print("[2] 운동 모드 선택 테스트")
    result = client.select_mode('squat')
    if result['success']:
        print(f"✓ 모드 선택 성공: {result['mode']}\n")
    else:
        print(f"✗ 모드 선택 실패: {result['error']}\n")

    # 서버 상태 조회
    print("[3] 서버 상태 조회")
    status = client.get_server_status()
    if status:
        print(f"✓ 서버 상태: {status}\n")
    else:
        print("✗ 상태 조회 실패\n")


if __name__ == "__main__":
    main()
