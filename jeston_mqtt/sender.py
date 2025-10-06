#!/usr/bin/env python3
"""
MQTT Token Sender for Testing
토큰 수신 테스트를 위한 발행 스크립트
"""

import paho.mqtt.client as mqtt
import json
import time
import random
import string
from datetime import datetime

# MQTT 브로커 설정
MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC = "token/receive"
MQTT_CLIENT_ID = "jetson_nano_token_sender"

def generate_random_token():
    """랜덤 토큰 생성"""
    token_length = random.randint(16, 32)
    return ''.join(random.choices(string.ascii_letters + string.digits, k=token_length))

def generate_jwt_style_token():
    """JWT 스타일의 토큰 생성 (예시)"""
    header = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"
    payload = ''.join(random.choices(string.ascii_letters + string.digits, k=40))
    signature = ''.join(random.choices(string.ascii_letters + string.digits, k=20))
    return f"{header}.{payload}.{signature}"

def send_test_tokens():
    """테스트 토큰 발송"""
    # MQTT 클라이언트 생성
    client = mqtt.Client(client_id=MQTT_CLIENT_ID)
    
    try:
        print("📡 MQTT 브로커 연결 중...")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        time.sleep(1)  # 연결 대기
        
        print(f"✅ 브로커 연결 성공: {MQTT_BROKER}:{MQTT_PORT}")
        print(f"📤 토픽: {MQTT_TOPIC}\n")
        print("="*60)
        
        # 다양한 형태의 토큰 발송
        test_cases = [
            {
                "type": "simple_token",
                "data": generate_random_token()
            },
            {
                "type": "jwt_style",
                "data": generate_jwt_style_token()
            },
            {
                "type": "json_token",
                "data": json.dumps({
                    "token": generate_random_token(),
                    "user_id": f"user_{random.randint(1000, 9999)}",
                    "timestamp": datetime.now().isoformat(),
                    "permissions": ["read", "write"],
                    "expires_in": 3600
                })
            },
            {
                "type": "api_key",
                "data": f"sk-{generate_random_token()}"
            },
            {
                "type": "bearer_token",
                "data": f"Bearer {generate_jwt_style_token()}"
            }
        ]
        
        print("🚀 토큰 발송 시작...\n")
        
        for i, test_case in enumerate(test_cases, 1):
            print(f"[{i}/{len(test_cases)}] 토큰 타입: {test_case['type']}")
            print(f"    발송 중...")
            
            # 토큰 발송
            result = client.publish(MQTT_TOPIC, test_case['data'])
            
            if result.rc == 0:
                print(f"    ✅ 발송 성공!")
                print(f"    📦 내용: {test_case['data'][:50]}..." if len(test_case['data']) > 50 else f"    📦 내용: {test_case['data']}")
            else:
                print(f"    ❌ 발송 실패! (에러 코드: {result.rc})")
            
            print("-"*40)
            time.sleep(2)  # 각 토큰 사이 2초 대기
        
        print("\n" + "="*60)
        print("✅ 모든 테스트 토큰 발송 완료!")
        
        # 추가 수동 발송 옵션
        print("\n💡 수동으로 토큰을 발송하려면 Enter를 누르세요.")
        print("   종료하려면 'q'를 입력하세요.\n")
        
        while True:
            user_input = input("토큰 발송 (Enter) 또는 종료 (q): ").strip().lower()
            
            if user_input == 'q':
                break
            
            # 랜덤 토큰 생성 및 발송
            token = {
                "token": generate_random_token(),
                "timestamp": datetime.now().isoformat(),
                "session_id": f"session_{random.randint(10000, 99999)}"
            }
            
            result = client.publish(MQTT_TOPIC, json.dumps(token))
            
            if result.rc == 0:
                print(f"✅ 토큰 발송 성공: {token['token']}")
            else:
                print(f"❌ 발송 실패! (에러 코드: {result.rc})")
        
    except KeyboardInterrupt:
        print("\n⚠️ 사용자에 의해 중단됨")
    except Exception as e:
        print(f"❌ 오류 발생: {e}")
    finally:
        print("\n👋 연결 종료 중...")
        client.loop_stop()
        client.disconnect()
        print("✅ 프로그램 종료")

def main():
    """메인 함수"""
    print("\n" + "🎯"*20)
    print("  MQTT Token Sender (테스트용)")
    print("  다양한 형태의 토큰을 발송합니다")
    print("🎯"*20 + "\n")
    
    send_test_tokens()

if __name__ == "__main__":
    main()