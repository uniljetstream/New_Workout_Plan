#!/usr/bin/env python3
"""
AI 서버 (Flask + MQTT)
MQTT 연결 문제 해결
"""

from flask import Flask, request, jsonify
import cv2
import numpy as np
import base64
import paho.mqtt.client as mqtt
import json
import time
import threading
from pose_analyzer import PoseAnalyzer
from ai_config import AIServerConfig

app = Flask(__name__)
analyzer = PoseAnalyzer()

# MQTT 클라이언트 전역 변수
mqtt_client = None
mqtt_connected = False
mqtt_lock = threading.Lock()

# ========================================
# MQTT 설정 및 초기화
# ========================================

def init_mqtt():
    """MQTT 클라이언트 초기화"""
    global mqtt_client, mqtt_connected
    
    mqtt_client = mqtt.Client(client_id="ai_server_mqtt", clean_session=True)
    
    def on_connect(client, userdata, flags, rc):
        global mqtt_connected
        if rc == 0:
            print("✓ MQTT 브로커 연결 성공")
            with mqtt_lock:
                mqtt_connected = True
        else:
            print(f"✗ MQTT 연결 실패 (rc={rc})")
            with mqtt_lock:
                mqtt_connected = False
    
    def on_disconnect(client, userdata, rc):
        global mqtt_connected
        print(f"⚠️ MQTT 연결 끊김 (rc={rc})")
        with mqtt_lock:
            mqtt_connected = False
    
    mqtt_client.on_connect = on_connect
    mqtt_client.on_disconnect = on_disconnect
    
    try:
        mqtt_broker = "10.10.16.111"
        mqtt_port = 1883
        
        print(f"🔄 MQTT 브로커 연결 시도: {mqtt_broker}:{mqtt_port}")
        mqtt_client.connect(mqtt_broker, mqtt_port, 60)
        mqtt_client.loop_start()
        
        # 연결 완료 대기 (최대 5초)
        for i in range(50):
            with mqtt_lock:
                if mqtt_connected:
                    print(f"✓ MQTT 연결 완료 ({i*0.1:.1f}초)")
                    return True
            time.sleep(0.1)
        
        print(f"⚠️ MQTT 연결 대기 시간 초과 (5초)")
        return False
            
    except Exception as e:
        print(f"✗ MQTT 초기화 실패: {e}")
        return False

def is_mqtt_connected():
    """MQTT 연결 상태 확인"""
    with mqtt_lock:
        return mqtt_connected

def publish_mqtt(topic, data, retry=3):
    """MQTT 메시지 발행 (재시도 포함)"""
    if mqtt_client is None:
        print("✗ MQTT 클라이언트가 초기화되지 않음")
        return False
    
    # 연결 상태 확인
    if not is_mqtt_connected():
        print(f"⚠️ MQTT 연결 안 됨 - 재연결 시도...")
        # 재연결 시도
        try:
            mqtt_client.reconnect()
            time.sleep(0.5)
        except:
            pass
        
        if not is_mqtt_connected():
            print(f"✗ MQTT 재연결 실패")
            return False
    
    # 메시지 전송 재시도
    for attempt in range(retry):
        try:
            payload = json.dumps(data, ensure_ascii=False)
            
            result = mqtt_client.publish(topic, payload, qos=1)  # QoS 1로 변경
            
            # 발행 완료 대기
            result.wait_for_publish(timeout=2.0)
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"✓ MQTT 전송 성공: {topic}")
                return True
            else:
                print(f"✗ MQTT 전송 실패 (rc={result.rc}, attempt={attempt+1}/{retry})")
                time.sleep(0.2)
                
        except Exception as e:
            print(f"✗ MQTT 전송 오류 (attempt={attempt+1}/{retry}): {e}")
            time.sleep(0.2)
    
    return False

# ========================================
# Flask API 엔드포인트
# ========================================

@app.route('/api/mode/select', methods=['POST'])
def select_mode():
    """운동 모드 선택 API (HTTP + MQTT 응답)"""
    try:
        data = request.get_json()

        if not data or 'mode' not in data:
            return jsonify({
                'status': 'error',
                'message': 'Missing mode parameter'
            }), 400

        mode = data['mode']

        if mode not in AIServerConfig.SUPPORTED_MODES:
            return jsonify({
                'status': 'error',
                'message': f'Unsupported mode: {mode}',
                'supported_modes': AIServerConfig.SUPPORTED_MODES
            }), 400

        # 모드 설정
        analyzer.set_mode(mode)

        # 포즈 시퀀스 정보
        poses = AIServerConfig.MODE_POSES.get(mode, [])

        print(f"✓ 운동 모드 선택됨: {mode} (포즈 개수: {len(poses)})")

        # HTTP 응답 데이터
        response_data = {
            'status': 'success',
            'message': f'{mode.upper()} mode selected',
            'mode': mode,
            'poses': poses,
            'total_poses': len(poses)
        }

        # MQTT 응답 데이터
        mqtt_response = {
            'status': 'success',
            'mode': mode,
            'poses': poses,
            'timestamp': int(time.time() * 1000)
        }
        
        # MQTT 상태 확인
        print(f"📡 MQTT 연결 상태: {is_mqtt_connected()}")
        
        # MQTT 전송
        mqtt_sent = publish_mqtt('qt/response/mode_selected', mqtt_response)
        
        if mqtt_sent:
            print(f"✓ MQTT 응답 전송 완료")
            print(f"   Topic: qt/response/mode_selected")
            print(f"   Mode: {mode}, Poses: {len(poses)}")
        else:
            print(f"✗ MQTT 응답 전송 실패 (HTTP는 정상)")

        return jsonify(response_data), 200

    except Exception as e:
        print(f"✗ 모드 선택 오류: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500


@app.route('/api/stream/frame', methods=['POST'])
def analyze_frame():
    """프레임 분석 API"""
    try:
        data = request.get_json()

        if not data or 'frame' not in data:
            return jsonify({
                'status': 'error',
                'message': 'Missing frame data'
            }), 400

        # pose_index 설정
        pose_index = data.get('pose_index', 0)
        if not analyzer.set_pose_index(pose_index):
            return jsonify({
                'status': 'error',
                'message': f'Invalid pose_index: {pose_index}'
            }), 400

        # Base64 디코딩
        try:
            frame_base64 = data['frame']
            frame_bytes = base64.b64decode(frame_base64)
            frame_array = np.frombuffer(frame_bytes, dtype=np.uint8)
            frame = cv2.imdecode(frame_array, cv2.IMREAD_COLOR)

            if frame is None:
                return jsonify({
                    'status': 'error',
                    'message': 'Failed to decode frame'
                }), 400

        except Exception as e:
            return jsonify({
                'status': 'error',
                'message': f'Frame decoding error: {str(e)}'
            }), 400

        # YOLO 추론
        results = analyzer.model(frame, verbose=False)
        
        # 프레임 분석
        result = analyzer.analyze_frame(frame)

        # 키포인트 정보 추가
        if results[0].keypoints is not None and len(results[0].keypoints) > 0:
            keypoints = results[0].keypoints[0]
            xy = keypoints.xy.cpu().numpy()[0].tolist()
            conf = keypoints.conf.cpu().numpy()[0].tolist()
            
            result['keypoints'] = {
                'xy': xy,
                'conf': conf
            }

        # 타임스탬프
        if 'timestamp' in data:
            result['timestamp'] = data['timestamp']

        return jsonify(result), 200

    except Exception as e:
        print(f"✗ 프레임 분석 오류: {e}")
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500


@app.route('/api/stream/stop', methods=['POST'])
def stop_stream():
    """스트리밍 중단 API"""
    try:
        data = request.get_json() or {}
        mode = data.get('mode', analyzer.current_mode)
        print(f"✓ 스트리밍 중단됨: {mode}")
        return jsonify({
            'status': 'success',
            'message': 'Stream stopped',
            'mode': mode
        }), 200
    except Exception as e:
        print(f"✗ 스트리밍 중단 오류: {e}")
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500


@app.route('/api/status', methods=['GET'])
def get_status():
    """서버 상태 확인 API"""
    return jsonify({
        'status': 'running',
        'current_mode': analyzer.current_mode,
        'supported_modes': AIServerConfig.SUPPORTED_MODES,
        'mqtt_connected': is_mqtt_connected()
    }), 200


@app.route('/api/health', methods=['GET'])
def health_check():
    """헬스 체크 API"""
    return jsonify({'status': 'healthy'}), 200


def main():
    """메인 진입점"""
    print("=" * 70)
    print("  AI 서버 (YOLO Pose 분석 + MQTT)")
    print("=" * 70)
    print(f"Flask: http://{AIServerConfig.HOST}:{AIServerConfig.PORT}")
    print(f"모델: {AIServerConfig.MODEL_PATH}")
    print(f"지원 모드: {len(AIServerConfig.SUPPORTED_MODES)}개")
    print("=" * 70)
    
    # MQTT 초기화
    print("\n🔄 MQTT 초기화 중...")
    mqtt_ok = init_mqtt()
    
    if mqtt_ok:
        print("✓ MQTT: 활성화")
    else:
        print("⚠️ MQTT: 비활성화 (HTTP만 사용)")
    
    print("=" * 70)
    print("\nAPI 엔드포인트:")
    print("  POST /api/mode/select   - 운동 모드 선택")
    print("  POST /api/stream/frame  - 프레임 분석")
    print("  POST /api/stream/stop   - 스트리밍 중단")
    print("  GET  /api/status        - 서버 상태")
    print("  GET  /api/health        - 헬스 체크")
    print("=" * 70)
    print("\n✓ 서버 시작!\n")

    app.run(
        host=AIServerConfig.HOST,
        port=AIServerConfig.PORT,
        debug=AIServerConfig.DEBUG,
        threaded=True
    )


if __name__ == "__main__":
    main()