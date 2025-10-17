#ifndef RESULT_PAGE_WIDGET_H
#define RESULT_PAGE_WIDGET_H

#include <QWidget>
#include <QString>
#include <QPixmap>

namespace Ui {
class ResultPage;
}

/**
 * @brief 운동 결과 화면 래퍼
 *
 * 루틴 운동 완료 후 총점, 랭크, 소요 시간 등을 표시한다.
 */
class ResultPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResultPageWidget(QWidget *parent = nullptr);
    ~ResultPageWidget();

    void setResults(int totalScore, int durationSeconds, int exerciseCount);
    void updateRankDisplay(const QString &rank);

signals:
    void retryRequested();
    void backRequested();

private:
    void setRankImage(const QString &rank);
    QString getRankFromScore(int totalScore, int exerciseCount) const;
    QPixmap createDefaultRankImage(const QString &rank);

    Ui::ResultPage *m_ui;
};

#endif // RESULT_PAGE_WIDGET_H