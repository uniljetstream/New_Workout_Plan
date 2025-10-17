#ifndef MAIN_MENU_PAGE_WIDGET_H
#define MAIN_MENU_PAGE_WIDGET_H

#include <QWidget>
#include <QString>

namespace Ui {
class MainMenuPage;
}

/**
 * @brief 메인 메뉴 화면 래퍼
 *
 * 기존 main.ui에서 생성된 Ui::MainMenuPage를 캡슐화하고
 * 화면 간 전환을 위한 시그널만 외부로 노출한다.
 */
class MainMenuPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenuPageWidget(QWidget *parent = nullptr);
    ~MainMenuPageWidget();

    void setStatusText(const QString &text);

signals:
    void exerciseSelectRequested();
    void settingsRequested();

private:
    Ui::MainMenuPage *m_ui;
};

#endif // MAIN_MENU_PAGE_WIDGET_H
