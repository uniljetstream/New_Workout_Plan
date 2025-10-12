"""
운동 자세 교정 프로그램 - YOLOv11 Pose Estimation
지원 운동: 플랭크, 런지, 스쿼트, 푸시업
"""

import cv2
import numpy as np
from ultralytics import YOLO
import math

class ExercisePoseChecker:
    def __init__(self, exercise_type='squat'):
        """
        Args:
            exercise_type: 'plank', 'lunge', 'squat', 'pushup' 중 선택
        """
        self.exercise_type = exercise_type
        
        # YOLOv11 pose 모델 로드 (사전 학습된 COCO 모델 사용)
        # 나중에 자신의 학습된 모델로 교체 가능: f'{exercise_type}_best.pt'
        self.model = YOLO('yolo11n-pose.pt')
        
        # COCO 키포인트 인덱스
        self.KEYPOINT_DICT = {
            'nose': 0, 'left_eye': 1, 'right_eye': 2,
            'left_ear': 3, 'right_ear': 4,
            'left_shoulder': 5, 'right_shoulder': 6,
            'left_elbow': 7, 'right_elbow': 8,
            'left_wrist': 9, 'right_wrist': 10,
            'left_hip': 11, 'right_hip': 12,
            'left_knee': 13, 'right_knee': 14,
            'left_ankle': 15, 'right_ankle': 16
        }
        
        # 운동별 카운터 및 상태
        self.rep_count = 0
        self.stage = None
        
    def calculate_angle(self, a, b, c):
        """세 점으로 각도 계산 (a-b-c에서 b가 꼭짓점)"""
        a = np.array(a)
        b = np.array(b)
        c = np.array(c)
        
        radians = np.arctan2(c[1]-b[1], c[0]-b[0]) - np.arctan2(a[1]-b[1], a[0]-b[0])
        angle = np.abs(radians * 180.0 / np.pi)
        
        if angle > 180.0:
            angle = 360 - angle
            
        return angle
    
    def get_keypoint(self, keypoints, name):
        """키포인트 좌표 가져오기"""
        idx = self.KEYPOINT_DICT[name]
        if idx < len(keypoints):
            x, y = keypoints[idx]
            return [float(x), float(y)]
        return None
    
    def check_squat(self, keypoints):
        """스쿼트 자세 체크"""
        # 무릎 각도 계산 (엉덩이-무릎-발목)
        left_hip = self.get_keypoint(keypoints, 'left_hip')
        left_knee = self.get_keypoint(keypoints, 'left_knee')
        left_ankle = self.get_keypoint(keypoints, 'left_ankle')
        
        if all([left_hip, left_knee, left_ankle]):
            knee_angle = self.calculate_angle(left_hip, left_knee, left_ankle)
            
            # 스쿼트 카운팅
            if knee_angle > 160:
                self.stage = "up"
            if knee_angle < 90 and self.stage == 'up':
                self.stage = "down"
                self.rep_count += 1
            
            # 자세 판단
            if knee_angle < 90:
                feedback = "Good depth!"
                color = (0, 255, 0)  # 녹색
            elif 90 <= knee_angle < 120:
                feedback = "Go deeper"
                color = (0, 255, 255)  # 노란색
            else:
                feedback = "Stand position"
                color = (255, 255, 255)  # 흰색
                
            return knee_angle, feedback, color
        
        return None, "Keypoints not detected", (0, 0, 255)
    
    def check_pushup(self, keypoints):
        """푸시업 자세 체크"""
        # 팔꿈치 각도 계산 (어깨-팔꿈치-손목)
        left_shoulder = self.get_keypoint(keypoints, 'left_shoulder')
        left_elbow = self.get_keypoint(keypoints, 'left_elbow')
        left_wrist = self.get_keypoint(keypoints, 'left_wrist')
        
        if all([left_shoulder, left_elbow, left_wrist]):
            elbow_angle = self.calculate_angle(left_shoulder, left_elbow, left_wrist)
            
            # 푸시업 카운팅
            if elbow_angle > 160:
                self.stage = "up"
            if elbow_angle < 90 and self.stage == 'up':
                self.stage = "down"
                self.rep_count += 1
            
            # 자세 판단
            if elbow_angle < 90:
                feedback = "Good form!"
                color = (0, 255, 0)
            elif 90 <= elbow_angle < 120:
                feedback = "Go lower"
                color = (0, 255, 255)
            else:
                feedback = "Up position"
                color = (255, 255, 255)
                
            return elbow_angle, feedback, color
        
        return None, "Keypoints not detected", (0, 0, 255)
    
    def check_plank(self, keypoints):
        """플랭크 자세 체크"""
        # 등 직선도 체크 (어깨-엉덩이-발목)
        left_shoulder = self.get_keypoint(keypoints, 'left_shoulder')
        left_hip = self.get_keypoint(keypoints, 'left_hip')
        left_ankle = self.get_keypoint(keypoints, 'left_ankle')
        
        if all([left_shoulder, left_hip, left_ankle]):
            back_angle = self.calculate_angle(left_shoulder, left_hip, left_ankle)
            
            # 플랭크 자세 판단 (각도가 170-190이면 직선)
            if 170 <= back_angle <= 190:
                feedback = "Perfect plank!"
                color = (0, 255, 0)
            elif back_angle < 170:
                feedback = "Hips too high"
                color = (0, 165, 255)
            else:
                feedback = "Hips too low"
                color = (0, 165, 255)
                
            return back_angle, feedback, color
        
        return None, "Keypoints not detected", (0, 0, 255)
    
    def check_lunge(self, keypoints):
        """런지 자세 체크"""
        # 앞쪽 무릎 각도 계산
        left_hip = self.get_keypoint(keypoints, 'left_hip')
        left_knee = self.get_keypoint(keypoints, 'left_knee')
        left_ankle = self.get_keypoint(keypoints, 'left_ankle')
        
        if all([left_hip, left_knee, left_ankle]):
            knee_angle = self.calculate_angle(left_hip, left_knee, left_ankle)
            
            # 런지 카운팅
            if knee_angle > 160:
                self.stage = "up"
            if knee_angle < 100 and self.stage == 'up':
                self.stage = "down"
                self.rep_count += 1
            
            # 자세 판단
            if 80 <= knee_angle <= 100:
                feedback = "Perfect lunge!"
                color = (0, 255, 0)
            elif knee_angle < 80:
                feedback = "Too deep"
                color = (0, 165, 255)
            elif knee_angle > 100 and knee_angle < 160:
                feedback = "Go deeper"
                color = (0, 255, 255)
            else:
                feedback = "Stand position"
                color = (255, 255, 255)
                
            return knee_angle, feedback, color
        
        return None, "Keypoints not detected", (0, 0, 255)
    
    def process_frame(self, frame):
        """프레임 처리 및 자세 분석"""
        # YOLOv11 추론
        results = self.model(frame, verbose=False)
        
        if len(results[0].keypoints) > 0:
            # 키포인트 가져오기
            keypoints = results[0].keypoints.xy[0].cpu().numpy()
            
            # 운동별 자세 체크
            if self.exercise_type == 'squat':
                angle, feedback, color = self.check_squat(keypoints)
            elif self.exercise_type == 'pushup':
                angle, feedback, color = self.check_pushup(keypoints)
            elif self.exercise_type == 'plank':
                angle, feedback, color = self.check_plank(keypoints)
            elif self.exercise_type == 'lunge':
                angle, feedback, color = self.check_lunge(keypoints)
            else:
                angle, feedback, color = None, "Unknown exercise", (0, 0, 255)
            
            # 키포인트 그리기
            annotated_frame = results[0].plot()
            
            # 정보 표시
            cv2.putText(annotated_frame, f'Exercise: {self.exercise_type.upper()}', 
                       (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
            cv2.putText(annotated_frame, f'Reps: {self.rep_count}', 
                       (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
            
            if angle:
                cv2.putText(annotated_frame, f'Angle: {int(angle)}', 
                           (10, 110), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
            
            cv2.putText(annotated_frame, feedback, 
                       (10, 150), cv2.FONT_HERSHEY_SIMPLEX, 1.2, color, 3)
            
            return annotated_frame
        else:
            # 사람이 감지되지 않음
            cv2.putText(frame, 'No person detected', 
                       (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
            return frame
    
    def run(self, source=0):
        """
        프로그램 실행
        Args:
            source: 0 (웹캠) 또는 비디오 파일 경로
        """
        cap = cv2.VideoCapture(source)
        
        print(f"=== Exercise Pose Checker Started ===")
        print(f"Exercise Type: {self.exercise_type.upper()}")
        print(f"Press 'q' to quit")
        print(f"Press 'r' to reset counter")
        print(f"Press '1'~'4' to change exercise")
        print(f"  1: Squat, 2: Pushup, 3: Plank, 4: Lunge")
        
        while cap.isOpened():
            ret, frame = cap.read()
            if not ret:
                break
            
            # 프레임 처리
            processed_frame = self.process_frame(frame)
            
            # 화면 표시
            cv2.imshow('Exercise Pose Checker', processed_frame)
            
            # 키 입력 처리
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('r'):
                self.rep_count = 0
                self.stage = None
                print("Counter reset!")
            elif key == ord('1'):
                self.exercise_type = 'squat'
                self.rep_count = 0
                self.stage = None
                print("Changed to: SQUAT")
            elif key == ord('2'):
                self.exercise_type = 'pushup'
                self.rep_count = 0
                self.stage = None
                print("Changed to: PUSHUP")
            elif key == ord('3'):
                self.exercise_type = 'plank'
                self.rep_count = 0
                self.stage = None
                print("Changed to: PLANK")
            elif key == ord('4'):
                self.exercise_type = 'lunge'
                self.rep_count = 0
                self.stage = None
                print("Changed to: LUNGE")
        
        cap.release()
        cv2.destroyAllWindows()
        print(f"\n=== Final Results ===")
        print(f"Exercise: {self.exercise_type.upper()}")
        print(f"Total Reps: {self.rep_count}")


if __name__ == "__main__":
    # 운동 선택 ('squat', 'pushup', 'plank', 'lunge')
    checker = ExercisePoseChecker(exercise_type='squat')
    
    # 웹캠으로 실행 (0) 또는 비디오 파일 경로 입력
    checker.run(source=0)