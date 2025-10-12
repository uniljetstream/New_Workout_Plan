#!/usr/bin/env python3
"""
UART 컨트롤러 모듈
STM32와 UART 통신을 통해 팬틸트 서보 모터를 제어
"""

import serial
import time
from watchtower_config import WatchTowerConfig


class UARTController:
    """STM32와 UART 통신을 담당하는 클래스"""

    def __init__(self, port=None, baudrate=None):
        """
        초기화

        Args:
            port: UART 포트 경로 (None이면 config에서 가져옴)
            baudrate: 통신 속도 (None이면 config에서 가져옴)
        """
        self.port = port or WatchTowerConfig.UART_PORT
        self.baudrate = baudrate or WatchTowerConfig.UART_BAUDRATE
        self.timeout = WatchTowerConfig.UART_TIMEOUT
        self.serial = None
        self.is_connected = False

    def connect(self):
        """
        UART 포트 연결

        Returns:
            bool: 연결 성공 여부
        """
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )

            # 연결 확인을 위한 짧은 대기
            time.sleep(0.1)

            if self.serial.is_open:
                self.is_connected = True
                print(f"✓ UART 연결 성공: {self.port} @ {self.baudrate} baud")

                # 초기화: 중앙 위치로 이동
                self.center()
                return True
            else:
                print(f"✗ UART 포트를 열 수 없습니다: {self.port}")
                return False

        except serial.SerialException as e:
            print(f"✗ UART 연결 실패: {e}")
            print(f"  포트: {self.port}")
            print(f"  권한 확인: sudo usermod -a -G dialout $USER")
            return False
        except Exception as e:
            print(f"✗ UART 초기화 오류: {e}")
            return False

    def disconnect(self):
        """UART 포트 연결 해제"""
        if self.serial and self.serial.is_open:
            try:
                # 중앙 위치로 복귀 후 연결 해제
                self.center()
                time.sleep(0.2)
                self.serial.close()
                self.is_connected = False
                print("✓ UART 연결 해제 완료")
            except Exception as e:
                print(f"✗ UART 연결 해제 오류: {e}")

    def send_command(self, command):
        """
        명령어 전송

        Args:
            command: 전송할 명령어 문자열

        Returns:
            bool: 전송 성공 여부
        """
        if not self.is_connected or not self.serial:
            print("✗ UART가 연결되지 않았습니다")
            return False

        try:
            # 명령어에 개행 문자 추가
            if not command.endswith('\n'):
                command += '\n'

            # 명령 전송
            self.serial.write(command.encode('utf-8'))
            self.serial.flush()

            return True

        except serial.SerialException as e:
            print(f"✗ UART 전송 오류: {e}")
            return False
        except Exception as e:
            print(f"✗ 명령 전송 실패: {e}")
            return False

    def send_pan(self, angle):
        """
        Pan 각도 전송

        Args:
            angle: Pan 각도 (-60~60도)

        Returns:
            bool: 전송 성공 여부
        """
        # 각도 제한
        angle = max(WatchTowerConfig.PAN_MIN, min(WatchTowerConfig.PAN_MAX, angle))

        command = f"PAN:{int(angle)}"
        return self.send_command(command)

    def send_tilt(self, angle):
        """
        Tilt 각도 전송

        Args:
            angle: Tilt 각도 (-60~60도)

        Returns:
            bool: 전송 성공 여부
        """
        # 각도 제한
        angle = max(WatchTowerConfig.TILT_MIN, min(WatchTowerConfig.TILT_MAX, angle))

        command = f"TILT:{int(angle)}"
        return self.send_command(command)

    def send_pan_tilt(self, pan_angle, tilt_angle, verbose=False):
        """
        Pan과 Tilt 각도를 동시에 전송

        Args:
            pan_angle: Pan 각도 (-60~60도)
            tilt_angle: Tilt 각도 (-60~60도)
            verbose: 상세 로그 출력 여부

        Returns:
            bool: 전송 성공 여부
        """
        # 각도 제한
        pan_angle = max(WatchTowerConfig.PAN_MIN, min(WatchTowerConfig.PAN_MAX, pan_angle))
        tilt_angle = max(WatchTowerConfig.TILT_MIN, min(WatchTowerConfig.TILT_MAX, tilt_angle))

        command = f"PANTILT:{int(pan_angle)},{int(tilt_angle)}"
        success = self.send_command(command)

        if verbose:
            status = "✓" if success else "✗"
            print(f"    {status} UART 전송: {command}")

        return success

    def center(self):
        """
        카메라를 중앙 위치로 이동

        Returns:
            bool: 전송 성공 여부
        """
        command = "CENTER"
        return self.send_command(command)

    def stop(self):
        """
        서보 모터 정지

        Returns:
            bool: 전송 성공 여부
        """
        command = "STOP"
        return self.send_command(command)

    def __enter__(self):
        """with 문 지원"""
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """with 문 종료 시 자동 연결 해제"""
        self.disconnect()


def main():
    """테스트용 메인 함수"""
    print("=" * 50)
    print("  UART 컨트롤러 테스트")
    print("=" * 50)

    # UART 컨트롤러 초기화
    uart = UARTController()

    if not uart.connect():
        print("✗ UART 연결 실패")
        return

    try:
        # 테스트 시퀀스
        print("\n📍 중앙 위치로 이동...")
        uart.center()
        time.sleep(1)

        print("\n📍 왼쪽으로 Pan (-30도)...")
        uart.send_pan(-30)
        time.sleep(1)

        print("\n📍 오른쪽으로 Pan (30도)...")
        uart.send_pan(30)
        time.sleep(1)

        print("\n📍 위로 Tilt (-30도)...")
        uart.send_tilt(-30)
        time.sleep(1)

        print("\n📍 아래로 Tilt (30도)...")
        uart.send_tilt(30)
        time.sleep(1)

        print("\n📍 중앙 복귀...")
        uart.center()
        time.sleep(1)

        print("\n✓ 테스트 완료!")

    except KeyboardInterrupt:
        print("\n⏸ 사용자에 의해 중지됨")
    finally:
        uart.disconnect()


if __name__ == "__main__":
    main()
