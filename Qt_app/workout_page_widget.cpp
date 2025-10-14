#include "workout_page_widget.h"

#include "ui_workout.h"
#include "videoframewidget.h"

#include <QLabel>
#include <QPushButton>

WorkoutPageWidget::WorkoutPageWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::WorkoutPage)
{
    m_ui->setupUi(this);

    connect(m_ui->startButton, &QPushButton::clicked,
            this, &WorkoutPageWidget::startRequested);
    connect(m_ui->stopButton, &QPushButton::clicked,
            this, &WorkoutPageWidget::stopRequested);
    connect(m_ui->backButton, &QPushButton::clicked,
            this, &WorkoutPageWidget::backRequested);
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
    resetHeartRate();
    resetRepCount();
    setTimerText(QStringLiteral("00:00"));
}

void WorkoutPageWidget::setExerciseProgress(const QString &exerciseName, int poseIndex, int totalPoses)
{
    m_ui->exerciseTitleLabel->setText(
        tr("운동: %1 (%2/%3)")
            .arg(exerciseName)
            .arg(poseIndex)
            .arg(totalPoses));
}

void WorkoutPageWidget::setFeedbackMessage(const QString &message, const QString &styleSheet)
{
    m_ui->feedbackLabel->setText(message);
    m_ui->feedbackLabel->setStyleSheet(styleSheet);
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
