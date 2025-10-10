#!/usr/bin/env python3
"""
WatchTower 클라이언트
AI 서버에 운동 모드 선택 및 영상 스트리밍을 수행하고 실시간 분석 결과를 수신
"""

import cv2
import requests
import base64
import time
import numpy as np
from watchtower_config import WatchTowerConfig
from uart_controller import UARTController
from pantilt_tracker import PanTiltTracker


class WatchTowerClient:
    """WatchTower HTTP 클라이언트"""

    def __init__(self):
        """초기화"""
        self.config = WatchTowerConfig
        self.server_url = f"http://{self.config.AI_SERVER_HOST}:{self.config.AI_SERVER_PORT}"
        self.camera = None
        self.current_mode = None
        self.is_streaming = False

        # 팬틸트 제어 초기화
        self.uart = None
        self.tracker = None
        self.pantilt_enabled = self.config.PANTILT_ENABLED

    def check_server(self):
        """
        AI 서버 연결 확인

        Returns:
            bool: 서버 연결 성공 여부
        """
        try:
            url = f"{self.server_url}/api/health"
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
        운동 모드 선택 (Step 1)

        Args:
            mode: 운동 모드 ('t_pose', 'squat', etc.)

        Returns:
            bool: 모드 선택 성공 여부
        """
        try:
            url = f"{self.server_url}{self.config.API_MODE_SELECT}"
            payload = {"mode": mode}

            print(f"→ 운동 모드 선택 요청: {mode}")
            response = requests.post(url, json=payload, timeout=self.config.REQUEST_TIMEOUT)

            if response.status_code == 200:
                result = response.json()
                print(f"✓ 서버 응답: {result['message']}")
                self.current_mode = mode
                return True
            else:
                error = response.json()
                print(f"✗ 모드 선택 실패: {error.get('message', 'Unknown error')}")
                return False

        except Exception as e:
            print(f"✗ 모드 선택 오류: {e}")
            return False

    def initialize_camera(self):
        """
        카메라 초기화

        Returns:
            bool: 카메라 초기화 성공 여부
        """
        try:
            self.camera = cv2.VideoCapture(self.config.CAMERA_ID)

            if not self.camera.isOpened():
                print(f"✗ 카메라 열기 실패: {self.config.CAMERA_ID}")
                return False

            # 카메라 속성 설정
            self.camera.set(cv2.CAP_PROP_FRAME_WIDTH, self.config.CAMERA_WIDTH)
            self.camera.set(cv2.CAP_PROP_FRAME_HEIGHT, self.config.CAMERA_HEIGHT)
            self.camera.set(cv2.CAP_PROP_FPS, self.config.CAMERA_FPS)

            actual_width = int(self.camera.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self.camera.get(cv2.CAP_PROP_FRAME_HEIGHT))

            print(f"✓ 카메라 초기화 완료: {actual_width}x{actual_height}")

            # 팬틸트 추적기 초기화 (카메라 해상도 기반)
            if self.pantilt_enabled:
                self.tracker = PanTiltTracker(
                    frame_width=actual_width,
                    frame_height=actual_height
                )
                print(f"✓ 팬틸트 추적기 초기화 완료")

            return True

        except Exception as e:
            print(f"✗ 카메라 초기화 실패: {e}")
            return False

    def initialize_pantilt(self):
        """
        팬틸트 UART 초기화

        Returns:
            bool: 초기화 성공 여부
        """
        if not self.pantilt_enabled:
            print("ℹ 팬틸트 제어 비활성화됨")
            return True

        try:
            self.uart = UARTController()
            if self.uart.connect():
                print(f"✓ 팬틸트 제어 초기화 완료")
                return True
            else:
                print(f"✗ 팬틸트 UART 연결 실패 (계속 진행)")
                self.pantilt_enabled = False
                return False

        except Exception as e:
            print(f"✗ 팬틸트 초기화 오류: {e} (계속 진행)")
            self.pantilt_enabled = False
            return False

    def send_frame(self, frame):
        """
        프레임을 AI 서버로 전송하고 분석 결과 수신 (Step 3, 4)

        Args:
            frame: OpenCV 프레임 (numpy 배열)

        Returns:
            dict: 분석 결과 또는 None
        """
        try:
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
            print("✗ 요청 타임아웃")
            return None
        except Exception as e:
            print(f"✗ 프레임 전송 오류: {e}")
            return None

    def stop_stream(self):
        """
        스트리밍 중단 요청 (Step 5)

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
                print(f"✓ 서버 응답: {result['message']}")
                self.is_streaming = False
                return True
            else:
                print(f"✗ 스트리밍 중단 실패: {response.status_code}")
                return False

        except Exception as e:
            print(f"✗ 스트리밍 중단 오류: {e}")
            return False

    def start_streaming(self, mode='t_pose', duration=None):
        """
        전체 스트리밍 프로세스 실행

        Args:
            mode: 운동 모드
            duration: 스트리밍 지속 시간 (초, None이면 무제한)
        """
        # 서버 연결 확인
        if not self.check_server():
            print("✗ AI 서버에 연결할 수 없습니다")
            return

        # Step 1: 운동 모드 선택
        if not self.select_mode(mode):
            print("✗ 운동 모드 선택 실패")
            return

        # 카메라 초기화
        if not self.initialize_camera():
            print("✗ 카메라 초기화 실패")
            return

        # 팬틸트 초기화
        self.initialize_pantilt()

        print(f"\n{'='*50}")
        print(f"  스트리밍 시작: {mode}")
        print(f"  목표 FPS: {self.config.STREAM_FPS}")
        if duration:
            print(f"  지속 시간: {duration}초")
        print(f"  팬틸트 제어: {'활성화' if self.pantilt_enabled else '비활성화'}")
        print(f"  종료: 'q' 키")
        print(f"{'='*50}\n")

        self.is_streaming = True
        frame_interval = 1.0 / self.config.STREAM_FPS
        frame_count = 0
        start_time = time.time()
        last_result = None

        try:
            while self.is_streaming:
                loop_start = time.time()

                # 프레임 캡처
                ret, frame = self.camera.read()
                if not ret:
                    print("✗ 프레임 캡처 실패")
                    break

                # Step 3, 4: 프레임 전송 및 분석 결과 수신
                result = self.send_frame(frame)

                if result and result.get('status') == 'success':
                    last_result = result
                    frame_count += 1

                    # 결과 출력
                    score = result.get('score', 0)
                    feedback = result.get('feedback', '')
                    is_correct = result.get('is_correct', False)

                    status = "✓ 정확" if is_correct else "✗ 조정 필요"
                    print(f"[{frame_count:04d}] {status} | 점수: {score}% | {feedback}", end='')

                    # 팬틸트 추적 업데이트
                    if self.pantilt_enabled and self.tracker and self.uart:
                        # 추적 정보 업데이트
                        if self.tracker.update(result):
                            # 새로운 팬틸트 각도 계산
                            pan, tilt = self.tracker.calculate_pan_tilt_angles()

                            # UART로 각도 전송
                            self.uart.send_pan_tilt(pan, tilt)

                            # 추적 정보 출력
                            track_info = self.tracker.get_tracking_info()
                            print(f" | 추적: ({track_info['target_x']}, {track_info['target_y']}) | Pan={pan:.0f}° Tilt={tilt:.0f}°", end='')

                    print()  # 개행

                # 프리뷰 표시
                if self.config.SHOW_PREVIEW and last_result:
                    self._draw_overlay(frame, last_result)
                    cv2.imshow(self.config.WINDOW_NAME, frame)

                # 종료 조건 확인
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    print("\n⏸ 사용자에 의해 중지됨")
                    break

                if duration and (time.time() - start_time) >= duration:
                    print(f"\n⏱ {duration}초 완료")
                    break

                # FPS 유지
                elapsed = time.time() - loop_start
                sleep_time = frame_interval - elapsed
                if sleep_time > 0:
                    time.sleep(sleep_time)

        except KeyboardInterrupt:
            print("\n⏸ 사용자에 의해 중지됨")
        except Exception as e:
            print(f"\n✗ 스트리밍 오류: {e}")
        finally:
            # Step 5: 스트리밍 중단
            self.stop_stream()
            self.cleanup()

            # 통계 출력
            elapsed_total = time.time() - start_time
            if frame_count > 0:
                avg_fps = frame_count / elapsed_total
                print(f"\n📊 통계:")
                print(f"  총 프레임: {frame_count}")
                print(f"  평균 FPS: {avg_fps:.2f}")
                print(f"  총 시간: {elapsed_total:.1f}초")

    def _draw_overlay(self, frame, result):
        """
        프레임에 분석 결과 오버레이

        Args:
            frame: OpenCV 프레임
            result: 분석 결과
        """
        score = result.get('score', 0)
        feedback = result.get('feedback', '')
        is_correct = result.get('is_correct', False)

        # 색상 결정
        if score >= 80:
            color = (0, 255, 0)  # 초록
        elif score >= 50:
            color = (0, 255, 255)  # 노랑
        else:
            color = (0, 0, 255)  # 빨강

        # 점수 표시
        cv2.putText(frame, f"Score: {score}%",
                    (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1.0, color, 3)

        # 피드백 표시
        cv2.putText(frame, feedback,
                    (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

        # 완벽한 자세일 때 강조
        if is_correct:
            cv2.putText(frame, "PERFECT!",
                        (10, 110), cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 255, 0), 4)

    def cleanup(self):
        """리소스 해제"""
        print("\n🧹 정리 중...")

        if self.camera is not None:
            self.camera.release()
            print("✓ 카메라 해제 완료")

        if self.uart is not None:
            self.uart.disconnect()

        cv2.destroyAllWindows()
        print("✓ 창 닫기 완료")


def main():
    """메인 진입점"""
    print("=" * 50)
    print("  WatchTower 클라이언트")
    print("=" * 50)
    print(f"AI 서버: {WatchTowerConfig.AI_SERVER_HOST}:{WatchTowerConfig.AI_SERVER_PORT}")
    print(f"카메라: /dev/video{WatchTowerConfig.CAMERA_ID}")
    print(f"스트림 FPS: {WatchTowerConfig.STREAM_FPS}")
    print("=" * 50)

    client = WatchTowerClient()

    # 예제: T자 서기 30초 스트리밍
    client.start_streaming(mode='t_pose', duration=30)


if __name__ == "__main__":
    main()
