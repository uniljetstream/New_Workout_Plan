#include "workout_page_widget.h"

#include "ui_workout.h"
#include "videoframewidget.h"

#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>

WorkoutPageWidget::WorkoutPageWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::WorkoutPage)
{
    m_ui->setupUi(this);

    // 고정된 배너 영역 유지 (문구 길이에 따라 레이아웃이 흔들리는 것 방지)
    m_ui->feedbackLabel->setWordWrap(true);
    m_ui->feedbackLabel->setAlignment(Qt::AlignCenter);
    m_ui->feedbackLabel->setMinimumHeight(120);
    m_ui->feedbackLabel->setMaximumHeight(120);
    QSizePolicy policy = m_ui->feedbackLabel->sizePolicy();
    policy.setVerticalPolicy(QSizePolicy::Fixed);
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_ui->feedbackLabel->setSizePolicy(policy);

    // currentPoseLabel 설정
    m_ui->currentPoseLabel->setWordWrap(true);
    m_ui->currentPoseLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_ui->currentPoseLabel->setMinimumHeight(60);
    QSizePolicy posePolicy = m_ui->currentPoseLabel->sizePolicy();
    posePolicy.setVerticalPolicy(QSizePolicy::Minimum);
    posePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_ui->currentPoseLabel->setSizePolicy(posePolicy);

    connect(m_ui->startButton, &QPushButton::clicked,
            this, &WorkoutPageWidget::startRequested);
    connect(m_ui->stopButton, &QPushButton::clicked,
            this, &WorkoutPageWidget::stopRequested);
    connect(m_ui->backButton, &QPushButton::clicked,
            this, &WorkoutPageWidget::backRequested);
    connect(m_ui->skipButton, &QPushButton::clicked,
            this, &WorkoutPageWidget::skipRequested);
}

WorkoutPageWidget::~WorkoutPageWidget()
{
    delete m_ui;
}

VideoFrameWidget *WorkoutPageWidget::videoWidget() const
{
    return m_ui->videoWidget;
}

void WorkoutPageWidget::prepareForExercise(const QString &exerciseName)
{
    m_ui->exerciseTitleLabel->setText(tr("운동: %1").arg(exerciseName));
    resetScore();
    setFeedbackMessage(tr("시작 버튼을 눌러주세요"));
    setCurrentPose(tr("대기 중..."));
    resetHeartRate();
    resetRepCount();
    setTimerText(QStringLiteral("00:00"));
    clearRoutineInfo();
    setSkipButtonVisible(false);
}

void WorkoutPageWidget::setExerciseProgress(const QString &exerciseName, int poseIndex, int totalPoses)
{
    m_ui->exerciseTitleLabel->setText(
        tr("운동: %1 (%2/%3)")
            .arg(exerciseName)
            .arg(poseIndex)
            .arg(totalPoses));
}

void WorkoutPageWidget::setCurrentPose(const QString &poseName)
{
    m_ui->currentPoseLabel->setText(tr("현재 자세: %1").arg(poseName));
}

void WorkoutPageWidget::setFeedbackMessage(const QString &message, const QString &styleSheet)
{
    m_ui->feedbackLabel->setText(message);
    if (!styleSheet.isEmpty())
    {
        m_ui->feedbackLabel->setStyleSheet(styleSheet);
    }
}

void WorkoutPageWidget::setFeedbackStyle(const QString &styleSheet)
{
    m_ui->feedbackLabel->setStyleSheet(styleSheet);
}

QString WorkoutPageWidget::feedbackText() const
{
    return m_ui->feedbackLabel->text();
}

void WorkoutPageWidget::setScore(int score)
{
    m_ui->scoreLabel->setText(tr("점수: %1").arg(score));
}

void WorkoutPageWidget::resetScore()
{
    m_ui->scoreLabel->setText(tr("점수: --"));
}

void WorkoutPageWidget::setHeartRate(int bpm)
{
    if (bpm < 0) {
        resetHeartRate();
    } else {
        m_ui->heartRateLabel->setText(tr("심박수: %1 BPM").arg(bpm));
    }
}

void WorkoutPageWidget::resetHeartRate()
{
    m_ui->heartRateLabel->setText(tr("심박수: -- BPM"));
}

void WorkoutPageWidget::setRepCount(int count)
{
    m_ui->repCountLabel->setText(tr("반복 횟수: %1").arg(count));
}

void WorkoutPageWidget::resetRepCount()
{
    m_ui->repCountLabel->setText(tr("반복 횟수: 0"));
}

void WorkoutPageWidget::setTimerText(const QString &text)
{
    m_ui->timerLabel->setText(text);
}

void WorkoutPageWidget::setRoutineInfo(const QString &currentExercise, int remainingReps)
{
    m_ui->routineInfoLabel->setText(
        tr("[루틴] %1 (남은 횟수: %2회)")
            .arg(currentExercise)
            .arg(remainingReps));
}

void WorkoutPageWidget::clearRoutineInfo()
{
    m_ui->routineInfoLabel->setText(QString());
}

void WorkoutPageWidget::setSkipButtonVisible(bool visible)
{
    m_ui->skipButton->setVisible(visible);
}