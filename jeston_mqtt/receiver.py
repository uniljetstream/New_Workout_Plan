#!/usr/bin/env python3
"""
MQTT Token Receiver for Jetson Nano
이 스크립트는 MQTT 브로커로부터 토큰을 수신하여 터미널에 표시합니다.
"""

import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime
import sys

# MQTT 브로커 설정
MQTT_BROKER = "localhost"  # Mosquitto가 로컬에서 실행중인 경우
MQTT_PORT = 1883
MQTT_TOPIC = "esp32/sensor/data"  # 토큰을 수신할 토픽
MQTT_CLIENT_ID = "jetson_nano_token_receiver"

# MQTT 연결 설정 (필요시 수정)
MQTT_USERNAME = ""  # 인증이 필요한 경우 설정
MQTT_PASSWORD = ""  # 인증이 필요한 경우 설정

class TokenReceiver:
    def __init__(self):
        # MQTT 클라이언트 초기화 (새로운 API 버전 사용)
        self.client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id=MQTT_CLIENT_ID
        )
        
        # 콜백 함수 설정
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
        # 인증 설정 (필요한 경우)
        if MQTT_USERNAME and MQTT_PASSWORD:
            self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    def on_connect(self, client, userdata, flags, reason_code, properties):
        """브로커 연결 시 호출되는 콜백 (API v2)"""
        if reason_code == 0:
            print(f"[{self.get_timestamp()}] ✅ MQTT 브로커에 성공적으로 연결되었습니다.")
            print(f"[{self.get_timestamp()}] 📡 토픽 구독 중: {MQTT_TOPIC}")
            client.subscribe(MQTT_TOPIC)
        else:
            print(f"[{self.get_timestamp()}] ❌ 연결 실패. 에러 코드: {reason_code}")
    
    def on_message(self, client, userdata, msg):
        """메시지 수신 시 호출되는 콜백"""
        try:
            # 수신된 메시지 디코딩
            payload = msg.payload.decode('utf-8')
            
            print("\n" + "="*60)
            print(f"[{self.get_timestamp()}] 🔔 새로운 토큰 수신!")
            print(f"토픽: {msg.topic}")
            print("-"*60)
            
            # JSON 형식인 경우 파싱 시도
            try:
                token_data = json.loads(payload)
                print("📦 토큰 내용 (JSON):")
                for key, value in token_data.items():
                    print(f"  • {key}: {value}")
            except json.JSONDecodeError:
                # JSON이 아닌 경우 문자열로 표시
                print(f"📦 토큰 내용 (문자열): {payload}")
            
            print("="*60 + "\n")
            
            # 토큰 로그 파일에 저장 (선택사항)
            self.save_token_to_file(payload)
            
        except Exception as e:
            print(f"[{self.get_timestamp()}] ⚠️ 메시지 처리 중 오류: {e}")
    
    def on_disconnect(self, client, userdata, disconnect_flags, reason_code, properties):
        """연결 해제 시 호출되는 콜백 (API v2)"""
        if reason_code != 0:
            print(f"[{self.get_timestamp()}] ⚠️ 예기치 않은 연결 해제. 에러 코드: {reason_code}")
            print(f"[{self.get_timestamp()}] 🔄 재연결 시도 중...")
    
    def get_timestamp(self):
        """현재 시간을 문자열로 반환"""
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    def save_token_to_file(self, token):
        """수신된 토큰을 파일에 저장 (선택사항)"""
        try:
            with open("received_tokens.log", "a") as f:
                f.write(f"[{self.get_timestamp()}] {token}\n")
        except Exception as e:
            print(f"[{self.get_timestamp()}] ⚠️ 토큰 저장 실패: {e}")
    
    def check_broker_connection(self):
        """브로커 연결 가능 여부 확인"""
        import socket
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(2)
            result = sock.connect_ex((MQTT_BROKER, MQTT_PORT))
            sock.close()
            return result == 0
        except:
            return False
    
    def start(self):
        """MQTT 클라이언트 시작"""
        try:
            print(f"[{self.get_timestamp()}] 🚀 MQTT Token Receiver 시작")
            
            # 브로커 연결 가능 여부 체크
            print(f"[{self.get_timestamp()}] 🔍 브로커 연결 가능 여부 확인 중...")
            if not self.check_broker_connection():
                print(f"[{self.get_timestamp()}] ❌ 브로커에 연결할 수 없습니다!")
                print("\n📝 해결 방법:")
                print("  1. Mosquitto 설치 확인: sudo apt-get install mosquitto")
                print("  2. Mosquitto 시작: sudo systemctl start mosquitto")
                print("  3. 상태 확인: sudo systemctl status mosquitto")
                print("  4. 방화벽 확인: sudo ufw allow 1883/tcp")
                return
            
            print(f"[{self.get_timestamp()}] 📡 브로커 연결 시도: {MQTT_BROKER}:{MQTT_PORT}")
            
            # 브로커에 연결
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            
            # 메시지 루프 시작
            self.client.loop_forever()
            
        except KeyboardInterrupt:
            print(f"\n[{self.get_timestamp()}] 👋 프로그램 종료 중...")
            self.client.disconnect()
            self.client.loop_stop()
            print(f"[{self.get_timestamp()}] ✅ 프로그램이 정상적으로 종료되었습니다.")
            
        except Exception as e:
            print(f"[{self.get_timestamp()}] ❌ 오류 발생: {e}")
            print("\n💡 일반적인 해결 방법:")
            print("  • Mosquitto 설치: sudo apt-get install mosquitto")
            print("  • paho-mqtt 설치: pip3 install paho-mqtt")
            print("  • Mosquitto 시작: sudo systemctl start mosquitto")
            return

def main():
    """메인 함수"""
    print("\n" + "🎯"*20)
    print("  MQTT Token Receiver for Jetson Nano")
    print("  Mosquitto 브로커로부터 토큰을 수신합니다")
    print("🎯"*20 + "\n")
    
    # paho-mqtt 버전 확인
    try:
        import paho.mqtt
        mqtt_version = paho.mqtt.__version__
        print(f"📚 paho-mqtt 버전: {mqtt_version}")
        
        # 버전이 2.0 미만인 경우 경고
        if mqtt_version < "2.0.0":
            print("⚠️  paho-mqtt를 최신 버전으로 업그레이드하는 것을 권장합니다:")
            print("    pip3 install --upgrade paho-mqtt\n")
    except:
        pass
    
    # 설정 정보 출력
    print("📋 현재 설정:")
    print(f"  • 브로커 주소: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"  • 구독 토픽: {MQTT_TOPIC}")
    print(f"  • 클라이언트 ID: {MQTT_CLIENT_ID}\n")
    
    # TokenReceiver 인스턴스 생성 및 시작
    receiver = TokenReceiver()
    receiver.start()

if __name__ == "__main__":
    main()