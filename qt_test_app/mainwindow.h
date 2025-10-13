#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMqtt/QMqttClient>
#include <QTimer>
#include <QDateTime>
#include "config.h"
#include "cursorcanvas.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // MQTT connection slots
    void on_connectButton_clicked();
    void on_disconnectButton_clicked();

    // WatchTower command slots
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_selectModeButton_clicked();
    void on_airMouseModeButton_clicked();
    void on_sensorModeButton_clicked();
    void on_calibrateButton_clicked();

    // AirMouse control slots
    void on_resetCursorButton_clicked();
    void on_cursorSensitivitySlider_valueChanged(int value);
    void on_cursorSmoothingCheckBox_toggled(bool checked);
    void on_showTrailCheckBox_toggled(bool checked);
    void on_testCursorButton_clicked();

    // Test timer
    void onTestTimerTimeout();

    // Clear log slot
    void on_clearLogButton_clicked();

    // MQTT client slots
    void onMqttConnected();
    void onMqttDisconnected();
    void onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic);
    void onMqttStateChanged(QMqttClient::ClientState state);
    void onMqttError(QMqttClient::ClientError error);

private:
    Ui::MainWindow *ui;
    QMqttClient *m_client;
    Config &m_config;
    CursorCanvas *m_cursorCanvas;
    QTimer *m_testTimer;

    // Helper methods
    void loadConfiguration();
    void setupMqttClient();
    void setupCursorCanvas();
    void subscribeToTopics();
    void publishMessage(const QString &topic, const QString &message);
    void appendLog(const QString &message, const QString &color = "black");
    void updateSensorData(const QJsonObject &data, bool isJoystick);
    void updateAirMouseData(const QJsonObject &data);
    void updateConnectionStatus(bool connected);
    void loadExerciseModes();
};
#endif // MAINWINDOW_H
