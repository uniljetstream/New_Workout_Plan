#ifndef WORKOUT_PAGE_WIDGET_H
#define WORKOUT_PAGE_WIDGET_H

#include <QWidget>
#include <QString>

class VideoFrameWidget;

namespace Ui {
class WorkoutPage;
}

/**
 * @brief 운동 진행 화면 래퍼
 *
 * 운동 진행 정보(피드백, 점수, 심박수 등)를 업데이트하는
 * 헬퍼 메서드와 시작/중지/뒤로가기 시그널을 제공한다.
 */
class WorkoutPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WorkoutPageWidget(QWidget *parent = nullptr);
    ~WorkoutPageWidget();

    VideoFrameWidget *videoWidget() const;

    void prepareForExercise(const QString &exerciseName);
    void setExerciseProgress(const QString &exerciseName, int poseIndex, int totalPoses);

    void setFeedbackMessage(const QString &message, const QString &styleSheet = QString());
    void setFeedbackStyle(const QString &styleSheet);
    QString feedbackText() const;

    void setScore(int score);
    void resetScore();

    void setHeartRate(int bpm);
    void resetHeartRate();

    void setRepCount(int count);
    void resetRepCount();

    void setTimerText(const QString &text);

signals:
    void startRequested();
    void stopRequested();
    void backRequested();

private:
    Ui::WorkoutPage *m_ui;
};

#endif // WORKOUT_PAGE_WIDGET_H
