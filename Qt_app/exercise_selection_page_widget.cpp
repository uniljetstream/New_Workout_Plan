#include "exercise_selection_page_widget.h"

#include "ui_exercise_selection.h"

#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QLayoutItem>
#include <QFont>
#include <QVector>

#include "exercise_catalog.h"

ExerciseSelectionPageWidget::ExerciseSelectionPageWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ExerciseSelectionPage)
{
    m_ui->setupUi(this);
    populateExercises();
    connectSignals();
}

ExerciseSelectionPageWidget::~ExerciseSelectionPageWidget()
{
    delete m_ui;
}

void ExerciseSelectionPageWidget::connectSignals()
{
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

void ExerciseSelectionPageWidget::populateExercises()
{
    clearExerciseButtons();

    const QVector<QString> colorPalette = {
        QStringLiteral("#4ECDC4"),
        QStringLiteral("#95E1D3"),
        QStringLiteral("#F38181"),
        QStringLiteral("#A8E6CF"),
        QStringLiteral("#FFD3B6"),
        QStringLiteral("#FFAAA5"),
        QStringLiteral("#B39CD0"),
        QStringLiteral("#C7CEEA")
    };

    const auto &catalog = exerciseCatalog();
    QFont buttonFont;
    buttonFont.setPointSize(18);
    buttonFont.setBold(true);

    int colorIndex = 0;
    for (const ExerciseOption &option : catalog) {
        const QString color = colorPalette[colorIndex % colorPalette.size()];
        QPushButton *button = createExerciseButton(option.displayName, color);
        button->setFont(buttonFont);

        connect(button, &QPushButton::clicked, this, [this, name = option.displayName]() {
            emit exerciseSelected(name);
        });

        m_ui->verticalLayout_exercises->addWidget(button);
        ++colorIndex;
    }

    m_ui->verticalLayout_exercises->addStretch();
}

void ExerciseSelectionPageWidget::clearExerciseButtons()
{
    if (!m_ui || !m_ui->verticalLayout_exercises) {
        return;
    }

    QLayoutItem *item = nullptr;
    while ((item = m_ui->verticalLayout_exercises->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

QPushButton *ExerciseSelectionPageWidget::createExerciseButton(const QString &label, const QString &color)
{
    auto *button = new QPushButton(label, m_ui->scrollAreaWidgetContents);
    button->setMinimumHeight(80);
    button->setStyleSheet(
        QStringLiteral("background-color: %1; color: white; border-radius: 10px; text-align: left; padding-left: 30px;")
            .arg(color));
    return button;
}
