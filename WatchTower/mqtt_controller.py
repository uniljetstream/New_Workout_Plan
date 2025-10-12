#!/usr/bin/env python3
"""
MQTT Controller for WatchTower
조이스틱과 Watch 디바이스와 MQTT 통신 관리
"""

import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime
from watchtower_config import WatchTowerConfig


class MQTTController:
    """MQTT 통신 컨트롤러"""

    def __init__(self):
        """초기화"""
        self.config = WatchTowerConfig
        self.client = None
        self.connected = False

        # 센서 데이터 저장
        self.joystick_data = {
            'accel_x': 0.0,
            'accel_y': 0.0,
            'accel_z': 0.0,
            'gyro_x': 0.0,
            'gyro_y': 0.0,
            'gyro_z': 0.0,
            'timestamp': None,
            'last_update': None
        }

        self.watch_data = {
            'heart_rate': 0,
            'timestamp': None,
            'last_update': None
        }

        # 디바이스 상태
        self.joystick_ready = False
        self.watch_ready = False

        # 콜백 함수들 (외부에서 등록 가능)
        self.on_joystick_data_callback = None
        self.on_watch_data_callback = None
        self.on_qt_select_mode_callback = None  # Qt에서 운동 모드 선택 시 호출
        self.on_qt_stop_callback = None         # Qt에서 정지 명령 시 호출

    def connect(self):
        """
        MQTT 브로커에 연결

        Returns:
            bool: 연결 성공 여부
        """
        try:
            # MQTT 클라이언트 생성 (API v2)
            self.client = mqtt.Client(
                mqtt.CallbackAPIVersion.VERSION2,
                client_id="watchtower_main"
            )

            # 콜백 함수 설정
            self.client.on_connect = self._on_connect
            self.client.on_message = self._on_message
            self.client.on_disconnect = self._on_disconnect

            # 인증 설정 (필요한 경우)
            if self.config.MQTT_USERNAME and self.config.MQTT_PASSWORD:
                self.client.username_pw_set(
                    self.config.MQTT_USERNAME,
                    self.config.MQTT_PASSWORD
                )

            # 브로커에 연결
            print(f"→ MQTT 브로커 연결 중: {self.config.MQTT_BROKER_HOST}:{self.config.MQTT_BROKER_PORT}")
            self.client.connect(
                self.config.MQTT_BROKER_HOST,
                self.config.MQTT_BROKER_PORT,
                self.config.MQTT_KEEPALIVE
            )

            # 백그라운드 루프 시작
            self.client.loop_start()

            # 연결 대기
            time.sleep(1)

            if self.connected:
                print("✓ MQTT 브로커 연결 성공")
                return True
            else:
                print("✗ MQTT 브로커 연결 실패")
                return False

        except Exception as e:
            print(f"✗ MQTT 연결 오류: {e}")
            return False

    def disconnect(self):
        """MQTT 브로커 연결 해제"""
        if self.client:
            print("→ MQTT 연결 해제 중...")
            self.client.loop_stop()
            self.client.disconnect()
            self.connected = False
            print("✓ MQTT 연결 해제 완료")

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        """
        브로커 연결 시 콜백 (API v2)

        Args:
            reason_code: 연결 결과 코드 (0: 성공)
        """
        if reason_code == 0:
            self.connected = True
            print("✓ MQTT 브로커 연결됨")

            # 센서 데이터 토픽 구독
            topics = [
                (self.config.TOPIC_QT_SELECT_MODE, self.config.MQTT_QOS),
                (self.config.TOPIC_QT_START, self.config.MQTT_QOS),
                (self.config.TOPIC_QT_STOP, self.config.MQTT_QOS),
                (self.config.TOPIC_JOYSTICK_DATA, self.config.MQTT_QOS),
                (self.config.TOPIC_WATCH_HEARTRATE, self.config.MQTT_QOS),
                (self.config.TOPIC_JOYSTICK_STATUS, self.config.MQTT_QOS),
                (self.config.TOPIC_WATCH_STATUS, self.config.MQTT_QOS)
            ]

            for topic, qos in topics:
                client.subscribe(topic, qos)
                print(f"  ✓ 토픽 구독: {topic}")
        else:
            self.connected = False
            print(f"✗ MQTT 연결 실패. 에러 코드: {reason_code}")

    def _on_disconnect(self, client, userdata, disconnect_flags, reason_code, properties):
        """
        연결 해제 시 콜백 (API v2)

        Args:
            reason_code: 연결 해제 이유 코드
        """
        self.connected = False
        if reason_code != 0:
            print(f"⚠ MQTT 연결 끊김. 에러 코드: {reason_code}")
            print("→ 재연결 시도 중...")

    def _on_message(self, client, userdata, msg):
        """
        메시지 수신 시 콜백

        Args:
            msg: 수신된 메시지
        """
        try:
            topic = msg.topic
            payload = msg.payload.decode('utf-8')

            # Qt 운동 모드 선택
            if topic == self.config.TOPIC_QT_SELECT_MODE:
                self._handle_qt_select_mode(payload)

            # Qt 정지 명령
            elif topic == self.config.TOPIC_QT_STOP:
                self._handle_qt_stop(payload)

            # 조이스틱 센서 데이터
            elif topic == self.config.TOPIC_JOYSTICK_DATA:
                self._handle_joystick_data(payload)

            # Watch 심박수 데이터
            elif topic == self.config.TOPIC_WATCH_HEARTRATE:
                self._handle_watch_data(payload)

            # 조이스틱 상태
            elif topic == self.config.TOPIC_JOYSTICK_STATUS:
                self._handle_joystick_status(payload)

            # Watch 상태
            elif topic == self.config.TOPIC_WATCH_STATUS:
                self._handle_watch_status(payload)

        except Exception as e:
            print(f"✗ 메시지 처리 오류: {e}")

    def _handle_joystick_data(self, payload):
        """
        조이스틱 센서 데이터 처리

        Expected JSON format:
        {
            "accel_x": 0.0,
            "accel_y": 0.0,
            "accel_z": 0.0,
            "gyro_x": 0.0,
            "gyro_y": 0.0,
            "gyro_z": 0.0,
            "timestamp": 1234567890
        }
        """
        try:
            data = json.loads(payload)

            # 데이터 업데이트
            self.joystick_data = {
                'accel_x': data.get('accel_x', 0.0),
                'accel_y': data.get('accel_y', 0.0),
                'accel_z': data.get('accel_z', 0.0),
                'gyro_x': data.get('gyro_x', 0.0),
                'gyro_y': data.get('gyro_y', 0.0),
                'gyro_z': data.get('gyro_z', 0.0),
                'timestamp': data.get('timestamp'),
                'last_update': time.time()
            }

            # 외부 콜백 호출
            if self.on_joystick_data_callback:
                self.on_joystick_data_callback(self.joystick_data)

        except json.JSONDecodeError:
            print(f"✗ 조이스틱 데이터 JSON 파싱 실패: {payload}")

    def _handle_watch_data(self, payload):
        """
        Watch 심박수 데이터 처리

        Expected JSON format:
        {
            "heart_rate": 75,
            "timestamp": 1234567890
        }
        """
        try:
            data = json.loads(payload)

            # 데이터 업데이트
            self.watch_data = {
                'heart_rate': data.get('heart_rate', 0),
                'timestamp': data.get('timestamp'),
                'last_update': time.time()
            }

            # 외부 콜백 호출
            if self.on_watch_data_callback:
                self.on_watch_data_callback(self.watch_data)

        except json.JSONDecodeError:
            print(f"✗ Watch 데이터 JSON 파싱 실패: {payload}")

    def _handle_joystick_status(self, payload):
        """조이스틱 상태 메시지 처리"""
        try:
            data = json.loads(payload)
            status = data.get('status', 'unknown')

            if status == 'ready':
                self.joystick_ready = True
                print("✓ 조이스틱 준비 완료")
            elif status == 'stopped':
                self.joystick_ready = False
                print("⏸ 조이스틱 중지됨")

        except json.JSONDecodeError:
            print(f"✗ 조이스틱 상태 JSON 파싱 실패: {payload}")

    def _handle_watch_status(self, payload):
        """Watch 상태 메시지 처리"""
        try:
            data = json.loads(payload)
            status = data.get('status', 'unknown')

            if status == 'ready':
                self.watch_ready = True
                print("✓ Watch 준비 완료")
            elif status == 'stopped':
                self.watch_ready = False
                print("⏸ Watch 중지됨")

        except json.JSONDecodeError:
            print(f"✗ Watch 상태 JSON 파싱 실패: {payload}")

    def _handle_qt_select_mode(self, payload):
        """
        Qt에서 온 운동 모드 선택 메시지 처리

        Expected JSON format:
        {
            "mode": "t_pose",
            "timestamp": 1234567890
        }
        """
        try:
            data = json.loads(payload)
            mode = data.get('mode', '')

            print(f"✓ Qt로부터 운동 모드 수신: {mode}")

            # 외부 콜백 호출 (WatchTower Main에서 HTTP 전송 처리)
            if self.on_qt_select_mode_callback:
                self.on_qt_select_mode_callback(mode, data.get('timestamp'))

        except json.JSONDecodeError:
            print(f"✗ Qt 모드 선택 JSON 파싱 실패: {payload}")
            self.send_qt_error("Invalid JSON format")
        except Exception as e:
            print(f"✗ Qt 모드 선택 처리 오류: {e}")
            self.send_qt_error(str(e))

    def _handle_qt_stop(self, payload):
        """
        Qt에서 온 정지 명령 메시지 처리

        Expected JSON format:
        {
            "command": "stop",
            "timestamp": 1234567890
        }
        """
        try:
            data = json.loads(payload)
            print(f"✓ Qt로부터 정지 명령 수신")

            # 외부 콜백 호출 (WatchTower Main에서 종료 처리)
            if self.on_qt_stop_callback:
                self.on_qt_stop_callback(data.get('timestamp'))

        except json.JSONDecodeError:
            print(f"✗ Qt 정지 명령 JSON 파싱 실패: {payload}")
        except Exception as e:
            print(f"✗ Qt 정지 명령 처리 오류: {e}")

    def send_start_command(self, mode):
        """
        조이스틱과 Watch에 시작 명령 전송

        Args:
            mode: 운동 모드 (t_pose, squat, pushup, etc.)

        Returns:
            bool: 전송 성공 여부
        """
        if not self.connected:
            print("✗ MQTT 브로커에 연결되지 않음")
            return False

        try:
            # 시작 명령 메시지
            command = {
                'command': 'start',
                'mode': mode,
                'timestamp': int(time.time() * 1000)
            }

            command_json = json.dumps(command)

            # 조이스틱에 시작 명령 전송
            result_joystick = self.client.publish(
                self.config.TOPIC_CMD_JOYSTICK,
                command_json,
                qos=self.config.MQTT_QOS
            )

            # Watch에 시작 명령 전송
            result_watch = self.client.publish(
                self.config.TOPIC_CMD_WATCH,
                command_json,
                qos=self.config.MQTT_QOS
            )

            if result_joystick.rc == 0 and result_watch.rc == 0:
                print(f"✓ 시작 명령 전송 완료: {mode}")
                return True
            else:
                print(f"✗ 시작 명령 전송 실패")
                return False

        except Exception as e:
            print(f"✗ 시작 명령 전송 오류: {e}")
            return False

    def send_stop_command(self):
        """
        조이스틱과 Watch에 정지 명령 전송

        Returns:
            bool: 전송 성공 여부
        """
        if not self.connected:
            print("✗ MQTT 브로커에 연결되지 않음")
            return False

        try:
            # 정지 명령 메시지
            command = {
                'command': 'stop',
                'timestamp': int(time.time() * 1000)
            }

            command_json = json.dumps(command)

            # 조이스틱에 정지 명령 전송
            result_joystick = self.client.publish(
                self.config.TOPIC_CMD_JOYSTICK,
                command_json,
                qos=self.config.MQTT_QOS
            )

            # Watch에 정지 명령 전송
            result_watch = self.client.publish(
                self.config.TOPIC_CMD_WATCH,
                command_json,
                qos=self.config.MQTT_QOS
            )

            if result_joystick.rc == 0 and result_watch.rc == 0:
                print("✓ 정지 명령 전송 완료")
                return True
            else:
                print("✗ 정지 명령 전송 실패")
                return False

        except Exception as e:
            print(f"✗ 정지 명령 전송 오류: {e}")
            return False

    def get_joystick_data(self):
        """
        최신 조이스틱 센서 데이터 가져오기

        Returns:
            dict: 조이스틱 센서 데이터
        """
        return self.joystick_data.copy()

    def get_watch_data(self):
        """
        최신 Watch 심박수 데이터 가져오기

        Returns:
            dict: Watch 심박수 데이터
        """
        return self.watch_data.copy()

    def is_joystick_alive(self, timeout=None):
        """
        조이스틱 연결 상태 확인

        Args:
            timeout: 타임아웃 시간 (초, None이면 기본값 사용)

        Returns:
            bool: 연결 상태
        """
        if timeout is None:
            timeout = self.config.DEVICE_TIMEOUT

        if self.joystick_data['last_update'] is None:
            return False

        elapsed = time.time() - self.joystick_data['last_update']
        return elapsed < timeout

    def is_watch_alive(self, timeout=None):
        """
        Watch 연결 상태 확인

        Args:
            timeout: 타임아웃 시간 (초, None이면 기본값 사용)

        Returns:
            bool: 연결 상태
        """
        if timeout is None:
            timeout = self.config.DEVICE_TIMEOUT

        if self.watch_data['last_update'] is None:
            return False

        elapsed = time.time() - self.watch_data['last_update']
        return elapsed < timeout

    def wait_for_devices(self, timeout=10.0):
        """
        디바이스들이 준비될 때까지 대기

        Args:
            timeout: 최대 대기 시간 (초)

        Returns:
            bool: 모든 디바이스가 준비되었는지 여부
        """
        print("→ 디바이스 준비 대기 중...")
        start_time = time.time()

        while (time.time() - start_time) < timeout:
            if self.joystick_ready and self.watch_ready:
                print("✓ 모든 디바이스 준비 완료")
                return True

            time.sleep(0.5)

        # 타임아웃
        print("⚠ 디바이스 준비 타임아웃")
        if not self.joystick_ready:
            print("  ✗ 조이스틱 준비 안됨")
        if not self.watch_ready:
            print("  ✗ Watch 준비 안됨")

        return False

    def send_qt_mode_selected(self, mode, success=True, message=""):
        """
        Qt에 운동 모드 선택 완료 응답 전송

        Args:
            mode: 선택된 운동 모드
            success: 성공 여부
            message: 추가 메시지
        """
        if not self.connected:
            return

        try:
            response = {
                'mode': mode,
                'success': success,
                'message': message,
                'timestamp': int(time.time() * 1000)
            }

            result = self.client.publish(
                self.config.TOPIC_QT_RESPONSE_MODE,
                json.dumps(response),
                qos=self.config.MQTT_QOS
            )

            if result.rc == 0:
                print(f"✓ Qt에 모드 선택 응답 전송: {mode}")
            else:
                print(f"✗ Qt 응답 전송 실패")

        except Exception as e:
            print(f"✗ Qt 응답 전송 오류: {e}")

    def send_qt_error(self, error_message):
        """
        Qt에 에러 메시지 전송

        Args:
            error_message: 에러 메시지
        """
        if not self.connected:
            return

        try:
            response = {
                'error': error_message,
                'timestamp': int(time.time() * 1000)
            }

            result = self.client.publish(
                self.config.TOPIC_QT_RESPONSE_ERROR,
                json.dumps(response),
                qos=self.config.MQTT_QOS
            )

            if result.rc == 0:
                print(f"✓ Qt에 에러 응답 전송: {error_message}")
            else:
                print(f"✗ Qt 에러 응답 전송 실패")

        except Exception as e:
            print(f"✗ Qt 에러 응답 전송 오류: {e}")

    def send_qt_analysis_result(self, analysis_data):
        """
        Qt에 AI 분석 결과 전송

        Args:
            analysis_data: AI 분석 결과 딕셔너리
        """
        if not self.connected:
            return

        try:
            result = self.client.publish(
                self.config.TOPIC_QT_RESPONSE_ANALYSIS,
                json.dumps(analysis_data),
                qos=self.config.MQTT_QOS
            )

            if result.rc != 0:
                print(f"✗ Qt 분석 결과 전송 실패")

        except Exception as e:
            print(f"✗ Qt 분석 결과 전송 오류: {e}")

    def send_qt_status(self, status, message=""):
        """
        Qt에 상태 응답 전송

        Args:
            status: 상태 ("stopped", "running", etc.)
            message: 추가 메시지
        """
        if not self.connected:
            return

        try:
            response = {
                'status': status,
                'message': message,
                'timestamp': int(time.time() * 1000)
            }

            result = self.client.publish(
                self.config.TOPIC_QT_RESPONSE_STATUS,
                json.dumps(response),
                qos=self.config.MQTT_QOS
            )

            if result.rc == 0:
                print(f"✓ Qt에 상태 응답 전송: {status}")
            else:
                print(f"✗ Qt 상태 응답 전송 실패")

        except Exception as e:
            print(f"✗ Qt 상태 응답 전송 오류: {e}")


def main():
    """테스트용 메인 함수"""
    print("="*50)
    print("  MQTT Controller 테스트")
    print("="*50)

    controller = MQTTController()

    # 콜백 함수 등록
    def on_joystick_data(data):
        print(f"[조이스틱] Accel: ({data['accel_x']:.2f}, {data['accel_y']:.2f}, {data['accel_z']:.2f}) "
              f"Gyro: ({data['gyro_x']:.2f}, {data['gyro_y']:.2f}, {data['gyro_z']:.2f})")

    def on_watch_data(data):
        print(f"[Watch] 심박수: {data['heart_rate']} bpm")

    controller.on_joystick_data_callback = on_joystick_data
    controller.on_watch_data_callback = on_watch_data

    # MQTT 연결
    if not controller.connect():
        print("✗ MQTT 브로커 연결 실패")
        return

    try:
        print("\n→ 센서 데이터 수신 중 (Ctrl+C로 종료)...\n")

        # 무한 루프 - 사용자가 Ctrl+C로 종료할 때까지 실행
        while True:
            time.sleep(0.1)  # CPU 과부하 방지

    except KeyboardInterrupt:
        print("\n⏸ 사용자에 의해 중지됨")
        controller.send_stop_command()
    finally:
        controller.disconnect()


if __name__ == "__main__":
    main()
