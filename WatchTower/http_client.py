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
            mode: 운동 모드 ('t_pose', 'squat', 'pushup', etc.)

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
                self.current_mode = mode
                return {
                    'success': True,
                    'message': result.get('message', 'Mode selected successfully'),
                    'mode': mode
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

    def send_frame(self, frame):
        """
        프레임을 AI 서버로 전송하고 분석 결과 수신

        Args:
            frame: OpenCV 프레임 (numpy 배열)

        Returns:
            dict: 분석 결과 또는 None
        """
        try:
            import cv2

            # JPEG 인코딩
            encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), self.config.JPEG_QUALITY]
            result, encoded_frame = cv2.imencode('.jpg', frame, encode_param)

            if not result:
                print("✗ 프레임 인코딩 실패")
                return None

            # Base64 인코딩
            frame_base64 = base64.b64encode(encoded_frame).decode('utf-8')

            # API 요청
            url = f"{self.server_url}{self.config.API_STREAM_FRAME}"
            payload = {
                "frame": frame_base64,
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
    result = client.select_mode('t_pose')
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
