#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QImage>
#include <QProcess>
#include <opencv2/opencv.hpp>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    // 버튼 클릭 슬롯
    void on_btnSquat_clicked();
    void on_btnPushup_clicked();
    void on_btnLunge_clicked();
    void on_btnPlank_clicked();
    void on_btnStart_clicked();
    void on_btnStop_clicked();

    // 타이머 슬롯
    void updateFrame();

    // MQTT 구독 프로세스 슬롯
    void onMqttSubOutput();

private:
    Ui::Widget *ui;

    // MQTT 통신 (mosquitto_pub/sub 사용)
    QProcess *m_mqttSub;
    void connectMqtt();
    void disconnectMqtt();
    void publishMessage(const QString &topic, const QString &payload);
    void processMqttMessage(const QString &topic, const QString &message);

    // 카메라
    cv::VideoCapture m_camera;
    QTimer *m_cameraTimer;
    bool initCamera();
    void stopCamera();
    void displayFrame(const cv::Mat &frame);

    // 상태 관리
    QString m_currentMode;
    bool m_isRunning;
    void selectMode(const QString &mode);
    void updateModeButtons();
    void updateUI(bool running);

    // MQTT 설정
    const QString MQTT_BROKER = "localhost";
    const int MQTT_PORT = 1883;

    // MQTT 토픽
    const QString TOPIC_QT_SELECT_MODE = "watchtower/qt/select_mode";
    const QString TOPIC_QT_STOP = "watchtower/qt/stop";
    const QString TOPIC_QT_MODE_SELECTED = "watchtower/qt/mode_selected";
    const QString TOPIC_QT_STATUS = "watchtower/qt/status";
    const QString TOPIC_QT_ANALYSIS = "watchtower/qt/analysis";

    // 카메라 설정
    const int CAMERA_ID = 0;
    const int CAMERA_WIDTH = 1280;
    const int CAMERA_HEIGHT = 720;
    const int CAMERA_FPS = 30;
    const int DISPLAY_FPS = 30;
};

#endif // WIDGET_H
