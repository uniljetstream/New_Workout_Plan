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
class ResultPageWidget;
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
        PAGE_WORKOUT = 3,
        PAGE_RESULT = 4
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
    void handleWorkoutSkipRequested();  // 새로 추가

    // Result Page handlers
    void handleResultRetryRequested();
    void handleResultBackRequested();

    // MQTT client slots
    void onMqttConnected();
    void onMqttDisconnected();
    void onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic);
    void onMqttStateChanged(QMqttClient::ClientState state);
    void onMqttError(QMqttClient::ClientError error);

    // Timer slot
    void onWorkoutTimerTimeout();
    void onPoseAnalysisTimeout();

private:
    // UI
    QStackedWidget *m_stackedWidget;
    MainMenuPageWidget *m_mainMenuPage;
    ExerciseSelectionPageWidget *m_exerciseSelectionPage;
    SettingsPageWidget *m_settingsPage;
    WorkoutPageWidget *m_workoutPage;
    ResultPageWidget *m_resultPage;  // 새로 추가

    // MQTT
    QMqttClient *m_client;
    Config &m_config;
    QTimer *m_mqttReconnectTimer;
    bool m_shouldAutoReconnect;
    bool m_userRequestedDisconnect;

    // AirMouse
    VideoFrameWidget *m_videoWidget;
    AirMouseManager *m_airMouseManager;

    // Workout state
    QString m_currentExercise;
    QString m_currentMode;
    QTimer *m_workoutTimer;
    QTimer *m_poseAnalysisTimer;
    int m_workoutSeconds;
    bool m_isWorkoutRunning;

    // Pose sequence state
    int m_currentPoseIndex;
    int m_totalPoses;
    QJsonArray m_poses;
    int m_repCount;
    int m_poseSuccessCounter;
    QString m_lastAnalyzedPoseName;
    bool m_manualFeedbackActive;
    int m_poseAnalysisTargetIndex;
    bool m_poseAnalysisPending;
    QString m_lastServerFeedback;

    // 심박수 통계 추적 변수
    QList<int> m_heartRateHistory;  // 운동 중 수집된 심박수 데이터
    int m_minHeartRate;             // 최소 심박수
    int m_maxHeartRate;             // 최대 심박수
    int m_avgHeartRate;             // 평균 심박수


    // 루틴 모드 관련 변수 (새로 추가)
    bool m_isRoutineMode;                    // 루틴 모드 여부
    QStringList m_routineExercises;          // 루틴에 포함된 운동 목록 (mode 문자열)
    int m_currentRoutineIndex;               // 현재 루틴 내 운동 인덱스
    int m_routineExerciseRepCount;          // 현재 루틴 운동의 반복 횟수
    QVector<int> m_routineScores;           // 각 운동별 점수 저장
    int m_routineTotalScore;                // 루틴 총 점수
             // 루틴 총 점수
    int m_currentExerciseAccumulatedScore;  // ⭐ 이 한 줄 추가
    int m_targetRepsForCurrentExercise;
    bool m_autoStopScheduled;
    bool m_readyForRepCount;
    int m_pendingRoutineScore;
    int m_lastCompletionScore;

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
    void stopWorkout(bool autoStopTriggered = false);
    void updateWorkoutTimer();
    void sendAirMouseModeCommand();
    void sendSensorModeCommand();
    void sendPoseIndex(int poseIndex);
    void updateAirMouseStatusIndicator(bool enabled);
    void attemptMqttReconnect();
    void scheduleMqttReconnect();
    void schedulePoseAnalysis(int poseIndex, int delayMs = 1000);
    void requestPoseAnalysis(int poseIndex);

        // 루틴 모드 헬퍼 메서드
    bool isRoutineMode(const QString &mode) const;
    void initializeRoutineMode(const QString &routineMode);
    void startNextRoutineExercise();
    void completeRoutineExercise();
    void finishRoutine();
    void updateExerciseProgressInfo();
    QString getExerciseDisplayName(const QString &mode) const;
    void refreshTargetRepCount();
    void scheduleAutoStop();
    void showIndividualResults(const QString &exerciseName, int finalScore, int durationSeconds, int completedReps);
    int getMaxScoreForRoutine(const QString &routineName) const;  // ⭐ 추가
    int calculateScoreGrade(int score, int maxScore) const;       // ⭐ 추가
    int routineRepsPerExercise() const;

    // MQTT protocol helpers
    QString convertExerciseNameToMode(const QString &exerciseName);
    void sendModeSelectCommand(const QString &mode);
    void handleQtResponse(const QString &responseType, const QJsonObject &data);

    // 심박수 통계 관련
    void resetHeartRateStats();
    void updateHeartRateStats(int bpm);
    void calculateHeartRateStats();

    // Pose sequence helpers
    void updatePoseDisplay();
    void nextPose();
    bool isLastPose() const;
    void handleSquatPoseSuccess();
    void handleLungePoseSuccess();
    void resetPoseSuccessState();
    QString currentPoseName() const;
    void setFeedbackBanner(const QString &message, bool success);
    QString squatInstructionText(int poseIndex) const;
    QString lungeInstructionText(int poseIndex) const;
    void updateFeedbackLabel(const QString &baseMessage, const QString &styleSheet = QString(), bool includeServerFeedback = true);
    QString composeFeedbackMessage(const QString &baseMessage, bool includeServerFeedback) const;
    QString translateFeedbackText(const QString &feedback) const;

    // 루틴 모드 헬퍼 메서드 (새로 추가)






};

#endif // MAINWINDOW_H
