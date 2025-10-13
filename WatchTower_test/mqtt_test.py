#!/usr/bin/env python3
"""
WatchTower MQTT 통신 테스트
조이스틱(ESP32)과 Watch(ESP32) MQTT 통신 테스트를 위한 CLI 인터페이스
"""

import time
import threading
from datetime import datetime
from mqtt_controller import MQTTController
from mqtt_config import MQTTTestConfig


class MQTTTest:
    """MQTT 통신 테스트 클래스"""

    def __init__(self):
        """초기화"""
        self.mqtt = MQTTController()
        self.running = True
        self.config = MQTTTestConfig

        # 통계
        self.stats = {
            'start_time': None,
            'joystick_data_count': 0,
            'watch_data_count': 0,
            'joystick_status_count': 0,
            'watch_status_count': 0,
            'command_sent_count': 0,
            'last_joystick_time': None,
            'last_watch_time': None
        }

        # 마지막 센서 데이터 (화면 업데이트용)
        self.last_joystick_str = ""
        self.last_watch_str = ""

        # 콜백 등록
        self.mqtt.on_joystick_data_callback = self.on_joystick_data
        self.mqtt.on_watch_data_callback = self.on_watch_data
        self.mqtt.on_joystick_status_callback = self.on_joystick_status
        self.mqtt.on_watch_status_callback = self.on_watch_status

    def on_joystick_data(self, data):
        """
        조이스틱 데이터 수신 콜백

        Args:
            data: 조이스틱 센서 데이터 딕셔너리
        """
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3] if self.config.SHOW_TIMESTAMP else ""
        prec = self.config.SENSOR_DATA_PRECISION

        data_str = (f"[조이스틱] Accel: ({data['accel_x']:.{prec}f}, "
                   f"{data['accel_y']:.{prec}f}, {data['accel_z']:.{prec}f}) | "
                   f"Gyro: ({data['gyro_x']:.{prec}f}, {data['gyro_y']:.{prec}f}, "
                   f"{data['gyro_z']:.{prec}f})")

        if timestamp:
            print(f"{timestamp} | {data_str}")
        else:
            print(data_str)

        self.last_joystick_str = data_str
        self.stats['joystick_data_count'] += 1
        self.stats['last_joystick_time'] = time.time()

    def on_watch_data(self, data):
        """
        Watch 데이터 수신 콜백

        Args:
            data: Watch 센서 데이터 딕셔너리
        """
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3] if self.config.SHOW_TIMESTAMP else ""

        data_str = f"[Watch] 심박수: {data['heart_rate']} bpm"

        if timestamp:
            print(f"{timestamp} | {data_str}")
        else:
            print(data_str)

        self.last_watch_str = data_str
        self.stats['watch_data_count'] += 1
        self.stats['last_watch_time'] = time.time()

    def on_joystick_status(self, status):
        """
        조이스틱 상태 수신 콜백

        Args:
            status: 상태 문자열 (ready, stopped, etc.)
        """
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3] if self.config.SHOW_TIMESTAMP else ""

        status_str = f"[조이스틱 상태] {status}"

        if timestamp:
            print(f"{timestamp} | {status_str}")
        else:
            print(status_str)

        self.stats['joystick_status_count'] += 1

    def on_watch_status(self, status):
        """
        Watch 상태 수신 콜백

        Args:
            status: 상태 문자열 (ready, stopped, etc.)
        """
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3] if self.config.SHOW_TIMESTAMP else ""

        status_str = f"[Watch 상태] {status}"

        if timestamp:
            print(f"{timestamp} | {status_str}")
        else:
            print(status_str)

        self.stats['watch_status_count'] += 1

    def send_start_command(self, mode=None):
        """
        디바이스에 시작 명령 전송

        Args:
            mode: 운동 모드 (None이면 기본값 사용)
        """
        if mode is None:
            mode = self.config.DEFAULT_MODE

        if self.mqtt.send_start_command(mode):
            self.stats['command_sent_count'] += 1

    def send_stop_command(self):
        """디바이스에 정지 명령 전송"""
        if self.mqtt.send_stop_command():
            self.stats['command_sent_count'] += 1

    def print_stats(self):
        """통계 출력"""
        if self.stats['start_time'] is None:
            print("\n⚠ 아직 통계 데이터가 없습니다.\n")
            return

        elapsed = time.time() - self.stats['start_time']

        print("\n" + "="*60)
        print("📊 MQTT 통신 통계")
        print("="*60)
        print(f"  실행 시간: {elapsed:.1f}초")
        print(f"  명령 전송: {self.stats['command_sent_count']}회")
        print()
        print(f"  [조이스틱]")
        print(f"    데이터 수신: {self.stats['joystick_data_count']}개", end='')
        if elapsed > 0 and self.stats['joystick_data_count'] > 0:
            print(f" (평균 {self.stats['joystick_data_count']/elapsed:.1f}/sec)")
        else:
            print()
        print(f"    상태 수신: {self.stats['joystick_status_count']}개")
        print(f"    연결 상태: {'✓ 활성' if self.mqtt.is_joystick_alive() else '✗ 비활성'}")

        print()
        print(f"  [Watch]")
        print(f"    데이터 수신: {self.stats['watch_data_count']}개", end='')
        if elapsed > 0 and self.stats['watch_data_count'] > 0:
            print(f" (평균 {self.stats['watch_data_count']/elapsed:.1f}/sec)")
        else:
            print()
        print(f"    상태 수신: {self.stats['watch_status_count']}개")
        print(f"    연결 상태: {'✓ 활성' if self.mqtt.is_watch_alive() else '✗ 비활성'}")
        print("="*60 + "\n")

    def show_menu(self):
        """메뉴 출력"""
        print("\n" + "="*60)
        print("명령어:")
        print("  1 - 디바이스 시작 (start 명령 전송)")
        print("  2 - 디바이스 정지 (stop 명령 전송)")
        print("  3 - 통계 출력")
        print("  4 - 메뉴 다시 보기")
        print("  q - 종료")
        print("="*60)

    def handle_command_input(self):
        """
        사용자 명령 입력 처리 (별도 스레드에서 실행)
        """
        while self.running:
            try:
                cmd = input("\n> ").strip().lower()

                if cmd == '1':
                    print(f"\n→ 시작 명령 전송 (모드: {self.config.DEFAULT_MODE})")
                    self.send_start_command()

                elif cmd == '2':
                    print("\n→ 정지 명령 전송")
                    self.send_stop_command()

                elif cmd == '3':
                    self.print_stats()

                elif cmd == '4':
                    self.show_menu()

                elif cmd == 'q':
                    print("\n→ 종료 중...")
                    self.running = False
                    break

                elif cmd == '':
                    # 빈 입력 무시
                    continue

                else:
                    print(f"⚠ 알 수 없는 명령: {cmd}")
                    print("   도움말: 4를 입력하여 메뉴를 확인하세요.")

            except EOFError:
                # Ctrl+D 입력 시
                print("\n→ 종료 중...")
                self.running = False
                break
            except Exception as e:
                print(f"✗ 입력 처리 오류: {e}")

    def run(self):
        """메인 실행 루프"""
        print("="*60)
        print("  WatchTower MQTT 통신 테스트")
        print("="*60)
        print(f"MQTT 브로커: {self.config.MQTT_BROKER_HOST}:{self.config.MQTT_BROKER_PORT}")
        print("="*60)
        print()

        # MQTT 연결
        print("[초기화]")
        if not self.mqtt.connect():
            print("\n✗ MQTT 브로커 연결 실패")
            print("   확인 사항:")
            print("   1. MQTT 브로커(mosquitto)가 실행 중인지 확인")
            print("      sudo systemctl status mosquitto")
            print("   2. mqtt_config.py에서 브로커 주소가 올바른지 확인")
            return

        self.stats['start_time'] = time.time()

        # 메뉴 표시
        self.show_menu()

        print("\n[실시간 센서 데이터]")
        print("-"*60)

        # 명령 입력 스레드 시작
        input_thread = threading.Thread(target=self.handle_command_input, daemon=True)
        input_thread.start()

        # 메인 루프 (센서 데이터 수신 대기)
        try:
            while self.running:
                time.sleep(0.1)  # CPU 과부하 방지

        except KeyboardInterrupt:
            print("\n\n⏸ 사용자에 의해 중지됨 (Ctrl+C)")
            self.running = False

        finally:
            # 종료 처리
            print("\n[종료 처리]")
            self.send_stop_command()
            time.sleep(0.5)  # 명령 전송 대기

            # 최종 통계 출력
            self.print_stats()

            # MQTT 연결 해제
            self.mqtt.disconnect()

            print("\n✓ 테스트 종료\n")


def main():
    """메인 진입점"""
    test = MQTTTest()
    test.run()


if __name__ == "__main__":
    main()
