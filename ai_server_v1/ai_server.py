#!/usr/bin/env python3
"""
AI 서버 (Flask)
WatchTower로부터 운동 모드 선택 및 영상을 수신하여 실시간 자세 분석 제공
카테고리별 루틴 지원
"""

from flask import Flask, request, jsonify
import cv2
import numpy as np
import base64
from pose_analyzer import PoseAnalyzer
from ai_config import AIServerConfig


app = Flask(__name__)
analyzer = PoseAnalyzer()


@app.route('/api/mode/select', methods=['POST'])
def select_mode():
    """
    운동 모드 선택 API (단일 모드)

    Request:
        {
            "mode": "squat"  # 'squat', 'pushup' 등
        }

    Response:
        {
            "status": "success",
            "message": "SQUAT mode selected",
            "mode": "squat"
        }
    """
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

        # 해당 모드의 포즈 시퀀스 정보 가져오기
        poses = AIServerConfig.MODE_POSES.get(mode, [])

        print(f"✓ 운동 모드 선택됨: {mode} (포즈 개수: {len(poses)})")

        return jsonify({
            'status': 'success',
            'message': f'{mode.upper()} mode selected',
            'mode': mode,
            'poses': poses,
            'total_poses': len(poses)
        }), 200

    except Exception as e:
        print(f"✗ 모드 선택 오류: {e}")
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500


@app.route('/api/stream/frame', methods=['POST'])
def analyze_frame():
    """
    프레임 분석 API (스트리밍 중 반복 호출)
    """
    try:
        data = request.get_json()

        if not data or 'frame' not in data:
            return jsonify({
                'status': 'error',
                'message': 'Missing frame data'
            }), 400

        # pose_index 설정 (옵션, 기본값 0)
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

        # 프레임 분석
        result = analyzer.analyze_frame(frame)

        # 분석 로그 출력
        pose_name = result.get('current_pose', 'unknown')
        is_correct = result.get('is_correct')
        score = result.get('score', '-')
        feedback = result.get('feedback', result.get('message', ''))
        status = 'CORRECT' if is_correct else 'CHECK'
        if result.get('status') == 'error':
            status = 'ERROR'
        print(f"[Analysis] mode={analyzer.current_mode} pose_index={pose_index} pose={pose_name} "
              f"status={status} score={score} feedback={feedback}")

        # 타임스탬프 추가 (있는 경우)
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
    """
    스트리밍 중단 API

    Request:
        {
            "mode": "squat"  # optional (현재 모드)
        }

    Response:
        {
            "status": "success",
            "message": "Stream stopped"
        }
    """
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
    """
    서버 상태 확인 API

    Response:
        {
            "status": "running",
            "current_mode": "squat" or null,
            "supported_modes": [...]
        }
    """
    return jsonify({
        'status': 'running',
        'current_mode': analyzer.current_mode,
        'supported_modes': AIServerConfig.SUPPORTED_MODES
    }), 200


@app.route('/api/health', methods=['GET'])
def health_check():
    """헬스 체크 API"""
    return jsonify({'status': 'healthy'}), 200


def main():
    """메인 진입점"""
    print("=" * 50)
    print("  AI 서버 (YOLO Pose 분석)")
    print("=" * 50)
    print(f"주소: http://{AIServerConfig.HOST}:{AIServerConfig.PORT}")
    print(f"지원 모드: {len(AIServerConfig.SUPPORTED_MODES)}개")
    print(f"모델: {AIServerConfig.MODEL_PATH}")
    print("=" * 50)
    print("\nAPI 엔드포인트:")
    print("  [운동 모드]")
    print("  POST /api/mode/select      - 운동 모드 선택")
    print("  POST /api/stream/frame     - 프레임 분석")
    print("  POST /api/stream/stop      - 스트리밍 중단")
    print("\n  [기타]")
    print("  GET  /api/status           - 서버 상태 확인")
    print("  GET  /api/health           - 헬스 체크")
    print("=" * 50)

    app.run(
        host=AIServerConfig.HOST,
        port=AIServerConfig.PORT,
        debug=AIServerConfig.DEBUG
    )


if __name__ == "__main__":
    main()
