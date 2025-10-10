#!/usr/bin/env python3
"""
Jetson Nano USB 카메라 스트리밍 클라이언트
USB 카메라에서 영상을 캡처하여 서버로 TCP 소켓을 통해 전송
"""

import cv2
import socket
import struct
import time
import pickle
from jetson_config import JetsonConfig


class JetsonStreamer:
    def __init__(self, server_host='localhost', server_port=9999, camera_id=0):
        """
        Jetson 스트리머 초기화

        Args:
            server_host: 서버 IP 주소
            server_port: 서버 포트 번호
            camera_id: USB 카메라 장치 ID (보통 0)
        """
        self.server_host = server_host
        self.server_port = server_port
        self.camera_id = camera_id
        self.sock = None
        self.camera = None

    def connect_to_server(self):
        """스트리밍 서버에 연결"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.server_host, self.server_port))
            print(f"✓ 서버 연결 성공: {self.server_host}:{self.server_port}")
            return True
        except Exception as e:
            print(f"✗ 연결 실패: {e}")
            return False

    def initialize_camera(self, width=640, height=480, fps=30):
        """
        USB 카메라 초기화

        Args:
            width: 프레임 너비
            height: 프레임 높이
            fps: 초당 프레임 수
        """
        try:
            self.camera = cv2.VideoCapture(self.camera_id)

            if not self.camera.isOpened():
                print(f"✗ 카메라 열기 실패: {self.camera_id}")
                return False

            # 카메라 속성 설정
            self.camera.set(cv2.CAP_PROP_FRAME_WIDTH, width)
            self.camera.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
            self.camera.set(cv2.CAP_PROP_FPS, fps)

            # 설정 확인
            actual_width = int(self.camera.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self.camera.get(cv2.CAP_PROP_FRAME_HEIGHT))
            actual_fps = int(self.camera.get(cv2.CAP_PROP_FPS))

            print(f"✓ 카메라 초기화 완료: {actual_width}x{actual_height} @ {actual_fps}fps")
            return True

        except Exception as e:
            print(f"✗ 카메라 초기화 실패: {e}")
            return False

    def send_frame(self, frame):
        """
        프레임 인코딩 후 서버로 전송

        Args:
            frame: OpenCV 프레임 (numpy 배열)
        """
        try:
            # JPEG로 인코딩
            encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), JetsonConfig.JPEG_QUALITY]
            result, encoded_frame = cv2.imencode('.jpg', frame, encode_param)

            if not result:
                print("✗ 프레임 인코딩 실패")
                return False

            # 프레임 데이터 직렬화
            data = pickle.dumps(encoded_frame)

            # 메시지 크기를 먼저 전송 (4바이트)
            message_size = struct.pack("L", len(data))
            self.sock.sendall(message_size + data)

            return True

        except Exception as e:
            print(f"✗ 전송 실패: {e}")
            return False

    def stream(self, target_fps=30):
        """
        메인 스트리밍 루프

        Args:
            target_fps: 목표 FPS
        """
        if not self.initialize_camera(
            width=JetsonConfig.CAMERA_WIDTH,
            height=JetsonConfig.CAMERA_HEIGHT,
            fps=target_fps
        ):
            return

        if not self.connect_to_server():
            self.cleanup()
            return

        print(f"\n▶ 스트리밍 시작 (종료: Ctrl+C)...")
        frame_interval = 1.0 / target_fps
        frame_count = 0
        start_time = time.time()

        try:
            while True:
                loop_start = time.time()

                # 프레임 캡처
                ret, frame = self.camera.read()

                if not ret:
                    print("✗ 프레임 캡처 실패")
                    break

                # 프레임 전송
                if not self.send_frame(frame):
                    print("✗ 연결 끊김, 재연결 시도 중...")
                    self.sock.close()
                    time.sleep(2)
                    if not self.connect_to_server():
                        break
                    continue

                # FPS 카운터
                frame_count += 1
                if frame_count % 30 == 0:
                    elapsed = time.time() - start_time
                    current_fps = frame_count / elapsed
                    print(f"📊 전송 프레임: {frame_count} | FPS: {current_fps:.2f}")

                # 목표 FPS 유지
                elapsed = time.time() - loop_start
                sleep_time = frame_interval - elapsed
                if sleep_time > 0:
                    time.sleep(sleep_time)

        except KeyboardInterrupt:
            print("\n⏸ 사용자에 의해 중지됨")
        except Exception as e:
            print(f"\n✗ 스트리밍 오류: {e}")
        finally:
            self.cleanup()

    def cleanup(self):
        """리소스 해제"""
        print("\n🧹 정리 중...")

        if self.camera is not None:
            self.camera.release()
            print("✓ 카메라 해제 완료")

        if self.sock is not None:
            try:
                self.sock.close()
                print("✓ 소켓 종료 완료")
            except:
                pass


def main():
    """메인 진입점"""
    print("=" * 50)
    print("  Jetson Nano USB 카메라 스트리밍 클라이언트")
    print("=" * 50)
    print(f"서버: {JetsonConfig.SERVER_HOST}:{JetsonConfig.SERVER_PORT}")
    print(f"카메라: /dev/video{JetsonConfig.CAMERA_ID}")
    print(f"목표 FPS: {JetsonConfig.TARGET_FPS}")
    print(f"해상도: {JetsonConfig.CAMERA_WIDTH}x{JetsonConfig.CAMERA_HEIGHT}")
    print("=" * 50)

    # 스트리머 생성 및 시작
    streamer = JetsonStreamer(
        server_host=JetsonConfig.SERVER_HOST,
        server_port=JetsonConfig.SERVER_PORT,
        camera_id=JetsonConfig.CAMERA_ID
    )

    streamer.stream(target_fps=JetsonConfig.TARGET_FPS)


if __name__ == "__main__":
    main()
