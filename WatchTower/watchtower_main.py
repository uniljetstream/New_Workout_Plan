#!/usr/bin/env python3
"""
WatchTower Main
Qt, MQTT, HTTP를 통합하는 메인 시스템
"""

import time
import cv2
import base64
import threading
from mqtt_controller import MQTTController
from http_client import HTTPClient
from watchtower_config import WatchTowerConfig
from uart_controller import UARTController
from pantilt_tracker import PanTiltTracker


class WatchTowerMain:
    """WatchTower 통합 시스템"""

    def __init__(self):
        """초기화"""
        self.config = WatchTowerConfig
        self.mqtt = MQTTController()
        self.http = HTTPClient()
        self.current_mode = None
        self.is_running = False

        # 카메라 및 팬틸트 관련
        self.camera = None
        self.uart = None
        self.tracker = None
        self.is_streaming = False
        self.pantilt_tracking_enabled = False
        self.first_analysis_received = False
        self.streaming_thread = None
        self.analysis_request_pending = False
        self.analysis_request_timestamp = 0.0
        self.analysis_request_pose_index = None

    def start(self):
        """시스템 시작"""
        print("="*60)
        print("  WatchTower 통합 시스템 시작")
        print("="*60)
        print(f"AI 서버: {self.config.AI_SERVER_HOST}:{self.config.AI_SERVER_PORT}")
        print(f"MQTT 브로커: {self.config.MQTT_BROKER_HOST}:{self.config.MQTT_BROKER_PORT}")
        print("="*60)

        # MQTT 연결
        print("\n[1] MQTT 브로커 연결")
        if not self.mqtt.connect():
            print("✗ MQTT 브로커 연결 실패")
            return False

        # AI 서버 연결 확인
        print("\n[2] AI 서버 연결 확인")
        if not self.http.check_server():
            print("✗ AI 서버 연결 실패")
            self.mqtt.disconnect()
            return False

        # 콜백 함수 등록
        self.mqtt.on_qt_select_mode_callback = self.handle_qt_mode_selection
        self.mqtt.on_qt_stop_callback = self.handle_qt_stop
        self.mqtt.on_joystick_data_callback = self.handle_joystick_data
        self.mqtt.on_watch_data_callback = self.handle_watch_data
        self.mqtt.on_qt_pose_index_callback = self.handle_qt_pose_index
        self.mqtt.on_qt_request_analysis_callback = self.handle_qt_request_analysis

        # 조이스틱을 기본 에어마우스 모드로 설정
        print("\n[3] 조이스틱 에어마우스 모드 설정 (MQTT)")
        if not self.mqtt.send_joystick_mode_command('airmouse'):
            print("⚠ 조이스틱 에어마우스 모드 설정 실패 (기본 모드 유지)")

        print("\n✓ WatchTower 시스템 준비 완료")
        print("→ Qt로부터 운동 모드 선택 대기 중...\n")

        self.is_running = True
        return True

    def handle_qt_mode_selection(self, mode, timestamp):
        """
        Qt에서 운동 모드 선택 시 호출되는 콜백

        Args:
            mode: 선택된 운동 모드
            timestamp: 타임스탬프
        """
        print(f"\n{'='*60}")
        print(f"  Qt 운동 모드 선택: {mode}")
        print(f"{'='*60}")

        # Step 1: AI 서버에 모드 전송 (HTTP)
        print(f"\n[Step 1] AI 서버에 모드 전송 (HTTP)")
        result = self.http.select_mode(mode)

        if not result['success']:
            # 실패 시 Qt에 에러 응답
            error_msg = result.get('error', 'Unknown error')
            print(f"✗ 모드 선택 실패: {error_msg}")
            self.mqtt.last_pose_sequence = {}
            self.mqtt.send_qt_mode_selected(mode, success=False, message=error_msg)
            return

        print(f"✓ AI 서버 모드 선택 완료")

        # Save pose sequence for Qt response
        pose_payload = {
            'poses': result.get('poses', []),
            'total_poses': result.get('total_poses', len(result.get('poses', [])))
        }
        self.mqtt.last_pose_sequence = pose_payload

        # 초기 포즈 인덱스 0으로 설정
        self.http.set_pose_index(0)
        self.analysis_request_pending = False
        self.analysis_request_timestamp = 0.0
        self.analysis_request_pose_index = None

        # Step 2: 조이스틱을 센서 모드로 전환 (MQTT)
        print(f"\n[Step 2] 조이스틱을 센서 모드로 전환 (MQTT)")
        if not self.mqtt.send_joystick_mode_command('sensor'):
            print(f"✗ 조이스틱 센서 모드 전환 실패")
            self.mqtt.send_qt_mode_selected(mode, success=False, message="조이스틱 모드 전환 실패")
            self.mqtt.last_pose_sequence = {}
            return

        # Step 3: 조이스틱과 Watch에 시작 명령 전송 (MQTT)
        print(f"\n[Step 3] 조이스틱과 Watch에 시작 명령 전송 (MQTT)")
        if not self.mqtt.send_start_command(mode):
            print(f"✗ 디바이스 시작 명령 전송 실패")
            # 센서 모드 전환이 성공했었다면 에어마우스로 복구 시도
            self.mqtt.send_joystick_mode_command('airmouse')
            self.mqtt.send_qt_mode_selected(mode, success=False, message="디바이스 연결 실패")
            self.mqtt.last_pose_sequence = {}
            return

        print(f"✓ 디바이스 시작 명령 전송 완료")

        # Step 4: 카메라 스트리밍 시작 (백그라운드 스레드)
        print(f"\n[Step 4] 카메라 스트리밍 시작")
        if not self.start_camera_streaming():
            print(f"✗ 카메라 스트리밍 시작 실패")
            self.mqtt.send_joystick_mode_command('airmouse')
            self.mqtt.send_qt_mode_selected(mode, success=False, message="카메라 초기화 실패")
            self.mqtt.last_pose_sequence = {}
            return

        # Step 5: Qt에 성공 응답 전송
        print(f"\n[Step 5] Qt에 성공 응답 전송")
        self.mqtt.send_qt_mode_selected(mode, success=True, message="운동 시작")

        self.current_mode = mode
        print(f"\n✓ 운동 시작: {mode}")
        print(f"{'='*60}\n")

    def handle_qt_pose_index(self, mode, pose_index):
        """Qt에서 전달한 포즈 인덱스를 AI 서버에 반영"""
        target_mode = mode or self.current_mode

        if target_mode and self.current_mode and target_mode != self.current_mode:
            print(f"⚠ 포즈 인덱스 모드 불일치: Qt={target_mode}, 현재={self.current_mode}")

        if self.http.set_pose_index(pose_index):
            print(f"✓ 포즈 인덱스 업데이트: {pose_index}")
            self.analysis_request_pose_index = pose_index

            # 포즈 변경 시 진동 피드백 전송 (0.5초, 강도 50%)
            self.mqtt.send_joystick_vibration(intensity=50, duration_ms=500)
        else:
            print(f"✗ 포즈 인덱스 설정 실패: {pose_index}")

    def handle_qt_request_analysis(self, mode, pose_index):
        """Qt에서 단일 분석을 요청했을 때 호출"""
        if mode and self.current_mode and mode != self.current_mode:
            print(f"⚠ 분석 요청 모드 불일치: Qt={mode}, 현재={self.current_mode}")

        if pose_index is not None:
            if self.http.set_pose_index(pose_index):
                self.analysis_request_pose_index = pose_index
            else:
                print(f"✗ 분석 요청 중 포즈 인덱스 설정 실패: {pose_index}")
                self.analysis_request_pose_index = self.http.current_pose_index
        else:
            self.analysis_request_pose_index = self.http.current_pose_index

        self.analysis_request_pending = True
        self.analysis_request_timestamp = time.time()
        print("→ 단일 분석 요청 플래그 설정")

    def handle_qt_stop(self, timestamp):
        """
        Qt에서 정지 명령 수신 시 호출되는 콜백

        Args:
            timestamp: 타임스탬프
        """
        print(f"\n{'='*60}")
        print(f"  Qt 정지 명령 수신")
        print(f"{'='*60}")

        # Step 1: 카메라 스트리밍 중지
        print(f"\n[Step 1] 카메라 스트리밍 중지")
        self.stop_camera_streaming()

        # Step 2: AI 서버에 분석 종료 요청 (HTTP)
        print(f"\n[Step 2] AI 서버에 분석 종료 요청 (HTTP)")
        if self.current_mode:
            if self.http.stop_stream():
                print(f"✓ AI 서버 분석 종료 완료")
            else:
                print(f"✗ AI 서버 분석 종료 실패 (계속 진행)")
        else:
            print(f"ℹ 진행 중인 운동 없음")

        # Step 3: 조이스틱과 Watch에 정지 명령 전송 (MQTT)
        print(f"\n[Step 3] 조이스틱과 Watch에 정지 명령 전송 (MQTT)")
        if self.mqtt.send_stop_command():
            print(f"✓ 디바이스 정지 명령 전송 완료")
        else:
            print(f"✗ 디바이스 정지 명령 전송 실패")

        # Step 4: 조이스틱을 에어마우스 모드로 전환 (MQTT)
        print(f"\n[Step 4] 조이스틱을 에어마우스 모드로 전환 (MQTT)")
        if self.mqtt.send_joystick_mode_command('airmouse'):
            print(f"✓ 조이스틱 에어마우스 모드 전환 완료")
        else:
            print(f"⚠ 조이스틱 에어마우스 모드 전환 실패")

        # Step 5: Qt에 종료 완료 응답 전송
        print(f"\n[Step 5] Qt에 종료 완료 응답 전송")
        self.mqtt.send_qt_status("stopped", "운동 종료")

        # 현재 모드 초기화
        self.current_mode = None

        print(f"\n✓ 운동 종료 처리 완료")
        print(f"{'='*60}\n")

    def handle_joystick_data(self, data):
        """
        조이스틱 센서 데이터 수신 시 콜백

        Args:
            data: 조이스틱 센서 데이터 딕셔너리
        """
        # 조이스틱 데이터 처리 로직 (필요시 구현)
        # 예: 데이터 로깅, 실시간 분석 등
        pass

    def handle_watch_data(self, data):
        """
        Watch 심박수 데이터 수신 시 콜백

        Args:
            data: Watch 심박수 데이터 딕셔너리
        """
        # Watch 데이터 처리 로직 (필요시 구현)
        # 예: 심박수 모니터링, 경고 알림 등
        pass

    def initialize_camera(self):
        """
        카메라 초기화

        Returns:
            bool: 초기화 성공 여부
        """
        try:
            self.camera = cv2.VideoCapture(self.config.CAMERA_ID)

            if not self.camera.isOpened():
                print(f"✗ 카메라 열기 실패: /dev/video{self.config.CAMERA_ID}")
                return False

            # 카메라 속성 설정
            self.camera.set(cv2.CAP_PROP_FRAME_WIDTH, self.config.CAMERA_WIDTH)
            self.camera.set(cv2.CAP_PROP_FRAME_HEIGHT, self.config.CAMERA_HEIGHT)
            self.camera.set(cv2.CAP_PROP_FPS, self.config.CAMERA_FPS)

            actual_width = int(self.camera.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self.camera.get(cv2.CAP_PROP_FRAME_HEIGHT))

            print(f"✓ 카메라 초기화 완료: {actual_width}x{actual_height}")

            # 팬틸트 추적기 초기화 (카메라 해상도 기반)
            if self.config.PANTILT_ENABLED:
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
        if not self.config.PANTILT_ENABLED:
            print("ℹ 팬틸트 제어 비활성화됨")
            return True

        try:
            self.uart = UARTController()
            if self.uart.connect():
                print(f"✓ 팬틸트 UART 초기화 완료")
                return True
            else:
                print(f"⚠ 팬틸트 UART 연결 실패 (계속 진행)")
                self.uart = None
                return False

        except Exception as e:
            print(f"⚠ 팬틸트 초기화 오류: {e} (계속 진행)")
            self.uart = None
            return False

    def start_camera_streaming(self):
        """
        카메라 스트리밍 시작 (백그라운드 스레드)

        Returns:
            bool: 시작 성공 여부
        """
        # 카메라 초기화
        if not self.initialize_camera():
            print("✗ 카메라 스트리밍 시작 실패: 카메라 초기화 실패")
            return False

        # 팬틸트 초기화
        self.initialize_pantilt()

        # 스트리밍 플래그 초기화
        self.is_streaming = True
        self.pantilt_tracking_enabled = False
        self.first_analysis_received = False
        self.analysis_request_pending = False
        self.analysis_request_timestamp = 0.0
        self.analysis_request_pose_index = None
        self.analysis_request_pose_index = None
        self.analysis_request_pending = False

        # 백그라운드 스레드로 스트리밍 루프 시작
        self.streaming_thread = threading.Thread(
            target=self._streaming_loop,
            daemon=True
        )
        self.streaming_thread.start()

        print(f"✓ 카메라 스트리밍 시작 (백그라운드 스레드)")
        return True

    def _streaming_loop(self):
        """
        카메라 스트리밍 메인 루프 (백그라운드 스레드에서 실행)
        """
        frame_interval = 1.0 / self.config.STREAM_FPS
        frame_count = 0
        stream_frame_index = 0
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), self.config.JPEG_QUALITY]

        print(f"→ 스트리밍 루프 시작 (목표 FPS: {self.config.STREAM_FPS})")

        try:
            while self.is_streaming:
                loop_start = time.time()

                # 프레임 캡처
                ret, frame = self.camera.read()
                if not ret:
                    print("⚠ 프레임 캡처 실패")
                    time.sleep(0.1)
                    continue

                # 프레임 JPEG 인코딩
                success, encoded_frame = cv2.imencode('.jpg', frame, encode_param)
                if not success:
                    print("⚠ 프레임 인코딩 실패")
                    time.sleep(0.05)
                    continue

                # Base64 변환 (Qt 스트리밍 및 AI 서버 전송에 사용)
                frame_base64 = base64.b64encode(encoded_frame).decode('utf-8')

                result = None
                if self.analysis_request_pending:
                    print("→ 단일 분석 실행")
                    pose_index = self.analysis_request_pose_index
                    result = self.http.send_frame(
                        frame_base64=frame_base64,
                        pose_index=pose_index
                    )
                    self.analysis_request_pending = False
                    self.analysis_request_pose_index = None

                # Qt 앱에 프레임 전송
                metadata = {
                    'timestamp': int(time.time() * 1000),
                    'frame_index': stream_frame_index,
                    'pose_index': self.http.current_pose_index
                }
                if self.http.current_mode:
                    metadata['mode'] = self.http.current_mode
                self.mqtt.send_qt_frame(frame_base64, metadata)
                stream_frame_index += 1

                if result and result.get('status') == 'success':
                    frame_count += 1

                    # 첫 번째 분석 결과 수신 시 팬틸트 추적 활성화
                    if not self.first_analysis_received:
                        self.first_analysis_received = True
                        self.pantilt_tracking_enabled = True
                        print("✓ 첫 분석 결과 수신 - 팬틸트 추적 시작")

                    # 결과 출력
                    score = result.get('score', 0)
                    feedback = result.get('feedback', '')
                    is_correct = result.get('is_correct', False)

                    status = "✓ 정확" if is_correct else "✗ 조정 필요"
                    print(f"[{frame_count:04d}] {status} | 점수: {score}% | {feedback}", end='')

                    # 팬틸트 추적 (활성화된 경우에만)
                    if self.pantilt_tracking_enabled and self.uart and self.tracker:
                        if self.tracker.update(result):
                            # 새로운 팬틸트 각도 계산
                            pan, tilt = self.tracker.calculate_pan_tilt_angles()

                            # UART로 각도 전송
                            uart_success = self.uart.send_pan_tilt(
                                pan, tilt,
                                verbose=self.config.PANTILT_VERBOSE
                            )

                            # 추적 정보 출력
                            track_info = self.tracker.get_tracking_info()
                            uart_status = "✓" if uart_success else "✗"
                            print(f" | 🎯 ({track_info['target_x']}, {track_info['target_y']}) | Pan={pan:.0f}° Tilt={tilt:.0f}° {uart_status}", end='')
                        else:
                            print(f" | 🎯 대상 없음", end='')

                    print()  # 개행

                    # Qt에 분석 결과 전송
                    self.mqtt.send_qt_analysis_result(result)

                # FPS 유지
                elapsed = time.time() - loop_start
                sleep_time = frame_interval - elapsed
                if sleep_time > 0:
                    time.sleep(sleep_time)

        except Exception as e:
            print(f"\n✗ 스트리밍 루프 오류: {e}")
        finally:
            print(f"\n→ 스트리밍 루프 종료 (총 {frame_count} 프레임 처리)")

    def stop_camera_streaming(self):
        """카메라 스트리밍 중지"""
        if not self.is_streaming:
            return

        print("→ 카메라 스트리밍 중지 중...")

        # 스트리밍 중지 플래그 설정
        self.is_streaming = False
        self.pantilt_tracking_enabled = False
        self.first_analysis_received = False
        self.analysis_request_pending = False
        self.analysis_request_timestamp = 0.0

        # 스레드 종료 대기
        if self.streaming_thread and self.streaming_thread.is_alive():
            self.streaming_thread.join(timeout=5.0)

        # 팬틸트 중앙 복귀
        if self.uart:
            print("→ 팬틸트 중앙 복귀 중...")
            self.uart.center()
            time.sleep(0.5)
            self.uart.disconnect()
            self.uart = None

        # 카메라 릴리즈
        if self.camera:
            self.camera.release()
            self.camera = None

        # 추적기 리셋
        if self.tracker:
            self.tracker.reset()

        print("✓ 카메라 스트리밍 중지 완료")

    def stop(self):
        """시스템 중지"""
        print("\n→ WatchTower 시스템 종료 중...")

        # 카메라 스트리밍 중지
        self.stop_camera_streaming()

        # 디바이스에 정지 명령 전송
        if self.mqtt.connected:
            self.mqtt.send_stop_command()

        # AI 서버에 스트리밍 중단 요청
        if self.current_mode:
            self.http.stop_stream()

        # MQTT 연결 해제
        self.mqtt.disconnect()

        self.is_running = False
        print("✓ WatchTower 시스템 종료 완료")

    def run(self):
        """메인 루프 실행"""
        if not self.start():
            print("\n✗ WatchTower 시스템 시작 실패")
            return

        try:
            # 무한 루프 - Ctrl+C로 종료할 때까지 실행
            while self.is_running:
                time.sleep(0.1)  # CPU 과부하 방지

        except KeyboardInterrupt:
            print("\n\n⏸ 사용자에 의해 중지됨")
        finally:
            self.stop()


def main():
    """메인 진입점"""
    watchtower = WatchTowerMain()
    watchtower.run()


if __name__ == "__main__":
    main()
