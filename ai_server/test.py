#!/usr/bin/env python3
"""
AI 서버 테스트 클라이언트
웹캠으로 실시간 자세 분석 테스트
"""

import cv2
import requests
import base64
import time
import json


class TestClient:
    def __init__(self, server_url="http://localhost:5000"):
        self.server_url = server_url
        self.current_mode = None
        self.current_pose_index = 0
        self.total_poses = 0
        
    def check_server(self):
        """서버 상태 확인"""
        try:
            response = requests.get(f"{self.server_url}/api/health", timeout=2)
            if response.status_code == 200:
                print("✓ 서버 연결 성공")
                return True
            else:
                print("✗ 서버 응답 오류")
                return False
        except Exception as e:
            print(f"✗ 서버 연결 실패: {e}")
            return False
    
    def select_mode(self, mode):
        """운동 모드 선택"""
        try:
            response = requests.post(
                f"{self.server_url}/api/mode/select",
                json={"mode": mode},
                timeout=5
            )
            
            if response.status_code == 200:
                data = response.json()
                self.current_mode = mode
                self.total_poses = data.get('total_poses', 0)
                self.current_pose_index = 0
                print(f"✓ 모드 선택: {mode} ({self.total_poses}개 포즈)")
                print(f"  포즈 목록:")
                for i, pose in enumerate(data.get('poses', [])):
                    print(f"    {i}. {pose['name']} - {pose['description']}")
                return True
            else:
                print(f"✗ 모드 선택 실패: {response.json()}")
                return False
                
        except Exception as e:
            print(f"✗ 모드 선택 오류: {e}")
            return False
    
    def analyze_frame(self, frame):
        """프레임 분석"""
        try:
            # 프레임을 base64로 인코딩
            _, buffer = cv2.imencode('.jpg', frame)
            frame_base64 = base64.b64encode(buffer).decode('utf-8')
            
            # 서버에 전송
            response = requests.post(
                f"{self.server_url}/api/stream/frame",
                json={
                    "frame": frame_base64,
                    "pose_index": self.current_pose_index,
                    "timestamp": int(time.time() * 1000)
                },
                timeout=5
            )
            
            if response.status_code == 200:
                return response.json()
            else:
                return None
                
        except Exception as e:
            print(f"✗ 분석 오류: {e}")
            return None
    
    def draw_skeleton(self, frame, result):
        """Draw keypoints and skeleton"""
        if not result or 'keypoints' not in result:
            return frame
        
        keypoints_data = result['keypoints']
        xy = keypoints_data['xy']
        conf = keypoints_data['conf']
        
        # YOLO Pose keypoint connections (COCO format)
        # [start_point, end_point]
        skeleton = [
            [15, 13], [13, 11], [16, 14], [14, 12], [11, 12],  # legs
            [5, 11], [6, 12], [5, 6],  # torso
            [5, 7], [6, 8], [7, 9], [8, 10],  # arms
            [1, 2], [0, 1], [0, 2], [1, 3], [2, 4], [3, 5], [4, 6]  # face
        ]
        
        # Draw skeleton lines
        for connection in skeleton:
            start_idx = connection[0]
            end_idx = connection[1]
            
            if start_idx < len(xy) and end_idx < len(xy):
                if conf[start_idx] > 0.5 and conf[end_idx] > 0.5:
                    start_point = (int(xy[start_idx][0]), int(xy[start_idx][1]))
                    end_point = (int(xy[end_idx][0]), int(xy[end_idx][1]))
                    cv2.line(frame, start_point, end_point, (0, 255, 0), 2)
        
        # Draw keypoints
        for i, (point, confidence) in enumerate(zip(xy, conf)):
            if confidence > 0.5:
                x, y = int(point[0]), int(point[1])
                # Different colors for different body parts
                if i < 5:  # face
                    color = (255, 0, 0)
                elif i < 11:  # arms
                    color = (0, 255, 255)
                else:  # legs
                    color = (255, 255, 0)
                
                cv2.circle(frame, (x, y), 4, color, -1)
                cv2.circle(frame, (x, y), 5, (255, 255, 255), 1)
        
        return frame
    
    def draw_result(self, frame, result):
        """Draw results on frame"""
        if not result:
            return frame
        
        height, width = frame.shape[:2]
        
        # Background box
        overlay = frame.copy()
        cv2.rectangle(overlay, (10, 10), (width - 10, 220), (0, 0, 0), -1)
        frame = cv2.addWeighted(overlay, 0.6, frame, 0.4, 0)
        
        # Draw skeleton first
        frame = self.draw_skeleton(frame, result)
        
        # Current pose info with view direction
        pose_desc = result.get('pose_description', '')
        # Extract view from description if available
        if 'Side view' in pose_desc:
            view_text = "VIEW: SIDE"
            view_color = (0, 255, 255)  # Yellow
        elif 'Front view' in pose_desc:
            view_text = "VIEW: FRONT"
            view_color = (255, 100, 255)  # Magenta
        else:
            view_text = ""
            view_color = (255, 255, 255)
        
        cv2.putText(frame, f"Pose: {pose_desc}", (20, 40),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        
        # View direction (large and prominent)
        if view_text:
            cv2.putText(frame, view_text, (width - 250, 50),
                       cv2.FONT_HERSHEY_SIMPLEX, 1.2, view_color, 3)
        
        # Score
        score = result.get('score', 0)
        is_correct = result.get('is_correct', False)
        color = (0, 255, 0) if is_correct else (0, 165, 255)
        cv2.putText(frame, f"Score: {score}%", (20, 90),
                   cv2.FONT_HERSHEY_SIMPLEX, 1.0, color, 2)
        
        # Feedback
        feedback = result.get('feedback', '')
        cv2.putText(frame, f"Feedback: {feedback}", (20, 130),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        
        # Progress
        cv2.putText(frame, f"Progress: {self.current_pose_index + 1}/{self.total_poses}", 
                   (20, 170), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        
        # Controls
        cv2.putText(frame, "[N]ext pose | [P]rev pose | [Q]uit", 
                   (20, height - 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        
        return frame
    
    def run(self, mode=None):
        """테스트 실행"""
        # 서버 체크
        if not self.check_server():
            print("\n서버를 먼저 실행해주세요: python ai_server.py")
            return
        
        # 모드 선택
        if mode is None:
            print("\n사용 가능한 모드:")
            modes = {
                '1': 'bodyweight_routine',
                '2': 'kettlebell_routine',
                '3': 'barbell_routine',
                '4': 'squat',
                '5': 'pushup',
                '6': 'plank',
                '7': 'lunge'
            }
            print("1. bodyweight_routine - 맨몸 운동 루틴")
            print("2. kettlebell_routine - 케틀벨 운동 루틴")
            print("3. barbell_routine - 바벨 운동 루틴")
            print("4. squat - 스쿼트")
            print("5. pushup - 푸시업")
            print("6. plank - 플랭크")
            print("7. lunge - 런지")
            
            choice = input("\n번호 또는 모드 이름을 입력하세요: ").strip()
            
            # 번호면 모드 이름으로 변환
            if choice in modes:
                mode = modes[choice]
            else:
                mode = choice
        
        if not self.select_mode(mode):
            return
        
        # 웹캠 시작
        cap = cv2.VideoCapture(0)
        if not cap.isOpened():
            print("✗ 카메라를 열 수 없습니다")
            return
        
        print("\n✓ 카메라 시작")
        print("=" * 50)
        print("조작 방법:")
        print("  N - 다음 포즈로 이동")
        print("  P - 이전 포즈로 이동")
        print("  Q - 종료")
        print("=" * 50)
        
        # FPS 계산용
        fps_time = time.time()
        fps = 0
        
        try:
            while True:
                ret, frame = cap.read()
                if not ret:
                    print("✗ 프레임을 읽을 수 없습니다")
                    break
                
                # 좌우 반전 (거울 모드)
                frame = cv2.flip(frame, 1)
                
                # 프레임 분석 (0.1초마다)
                current_time = time.time()
                if current_time - fps_time > 0.1:
                    result = self.analyze_frame(frame)
                    fps = 1.0 / (current_time - fps_time)
                    fps_time = current_time
                    
                    # 결과 그리기
                    if result:
                        frame = self.draw_result(frame, result)
                
                # FPS 표시
                cv2.putText(frame, f"FPS: {int(fps)}", (frame.shape[1] - 150, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                
                # 화면 표시
                cv2.imshow('Pose Analysis Test', frame)
                
                # 키 입력 처리
                key = cv2.waitKey(1) & 0xFF
                
                if key == ord('q') or key == ord('Q'):
                    print("\n종료합니다")
                    break
                elif key == ord('n') or key == ord('N'):
                    if self.current_pose_index < self.total_poses - 1:
                        self.current_pose_index += 1
                        print(f"다음 포즈로 이동: {self.current_pose_index + 1}/{self.total_poses}")
                    else:
                        print("마지막 포즈입니다")
                elif key == ord('p') or key == ord('P'):
                    if self.current_pose_index > 0:
                        self.current_pose_index -= 1
                        print(f"이전 포즈로 이동: {self.current_pose_index + 1}/{self.total_poses}")
                    else:
                        print("첫 번째 포즈입니다")
                        
        finally:
            cap.release()
            cv2.destroyAllWindows()
            print("✓ 테스트 종료")


def main():
    """메인 함수"""
    import sys
    
    print("=" * 50)
    print("  AI 서버 테스트 클라이언트")
    print("=" * 50)
    
    # 명령행 인자로 모드 지정 가능
    mode = sys.argv[1] if len(sys.argv) > 1 else None
    
    client = TestClient()
    client.run(mode)


if __name__ == "__main__":
    main()