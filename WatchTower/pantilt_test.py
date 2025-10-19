#!/usr/bin/env python3
"""
팬틸트 모터 기본 동작 확인용 테스트 스크립트
UARTController를 사용해 Pan/Tilt 서보에 단계적인 각도를 전송한다.
"""

import time
from contextlib import contextmanager

from uart_controller import UARTController
from watchtower_config import WatchTowerConfig


@contextmanager
def pantilt_connection():
    """UARTController 연결을 context manager 형태로 감싼 헬퍼."""
    controller = UARTController()
    connected = controller.connect()
    try:
        if not connected:
            raise RuntimeError("팬틸트 UART 연결 실패")
        yield controller
    finally:
        controller.disconnect()


def run_test():
    """
    간단한 시퀀스를 통해 팬틸트가 정상적으로 구동되는지 확인한다.

    - 중앙 위치에서 시작
    - 좌우/상하 기본 위치로 이동
    - 작은 스윕 동작으로 연속 움직임 확인
    """
    print("=" * 50)
    print(" 팬틸트 모터 동작 확인 테스트 ")
    print("=" * 50)
    print(f"UART 포트: {WatchTowerConfig.UART_PORT}")
    print(f"Baudrate: {WatchTowerConfig.UART_BAUDRATE}")
    print("Ctrl+C 로 종료할 수 있습니다.\n")

    try:
        with pantilt_connection() as uart:
            print("→ 중앙 위치로 이동")
            uart.center()
            time.sleep(1.0)

            test_positions = [
                ("왼쪽", -30, 0),
                ("오른쪽", 30, 0),
                ("위쪽", 0, 20),
                ("아래쪽", 0, -20),
                ("대각선(좌상)", -20, 15),
                ("대각선(우하)", 20, -15),
            ]

            for label, pan, tilt in test_positions:
                print(f"→ {label} 이동 (Pan={pan}, Tilt={tilt})")
                uart.send_pan_tilt(pan, tilt, verbose=True)
                time.sleep(1.5)

            print("\n→ 연속 스윕 테스트 (Pan -40 → 40)")
            for pan in range(-40, 41, 10):
                uart.send_pan_tilt(pan, 0, verbose=True)
                time.sleep(0.6)

            print("\n→ 연속 스윕 테스트 (Tilt -30 → 30)")
            for tilt in range(-30, 31, 10):
                uart.send_pan_tilt(0, tilt, verbose=True)
                time.sleep(0.6)

            print("\n→ 테스트 종료, 중앙 위치 복귀")
            uart.center()
            time.sleep(1.0)

    except KeyboardInterrupt:
        print("\n사용자 중지 감지, 중앙 위치로 복귀합니다.")
        with pantilt_connection() as uart:
            uart.center()
    except Exception as exc:
        print(f"\n✗ 테스트 중 오류 발생: {exc}")

    print("\n✓ 팬틸트 테스트 종료")


if __name__ == "__main__":
    run_test()
