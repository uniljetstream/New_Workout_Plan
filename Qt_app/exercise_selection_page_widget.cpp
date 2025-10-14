#include "exercise_selection_page_widget.h"

#include "ui_exercise_selection.h"

#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>

ExerciseSelectionPageWidget::ExerciseSelectionPageWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ExerciseSelectionPage)
{
    m_ui->setupUi(this);
    connectSignals();
}

ExerciseSelectionPageWidget::~ExerciseSelectionPageWidget()
{
    delete m_ui;
}

void ExerciseSelectionPageWidget::connectSignals()
{
    connect(m_ui->squatButton, &QPushButton::clicked, this, [this]() {
        emit exerciseSelected(tr("스쿼트"));
    });

    connect(m_ui->pushupButton, &QPushButton::clicked, this, [this]() {
        emit exerciseSelected(tr("푸시업"));
    });

    connect(m_ui->plankButton, &QPushButton::clicked, this, [this]() {
        emit featureUnavailable(tr("플랭크 운동은 곧 출시됩니다."));
    });
    connect(m_ui->lungeButton, &QPushButton::clicked, this, [this]() {
        emit featureUnavailable(tr("런지 운동은 곧 출시됩니다."));
    });
    connect(m_ui->jumpingJackButton, &QPushButton::clicked, this, [this]() {
        emit featureUnavailable(tr("점핑잭 운동은 곧 출시됩니다."));
    });
    connect(m_ui->mountainClimberButton, &QPushButton::clicked, this, [this]() {
        emit featureUnavailable(tr("마운틴 클라이머 운동은 곧 출시됩니다."));
    });
    connect(m_ui->burpeeButton, &QPushButton::clicked, this, [this]() {
        emit featureUnavailable(tr("버피 운동은 곧 출시됩니다."));
    });
    connect(m_ui->customButton, &QPushButton::clicked, this, [this]() {
        emit featureUnavailable(tr("사용자 정의 운동 기능은 곧 출시됩니다."));
    });

    connect(m_ui->scrollUpButton, &QPushButton::clicked, this, [this]() {
        if (auto *scrollBar = m_ui->exerciseScrollArea->verticalScrollBar()) {
            scrollBar->setValue(scrollBar->value() - 100);
        }
    });

    connect(m_ui->scrollDownButton, &QPushButton::clicked, this, [this]() {
        if (auto *scrollBar = m_ui->exerciseScrollArea->verticalScrollBar()) {
            scrollBar->setValue(scrollBar->value() + 100);
        }
    });

    connect(m_ui->backButton, &QPushButton::clicked,
            this, &ExerciseSelectionPageWidget::backRequested);
}
