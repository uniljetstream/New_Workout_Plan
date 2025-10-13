"""
운동 자세 교정 프로그램 - GUI 버전
엣지 디바이스에서 전송받은 영상 분석
"""

import cv2
import numpy as np
from ultralytics import YOLO
import math
import os
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from datetime import datetime
from pathlib import Path
import threading


class ExercisePoseChecker:
    def __init__(self, exercise_type='squat'):
        self.exercise_type = exercise_type
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
        """세 점으로 각도 계산"""
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
        left_hip = self.get_keypoint(keypoints, 'left_hip')
        left_knee = self.get_keypoint(keypoints, 'left_knee')
        left_ankle = self.get_keypoint(keypoints, 'left_ankle')
        
        if all([left_hip, left_knee, left_ankle]):
            knee_angle = self.calculate_angle(left_hip, left_knee, left_ankle)
            
            if knee_angle > 160:
                self.stage = "up"
            if knee_angle < 90 and self.stage == 'up':
                self.stage = "down"
                self.rep_count += 1
            
            if knee_angle < 90:
                feedback = "Good depth!"
                color = (0, 255, 0)
            elif 90 <= knee_angle < 120:
                feedback = "Go deeper"
                color = (0, 255, 255)
            else:
                feedback = "Stand position"
                color = (255, 255, 255)
                
            return knee_angle, feedback, color
        
        return None, "Keypoints not detected", (0, 0, 255)
    
    def check_pushup(self, keypoints):
        """푸시업 자세 체크"""
        left_shoulder = self.get_keypoint(keypoints, 'left_shoulder')
        left_elbow = self.get_keypoint(keypoints, 'left_elbow')
        left_wrist = self.get_keypoint(keypoints, 'left_wrist')
        
        if all([left_shoulder, left_elbow, left_wrist]):
            elbow_angle = self.calculate_angle(left_shoulder, left_elbow, left_wrist)
            
            if elbow_angle > 160:
                self.stage = "up"
            if elbow_angle < 90 and self.stage == 'up':
                self.stage = "down"
                self.rep_count += 1
            
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
        left_shoulder = self.get_keypoint(keypoints, 'left_shoulder')
        left_hip = self.get_keypoint(keypoints, 'left_hip')
        left_ankle = self.get_keypoint(keypoints, 'left_ankle')
        
        if all([left_shoulder, left_hip, left_ankle]):
            back_angle = self.calculate_angle(left_shoulder, left_hip, left_ankle)
            
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
        left_hip = self.get_keypoint(keypoints, 'left_hip')
        left_knee = self.get_keypoint(keypoints, 'left_knee')
        left_ankle = self.get_keypoint(keypoints, 'left_ankle')
        
        if all([left_hip, left_knee, left_ankle]):
            knee_angle = self.calculate_angle(left_hip, left_knee, left_ankle)
            
            if knee_angle > 160:
                self.stage = "up"
            if knee_angle < 100 and self.stage == 'up':
                self.stage = "down"
                self.rep_count += 1
            
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
        results = self.model(frame, verbose=False)
        
        if len(results[0].keypoints) > 0:
            keypoints = results[0].keypoints.xy[0].cpu().numpy()
            
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
            
            annotated_frame = results[0].plot()
            
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
            cv2.putText(frame, 'No person detected', 
                       (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
            return frame


class ExerciseGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("운동 자세 교정 프로그램")
        self.root.geometry("600x500")
        
        # 기본 경로 설정
        self.base_dir = Path.cwd()
        self.input_dir = self.base_dir / "input_videos"
        self.output_dir = self.base_dir / "output_videos"
        
        # 폴더 생성
        self.input_dir.mkdir(exist_ok=True)
        self.output_dir.mkdir(exist_ok=True)
        
        # 처리 상태
        self.is_processing = False
        self.checker = None
        
        self.setup_gui()
        
    def setup_gui(self):
        """GUI 구성"""
        # 타이틀
        title_frame = tk.Frame(self.root, bg='#2c3e50', height=60)
        title_frame.pack(fill='x')
        title_frame.pack_propagate(False)
        
        title_label = tk.Label(
            title_frame, 
            text="🏋️ 운동 자세 교정 시스템", 
            font=('Arial', 20, 'bold'),
            bg='#2c3e50',
            fg='white'
        )
        title_label.pack(pady=15)
        
        # 메인 프레임
        main_frame = tk.Frame(self.root, padx=20, pady=20)
        main_frame.pack(fill='both', expand=True)
        
        # 운동 선택
        exercise_frame = tk.LabelFrame(main_frame, text="운동 선택", font=('Arial', 12, 'bold'), padx=10, pady=10)
        exercise_frame.pack(fill='x', pady=(0, 15))
        
        self.exercise_var = tk.StringVar(value='squat')
        exercises = [
            ('스쿼트 (Squat)', 'squat'),
            ('푸시업 (Pushup)', 'pushup'),
            ('플랭크 (Plank)', 'plank'),
            ('런지 (Lunge)', 'lunge')
        ]
        
        for i, (text, value) in enumerate(exercises):
            rb = tk.Radiobutton(
                exercise_frame,
                text=text,
                variable=self.exercise_var,
                value=value,
                font=('Arial', 11)
            )
            rb.grid(row=i//2, column=i%2, sticky='w', padx=10, pady=5)
        
        # 경로 설정
        path_frame = tk.LabelFrame(main_frame, text="폴더 경로", font=('Arial', 12, 'bold'), padx=10, pady=10)
        path_frame.pack(fill='x', pady=(0, 15))
        
        # 입력 폴더
        tk.Label(path_frame, text="입력 영상 폴더:", font=('Arial', 10)).grid(row=0, column=0, sticky='w', pady=5)
        self.input_path_label = tk.Label(path_frame, text=str(self.input_dir), font=('Arial', 9), fg='blue')
        self.input_path_label.grid(row=0, column=1, sticky='w', padx=10)
        tk.Button(path_frame, text="변경", command=self.change_input_dir, width=8).grid(row=0, column=2, padx=5)
        
        # 출력 폴더
        tk.Label(path_frame, text="결과 저장 폴더:", font=('Arial', 10)).grid(row=1, column=0, sticky='w', pady=5)
        self.output_path_label = tk.Label(path_frame, text=str(self.output_dir), font=('Arial', 9), fg='blue')
        self.output_path_label.grid(row=1, column=1, sticky='w', padx=10)
        tk.Button(path_frame, text="변경", command=self.change_output_dir, width=8).grid(row=1, column=2, padx=5)
        
        # 영상 선택
        video_frame = tk.LabelFrame(main_frame, text="영상 선택", font=('Arial', 12, 'bold'), padx=10, pady=10)
        video_frame.pack(fill='both', expand=True, pady=(0, 15))
        
        # 리스트박스
        listbox_frame = tk.Frame(video_frame)
        listbox_frame.pack(fill='both', expand=True)
        
        scrollbar = tk.Scrollbar(listbox_frame)
        scrollbar.pack(side='right', fill='y')
        
        self.video_listbox = tk.Listbox(
            listbox_frame,
            font=('Arial', 10),
            yscrollcommand=scrollbar.set,
            height=8
        )
        self.video_listbox.pack(side='left', fill='both', expand=True)
        scrollbar.config(command=self.video_listbox.yview)
        
        # 새로고침 버튼
        tk.Button(video_frame, text="📂 영상 목록 새로고침", command=self.refresh_video_list, 
                 font=('Arial', 10)).pack(pady=(10, 0))
        
        # 제어 버튼
        button_frame = tk.Frame(main_frame)
        button_frame.pack(fill='x')
        
        self.start_button = tk.Button(
            button_frame,
            text="▶ 자세 분석 시작",
            command=self.start_processing,
            font=('Arial', 12, 'bold'),
            bg='#27ae60',
            fg='white',
            height=2
        )
        self.start_button.pack(side='left', fill='x', expand=True, padx=(0, 5))
        
        self.stop_button = tk.Button(
            button_frame,
            text="⏹ 중지",
            command=self.stop_processing,
            font=('Arial', 12, 'bold'),
            bg='#e74c3c',
            fg='white',
            height=2,
            state='disabled'
        )
        self.stop_button.pack(side='right', fill='x', expand=True, padx=(5, 0))
        
        # 상태 표시
        self.status_label = tk.Label(
            main_frame,
            text="대기 중...",
            font=('Arial', 10),
            fg='gray'
        )
        self.status_label.pack(pady=(10, 0))
        
        # 초기 영상 목록 로드
        self.refresh_video_list()
    
    def change_input_dir(self):
        """입력 폴더 변경"""
        directory = filedialog.askdirectory(title="입력 영상 폴더 선택")
        if directory:
            self.input_dir = Path(directory)
            self.input_path_label.config(text=str(self.input_dir))
            self.refresh_video_list()
    
    def change_output_dir(self):
        """출력 폴더 변경"""
        directory = filedialog.askdirectory(title="결과 저장 폴더 선택")
        if directory:
            self.output_dir = Path(directory)
            self.output_path_label.config(text=str(self.output_dir))
    
    def refresh_video_list(self):
        """영상 목록 새로고침"""
        self.video_listbox.delete(0, tk.END)
        
        video_extensions = ['.mp4', '.avi', '.mov', '.mkv', '.wmv']
        videos = []
        
        for ext in video_extensions:
            videos.extend(self.input_dir.glob(f'*{ext}'))
        
        if videos:
            for video in sorted(videos):
                self.video_listbox.insert(tk.END, video.name)
            self.status_label.config(text=f"{len(videos)}개의 영상 발견", fg='green')
        else:
            self.status_label.config(text="입력 폴더에 영상이 없습니다", fg='orange')
    
    def start_processing(self):
        """자세 분석 시작"""
        # 선택된 영상 확인
        selection = self.video_listbox.curselection()
        if not selection:
            messagebox.showwarning("경고", "분석할 영상을 선택해주세요!")
            return
        
        video_name = self.video_listbox.get(selection[0])
        video_path = self.input_dir / video_name
        
        # 버튼 상태 변경
        self.start_button.config(state='disabled')
        self.stop_button.config(state='normal')
        self.is_processing = True
        
        # 상태 업데이트
        self.status_label.config(text=f"분석 중: {video_name}", fg='blue')
        
        # 별도 스레드에서 처리
        thread = threading.Thread(
            target=self.process_video,
            args=(video_path,),
            daemon=True
        )
        thread.start()
    
    def stop_processing(self):
        """자세 분석 중지"""
        self.is_processing = False
        self.start_button.config(state='normal')
        self.stop_button.config(state='disabled')
        self.status_label.config(text="중지됨", fg='red')
    
    def process_video(self, video_path):
        """영상 처리"""
        try:
            # 운동 타입 선택
            exercise_type = self.exercise_var.get()
            self.checker = ExercisePoseChecker(exercise_type=exercise_type)
            
            # 입력 영상 열기
            cap = cv2.VideoCapture(str(video_path))
            
            if not cap.isOpened():
                messagebox.showerror("오류", "영상을 열 수 없습니다!")
                self.stop_processing()
                return
            
            # 출력 파일 설정
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output_name = f"{exercise_type}_{timestamp}_analyzed.mp4"
            output_path = self.output_dir / output_name
            
            # 비디오 설정
            w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            fps = int(cap.get(cv2.CAP_PROP_FPS))
            
            fourcc = cv2.VideoWriter_fourcc(*'mp4v')
            out = cv2.VideoWriter(str(output_path), fourcc, fps, (w, h))
            
            frame_count = 0
            total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
            
            while cap.isOpened() and self.is_processing:
                ret, frame = cap.read()
                if not ret:
                    break
                
                frame_count += 1
                
                # 프레임 처리
                processed_frame = self.checker.process_frame(frame)
                
                # 결과 저장
                out.write(processed_frame)
                
                # 진행률 업데이트
                progress = (frame_count / total_frames) * 100
                self.root.after(0, self.update_progress, progress, frame_count, total_frames)
                
                # 화면에 표시 (선택적)
                cv2.imshow('Processing', processed_frame)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            
            # 정리
            cap.release()
            out.release()
            cv2.destroyAllWindows()
            
            # 완료 메시지
            if self.is_processing:
                result_msg = f"분석 완료!\n\n운동: {exercise_type.upper()}\n총 횟수: {self.checker.rep_count}\n저장 위치: {output_path}"
                self.root.after(0, lambda: messagebox.showinfo("완료", result_msg))
                self.root.after(0, lambda: self.status_label.config(text="완료!", fg='green'))
            
            # 버튼 상태 복구
            self.root.after(0, lambda: self.start_button.config(state='normal'))
            self.root.after(0, lambda: self.stop_button.config(state='disabled'))
            self.is_processing = False
            
        except Exception as e:
            error_msg = f"오류 발생: {str(e)}"
            self.root.after(0, lambda: messagebox.showerror("오류", error_msg))
            self.root.after(0, self.stop_processing)
    
    def update_progress(self, progress, current, total):
        """진행률 업데이트"""
        self.status_label.config(
            text=f"처리 중... {progress:.1f}% ({current}/{total} 프레임)",
            fg='blue'
        )


def main():
    root = tk.Tk()
    app = ExerciseGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()