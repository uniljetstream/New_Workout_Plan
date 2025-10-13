#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QtMqtt/QMqttClient>
#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include "config.h"
#include "videoframewidget.h"
#include "airmouse_manager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainMenuPage;
    class ExerciseSelectionPage;
    class SettingsPage;
    class WorkoutPage;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Page enums
    enum Page {
        PAGE_MAIN_MENU = 0,
        PAGE_EXERCISE_SELECTION = 1,
        PAGE_SETTINGS = 2,
        PAGE_WORKOUT = 3
    };

private slots:
    // Main Menu Page slots
    void on_exerciseSelectButton_clicked();
    void on_settingsButton_clicked();

    // Exercise Selection Page slots
    void on_tPoseButton_clicked();
    void on_squatButton_clicked();
    void on_pushupButton_clicked();
    void on_plankButton_clicked();
    void on_lungeButton_clicked();
    void on_jumpingJackButton_clicked();
    void on_mountainClimberButton_clicked();
    void on_burpeeButton_clicked();
    void on_customButton_clicked();
    void on_scrollUpButton_clicked();
    void on_scrollDownButton_clicked();
    void on_exerciseSelection_backButton_clicked();

    // Settings Page slots
    void on_settings_connectButton_clicked();
    void on_settings_disconnectButton_clicked();
    void on_settings_calibrateButton_clicked();
    void on_settings_testAirMouseButton_clicked();
    void on_settings_saveButton_clicked();
    void on_settings_backButton_clicked();
    void on_sensitivitySlider_valueChanged(int value);
    void on_smoothingCheckBox_toggled(bool checked);
    void on_trailCheckBox_toggled(bool checked);

    // Workout Page slots
    void on_workout_startButton_clicked();
    void on_workout_stopButton_clicked();
    void on_workout_backButton_clicked();

    // MQTT client slots
    void onMqttConnected();
    void onMqttDisconnected();
    void onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic);
    void onMqttStateChanged(QMqttClient::ClientState state);
    void onMqttError(QMqttClient::ClientError error);

    // Timer slot
    void onWorkoutTimerTimeout();

private:
    // UI
    QStackedWidget *m_stackedWidget;
    QWidget *m_mainMenuPage;
    QWidget *m_exerciseSelectionPage;
    QWidget *m_settingsPage;
    QWidget *m_workoutPage;

    Ui::MainMenuPage *ui_mainMenu;
    Ui::ExerciseSelectionPage *ui_exerciseSelection;
    Ui::SettingsPage *ui_settings;
    Ui::WorkoutPage *ui_workout;

    // MQTT
    QMqttClient *m_client;
    Config &m_config;

    // AirMouse
    VideoFrameWidget *m_videoWidget;    // Displays WatchTower video stream
    AirMouseManager *m_airMouseManager;  // Global airmouse controller

    // Workout state
    QString m_currentExercise;
    QString m_currentMode;  // English mode name for MQTT
    QTimer *m_workoutTimer;
    int m_workoutSeconds;
    bool m_isWorkoutRunning;

    // Pose sequence state
    int m_currentPoseIndex;      // Current pose index (0-based)
    int m_totalPoses;            // Total number of poses in current mode
    QJsonArray m_poses;          // Array of pose objects from AI server
    int m_repCount;              // Repetition count

    // Helper methods
    void setupPages();
    void setupMqttClient();
    void setupVideoWidget();
    void setupAirMouse();
    void switchToPage(Page page);
    void loadConfiguration();
    void subscribeToTopics();
    void publishMessage(const QString &topic, const QString &message);
    void updateMqttConnectionStatus(bool connected);
    void updateSensorData(const QJsonObject &data, bool isJoystick);
    void updateAirMouseData(const QJsonObject &data);
    void updateWorkoutFeedback(const QJsonObject &data);
    void displayVideoFrame(const QString &base64Frame);
    void clearVideoFrame(const QString &message = QString());
    void startWorkout(const QString &exerciseName);
    void stopWorkout();
    void updateWorkoutTimer();
    void sendAirMouseModeCommand();
    void sendSensorModeCommand();

    // MQTT protocol helpers
    QString convertExerciseNameToMode(const QString &exerciseName);
    void sendModeSelectCommand(const QString &mode);
    void handleQtResponse(const QString &responseType, const QJsonObject &data);

    // Pose sequence helpers
    void updatePoseDisplay();
    void nextPose();
    bool isLastPose() const;
};

#endif // MAINWINDOW_H
