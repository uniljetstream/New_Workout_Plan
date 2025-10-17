#ifndef EXERCISE_SELECTION_PAGE_WIDGET_H
#define EXERCISE_SELECTION_PAGE_WIDGET_H

#include <QWidget>
#include <QString>

namespace Ui {
class ExerciseSelectionPage;
}

class QPushButton;

/**
 * @brief 운동 선택 화면 래퍼
 *
 * 운동 버튼 클릭/뒤로가기 요청을 시그널로 외부에 전달하고
 * 스크롤 버튼은 내부에서 직접 처리한다.
 */
class ExerciseSelectionPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExerciseSelectionPageWidget(QWidget *parent = nullptr);
    ~ExerciseSelectionPageWidget();

signals:
    void exerciseSelected(const QString &exerciseName);
    void featureUnavailable(const QString &message);
    void backRequested();

private:
    void connectSignals();
    void populateExercises();
    void clearExerciseButtons();
    QPushButton *createExerciseButton(const QString &label, const QString &color);

    Ui::ExerciseSelectionPage *m_ui;
};

#endif // EXERCISE_SELECTION_PAGE_WIDGET_H
