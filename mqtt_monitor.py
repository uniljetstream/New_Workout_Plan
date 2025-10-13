#!/usr/bin/env python3
"""
MQTT 모니터
모든 MQTT 토픽을 수신하여 화면에 표시
"""

import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime


class MQTTMonitor:
    """MQTT 메시지 모니터링 클래스"""

    def __init__(self, broker_host="localhost", broker_port=1883):
        """
        초기화

        Args:
            broker_host: MQTT 브로커 주소
            broker_port: MQTT 브로커 포트
        """
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.client = mqtt.Client(client_id="mqtt_monitor")
        self.message_count = 0

        # 콜백 설정
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect

    def on_connect(self, client, userdata, flags, rc):
        """
        브로커 연결 시 호출되는 콜백

        Args:
            rc: 연결 결과 코드 (0 = 성공)
        """
        if rc == 0:
            print("=" * 70)
            print(f"✓ MQTT 브로커 연결 성공: {self.broker_host}:{self.broker_port}")
            print("=" * 70)

            # 모든 토픽 구독 (# 와일드카드 사용)
            client.subscribe("#", qos=0)
            print("✓ 모든 토픽 구독 시작 (#)")
            print("=" * 70)
            print(f"{'시간':<12} | {'토픽':<30} | {'메시지'}")
            print("-" * 70)
        else:
            print(f"✗ 브로커 연결 실패: 코드 {rc}")

    def on_message(self, client, userdata, msg):
        """
        메시지 수신 시 호출되는 콜백

        Args:
            msg: 수신된 MQTT 메시지
        """
        self.message_count += 1
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        topic = msg.topic
        payload = msg.payload.decode('utf-8', errors='replace')

        # JSON 형식이면 파싱하여 예쁘게 출력
        try:
            json_data = json.loads(payload)
            payload_str = json.dumps(json_data, ensure_ascii=False)
        except:
            payload_str = payload

        # 메시지 출력
        print(f"{timestamp} | {topic:<30} | {payload_str}")

    def on_disconnect(self, client, userdata, rc):
        """
        브로커 연결 해제 시 호출되는 콜백

        Args:
            rc: 연결 해제 코드
        """
        if rc != 0:
            print(f"\n✗ 브로커 연결이 끊어졌습니다: 코드 {rc}")
        else:
            print(f"\n✓ 브로커 연결 정상 종료")

    def start(self):
        """모니터링 시작"""
        try:
            print(f"\n🔍 MQTT 모니터 시작")
            print(f"브로커: {self.broker_host}:{self.broker_port}")
            print(f"종료: Ctrl+C\n")

            # 브로커에 연결
            self.client.connect(self.broker_host, self.broker_port, keepalive=60)

            # 메시지 수신 루프 시작 (블로킹)
            self.client.loop_forever()

        except KeyboardInterrupt:
            print(f"\n\n⏸ 사용자에 의해 중지됨")
            self.stop()
        except Exception as e:
            print(f"\n✗ 오류 발생: {e}")
            self.stop()

    def stop(self):
        """모니터링 중지"""
        print(f"\n📊 통계:")
        print(f"  총 수신 메시지: {self.message_count}개")
        print(f"\n🧹 정리 중...")

        self.client.disconnect()
        self.client.loop_stop()
        print("✓ MQTT 클라이언트 종료 완료")


def main():
    """메인 진입점"""
    # WatchTower (Jetson Nano)가 브로커 역할을 하는 경우
    # 로컬에서 실행 시 "localhost"
    # 원격에서 실행 시 WatchTower의 IP 주소로 변경
    BROKER_HOST = "localhost"  # 또는 "192.168.1.100" 등
    BROKER_PORT = 1883

    monitor = MQTTMonitor(broker_host=BROKER_HOST, broker_port=BROKER_PORT)
    monitor.start()


if __name__ == "__main__":
    main()
