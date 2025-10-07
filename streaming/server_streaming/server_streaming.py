#!/usr/bin/env python3
"""
스트리밍 서버
Jetson Nano로부터 TCP 소켓을 통해 영상을 수신하여 실시간으로 표시
"""

import cv2
import socket
import struct
import pickle
import numpy as np
import time
from server_config import ServerConfig


class StreamingServer:
    def __init__(self, host='0.0.0.0', port=9999):
        """
        스트리밍 서버 초기화

        Args:
            host: 서버 바인딩 주소 (0.0.0.0은 모든 인터페이스)
            port: 서버 포트 번호
        """
        self.host = host
        self.port = port
        self.server_socket = None
        self.client_socket = None

    def start_server(self):
        """TCP 서버 시작 및 클라이언트 연결 대기"""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(5)

            print(f"✓ 서버 시작: {self.host}:{self.port}")
            print(f"⏳ Jetson Nano 연결 대기 중...")

            self.client_socket, client_address = self.server_socket.accept()
            print(f"✓ 클라이언트 연결됨: {client_address[0]}:{client_address[1]}")

            return True

        except Exception as e:
            print(f"✗ 서버 시작 실패: {e}")
            return False

    def receive_frame(self):
        """
        클라이언트로부터 프레임 수신 및 디코딩

        Returns:
            frame: OpenCV 프레임 (numpy 배열) 또는 실패시 None
        """
        try:
            # 메시지 크기 수신 (4바이트)
            data = b""
            payload_size = struct.calcsize("L")

            while len(data) < payload_size:
                packet = self.client_socket.recv(4096)
                if not packet:
                    return None
                data += packet

            packed_msg_size = data[:payload_size]
            data = data[payload_size:]
            msg_size = struct.unpack("L", packed_msg_size)[0]

            # 프레임 데이터 수신
            while len(data) < msg_size:
                packet = self.client_socket.recv(4096)
                if not packet:
                    return None
                data += packet

            frame_data = data[:msg_size]

            # 역직렬화 및 프레임 디코딩
            encoded_frame = pickle.loads(frame_data)
            frame = cv2.imdecode(encoded_frame, cv2.IMREAD_COLOR)

            return frame

        except Exception as e:
            print(f"✗ 수신 실패: {e}")
            return None

    def receive_and_display(self, save_video=False, output_file='output.avi'):
        """
        메인 수신 루프 - 프레임 수신 및 표시

        Args:
            save_video: 수신한 비디오를 파일로 저장할지 여부
            output_file: 출력 비디오 파일명
        """
        if not self.start_server():
            return

        print(f"\n▶ 스트림 수신 중 (종료: 'q' 키)...")

        video_writer = None
        frame_count = 0
        start_time = time.time()

        try:
            while True:
                frame = self.receive_frame()

                if frame is None:
                    print("✗ 연결 끊김 또는 프레임 수신 실패")
                    break

                # 필요시 비디오 작성기 초기화
                if save_video and video_writer is None:
                    height, width = frame.shape[:2]
                    fourcc = cv2.VideoWriter_fourcc(*ServerConfig.VIDEO_CODEC)
                    video_writer = cv2.VideoWriter(output_file, fourcc, ServerConfig.VIDEO_FPS, (width, height))
                    print(f"✓ 녹화 중: {output_file}")

                # 프레임을 비디오 파일에 저장
                if save_video and video_writer is not None:
                    video_writer.write(frame)

                # 프레임 표시
                cv2.imshow(ServerConfig.WINDOW_NAME, frame)

                # FPS 카운터
                frame_count += 1
                if frame_count % 30 == 0:
                    elapsed = time.time() - start_time
                    current_fps = frame_count / elapsed
                    print(f"📊 수신 프레임: {frame_count} | FPS: {current_fps:.2f}")

                # 종료 확인
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    print("\n⏸ 사용자에 의해 중지됨")
                    break

        except KeyboardInterrupt:
            print("\n⏸ 사용자에 의해 중지됨")
        except Exception as e:
            print(f"\n✗ 스트리밍 오류: {e}")
        finally:
            self.cleanup(video_writer)

    def cleanup(self, video_writer=None):
        """
        리소스 해제

        Args:
            video_writer: 해제할 OpenCV VideoWriter 객체
        """
        print("\n🧹 정리 중...")

        if video_writer is not None:
            video_writer.release()
            print("✓ 비디오 파일 저장 완료")

        cv2.destroyAllWindows()
        print("✓ 창 닫기 완료")

        if self.client_socket is not None:
            try:
                self.client_socket.close()
                print("✓ 클라이언트 소켓 종료 완료")
            except:
                pass

        if self.server_socket is not None:
            try:
                self.server_socket.close()
                print("✓ 서버 소켓 종료 완료")
            except:
                pass


def main():
    """메인 진입점"""
    print("=" * 50)
    print("  Jetson Nano 스트리밍 서버")
    print("=" * 50)
    print(f"수신 주소: {ServerConfig.HOST}:{ServerConfig.PORT}")
    print(f"비디오 저장: {ServerConfig.SAVE_VIDEO}")
    if ServerConfig.SAVE_VIDEO:
        print(f"출력 파일: {ServerConfig.OUTPUT_FILE}")
    print("=" * 50)

    # 서버 생성 및 시작
    server = StreamingServer(host=ServerConfig.HOST, port=ServerConfig.PORT)
    server.receive_and_display(
        save_video=ServerConfig.SAVE_VIDEO,
        output_file=ServerConfig.OUTPUT_FILE
    )


if __name__ == "__main__":
    main()
