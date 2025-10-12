from ultralytics import YOLO
import cv2
import numpy as np

def calculate_angle(p1, p2, p3):
    """
    3개 점으로 각도 계산 (p2가 중심점)
    반환값: 각도 (0-180도)
    """
    if p1 is None or p2 is None or p3 is None:
        return None

    # 벡터 계산
    v1 = np.array([p1[0] - p2[0], p1[1] - p2[1]])
    v2 = np.array([p3[0] - p2[0], p3[1] - p2[1]])

    # 각도 계산 (라디안 -> 도)
    cos_angle = np.dot(v1, v2) / (np.linalg.norm(v1) * np.linalg.norm(v2) + 1e-6)
    angle = np.arccos(np.clip(cos_angle, -1.0, 1.0))
    return np.degrees(angle)

def calculate_horizontal_angle(p1, p2):
    """
    두 점이 수평선과 이루는 각도 계산
    반환값: 각도 (0도 = 완전 수평)
    """
    if p1 is None or p2 is None:
        return None

    dx = p2[0] - p1[0]
    dy = p2[1] - p1[1]
    angle = np.degrees(np.arctan2(dy, dx))
    return abs(angle)  # 절대값으로 수평 기준

def check_t_pose(left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist):
    """
    T자 서기 자세 판별
    기준:
    1. 양쪽 팔이 거의 일직선 (어깨-팔꿈치-손목 각도 > 160도)
    2. 양팔이 수평 (어깨-손목 수평각도 < 20도)
    """
    points = [left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist]
    if any(p is None for p in points):
        return False, 0, "키포인트 감지 부족"

    # 왼팔 각도 (어깨-팔꿈치-손목)
    left_arm_angle = calculate_angle(left_shoulder, left_elbow, left_wrist)
    # 오른팔 각도
    right_arm_angle = calculate_angle(right_shoulder, right_elbow, right_wrist)

    # 팔의 수평 각도
    left_horizontal = calculate_horizontal_angle(left_shoulder, left_wrist)
    right_horizontal = calculate_horizontal_angle(right_shoulder, right_wrist)

    if None in [left_arm_angle, right_arm_angle, left_horizontal, right_horizontal]:
        return False, 0, "각도 계산 실패"

    # T자 판정 기준
    arm_straight_threshold = 160  # 팔이 펴진 정도
    horizontal_threshold = 20     # 수평 정도

    left_arm_ok = left_arm_angle > arm_straight_threshold
    right_arm_ok = right_arm_angle > arm_straight_threshold
    left_horizontal_ok = left_horizontal < horizontal_threshold
    right_horizontal_ok = right_horizontal < horizontal_threshold

    # 점수 계산 (0-100)
    score = 0
    if left_arm_ok:
        score += 25
    if right_arm_ok:
        score += 25
    if left_horizontal_ok:
        score += 25
    if right_horizontal_ok:
        score += 25

    # 피드백 메시지
    feedback = []
    if not left_arm_ok:
        feedback.append(f"왼팔 펴기 ({left_arm_angle:.1f}° < {arm_straight_threshold}°)")
    if not right_arm_ok:
        feedback.append(f"오른팔 펴기 ({right_arm_angle:.1f}° < {arm_straight_threshold}°)")
    if not left_horizontal_ok:
        feedback.append(f"왼팔 수평 ({left_horizontal:.1f}° > {horizontal_threshold}°)")
    if not right_horizontal_ok:
        feedback.append(f"오른팔 수평 ({right_horizontal:.1f}° > {horizontal_threshold}°)")

    is_correct = score == 100
    message = "완벽한 T자 자세!" if is_correct else ", ".join(feedback)

    return is_correct, score, message

# YOLO v11 Pose 모델 로드
model = YOLO('yolo11s-pose.pt')

# 웹캠 열기
cap = cv2.VideoCapture(0)

# 웹캠 체크
if not cap.isOpened():
    print("Error: 웹캠을 열 수 없습니다!")
    exit()

# 웹캠 해상도 확인
print(f"웹캠 해상도: {int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))}x{int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))}")

print("YOLO v11 Pose - 17개 키포인트 인덱스:")
print("0: 코, 1-2: 눈, 3-4: 귀")
print("5: 왼쪽 어깨, 6: 오른쪽 어깨")
print("7: 왼쪽 팔꿈치, 8: 오른쪽 팔꿈치")
print("9: 왼쪽 손목, 10: 오른쪽 손목")
print("11: 왼쪽 엉덩이, 12: 오른쪽 엉덩이")
print("13: 왼쪽 무릎, 14: 오른쪽 무릎")
print("15: 왼쪽 발목, 16: 오른쪽 발목")
print("\nT자 서기: 양팔을 수평으로 벌리세요!")
print("종료: 'q' 키를 누르세요\n")

# 창 미리 생성
cv2.namedWindow('YOLO Pose - T자 서기 테스트', cv2.WINDOW_NORMAL)

frame_count = 0
while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        print(f"Error: 프레임을 읽을 수 없습니다! (frame {frame_count})")
        break

    frame_count += 1
    if frame_count % 30 == 0:
        print(f"프레임 처리 중... {frame_count}")

    # YOLO로 포즈 감지
    results = model(frame, verbose=False)

    # 결과 시각화
    annotated_frame = results[0].plot()

    # 키포인트 데이터 추출
    if results[0].keypoints is not None and len(results[0].keypoints) > 0:
        keypoints = results[0].keypoints[0]  # 첫 번째 사람
        xy = keypoints.xy.cpu().numpy()[0]  # (17, 2) 배열
        conf = keypoints.conf.cpu().numpy()[0]  # (17,) 신뢰도

        # 주요 포인트 출력 (어깨-팔꿈치-손목)
        left_shoulder = xy[5] if conf[5] > 0.5 else None
        right_shoulder = xy[6] if conf[6] > 0.5 else None
        left_elbow = xy[7] if conf[7] > 0.5 else None
        right_elbow = xy[8] if conf[8] > 0.5 else None
        left_wrist = xy[9] if conf[9] > 0.5 else None
        right_wrist = xy[10] if conf[10] > 0.5 else None

        # T자 자세 판별
        is_correct, score, message = check_t_pose(
            left_shoulder, right_shoulder, left_elbow, right_elbow, left_wrist, right_wrist
        )

        # 점수에 따른 색상 (빨강 -> 노랑 -> 초록)
        if score >= 80:
            color = (0, 255, 0)  # 초록
        elif score >= 50:
            color = (0, 255, 255)  # 노랑
        else:
            color = (0, 0, 255)  # 빨강

        # 화면에 점수 표시
        cv2.putText(annotated_frame, f"T-Pose Score: {score}%",
                    (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1.0, color, 3)

        # 피드백 메시지 표시
        cv2.putText(annotated_frame, message,
                    (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

        # 완벽한 자세일 때 강조 표시
        if is_correct:
            cv2.putText(annotated_frame, "PERFECT T-POSE!",
                        (10, 110), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 255, 0), 3)

    # 화면 표시
    cv2.imshow('YOLO Pose - T자 서기 테스트', annotated_frame)

    # 'q' 키로 종료
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()