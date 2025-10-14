#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QtMqtt/QMqttClient>
#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include "config.h"
#include "airmouse_manager.h"

class MainMenuPageWidget;
class ExerciseSelectionPageWidget;
class SettingsPageWidget;
class WorkoutPageWidget;
class VideoFrameWidget;

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
    // Main Menu Page handlers
    void handleExerciseSelectRequested();
    void handleSettingsRequested();

    // Exercise Selection Page handlers
    void handleExerciseSelected(const QString &exerciseName);
    void handleFeatureUnavailable(const QString &message);
    void handleExerciseSelectionBack();

    // Settings Page handlers
    void handleConnectRequested();
    void handleDisconnectRequested();
    void handleCalibrateRequested();
    void handleAirMouseToggleRequested();
    void handleSaveRequested();
    void handleSettingsBackRequested();
    void handleSensitivityChanged(double value);
    void handleSmoothingChanged(bool checked);
    void handleTrailChanged(bool checked);

    // Workout Page handlers
    void handleWorkoutStartRequested();
    void handleWorkoutStopRequested();
    void handleWorkoutBackRequested();

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
    MainMenuPageWidget *m_mainMenuPage;
    ExerciseSelectionPageWidget *m_exerciseSelectionPage;
    SettingsPageWidget *m_settingsPage;
    WorkoutPageWidget *m_workoutPage;

    // MQTT
    QMqttClient *m_client;
    Config &m_config;
    QTimer *m_mqttReconnectTimer;
    bool m_shouldAutoReconnect;
    bool m_userRequestedDisconnect;

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
    void updateAirMouseStatusIndicator(bool enabled);
    void attemptMqttReconnect();
    void scheduleMqttReconnect();

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
