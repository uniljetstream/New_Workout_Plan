#ifndef SETTINGS_PAGE_WIDGET_H
#define SETTINGS_PAGE_WIDGET_H

#include <QWidget>
#include <QString>

namespace Ui {
class SettingsPage;
}

/**
 * @brief 설정 화면 래퍼
 *
 * MQTT 연결 설정, 에어마우스 옵션 등을 외부(MainWindow)와 신호/슬롯으로
 * 연결하기 위한 인터페이스를 제공한다.
 */
class SettingsPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPageWidget(QWidget *parent = nullptr);
    ~SettingsPageWidget();

    QString broker() const;
    int port() const;

    void setBroker(const QString &host);
    void setPort(int port);
    void setMqttStatus(bool connected);
    void setAirMouseStatus(bool enabled);

signals:
    void connectRequested();
    void disconnectRequested();
    void calibrateRequested();
    void toggleAirMouseRequested();
    void saveRequested();
    void backRequested();

    void sensitivityChanged(double value);
    void smoothingChanged(bool enabled);
    void showTrailChanged(bool enabled);

private:
    void updateAirMouseButton(bool enabled);

    Ui::SettingsPage *m_ui;
};

#endif // SETTINGS_PAGE_WIDGET_H
