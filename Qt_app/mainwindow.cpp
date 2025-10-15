#include "mainwindow.h"

#include "exercise_selection_page_widget.h"
#include "main_menu_page_widget.h"
#include "settings_page_widget.h"
#include "videoframewidget.h"
#include "workout_page_widget.h"
#include "exercise_catalog.h"

#include <QDateTime>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStatusBar>

namespace
{
    constexpr int kDefaultPoseSuccessThreshold = 1;
    constexpr int kSquatPoseSuccessThreshold = 1;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_stackedWidget(nullptr), m_mainMenuPage(nullptr), m_exerciseSelectionPage(nullptr),
      m_settingsPage(nullptr), m_workoutPage(nullptr), m_client(nullptr), m_config(Config::instance()),
      m_videoWidget(nullptr), m_airMouseManager(nullptr), m_currentExercise(""), m_currentMode(""),
      m_workoutTimer(nullptr), m_workoutSeconds(0), m_isWorkoutRunning(false), m_currentPoseIndex(0), m_totalPoses(0),
      m_repCount(0), m_poseSuccessCounter(0), m_lastAnalyzedPoseName(), m_manualFeedbackActive(false),
      m_lastServerFeedback()
{
    loadConfiguration();
    setupPages();
    m_mqttReconnectTimer = new QTimer(this);
    m_mqttReconnectTimer->setInterval(3000);
    m_mqttReconnectTimer->setSingleShot(true);
    connect(m_mqttReconnectTimer, &QTimer::timeout, this, &MainWindow::attemptMqttReconnect);
    m_shouldAutoReconnect = m_config.autoConnect();
    m_userRequestedDisconnect = false;
    setupMqttClient();
    setupVideoWidget();
    setupAirMouse();

    m_poseAnalysisTimer = new QTimer(this);
    m_poseAnalysisTimer->setSingleShot(true);
    connect(m_poseAnalysisTimer, &QTimer::timeout, this, &MainWindow::onPoseAnalysisTimeout);
    m_poseAnalysisPending = false;
    m_poseAnalysisTargetIndex = 0;

    // Start with main menu
    switchToPage(PAGE_MAIN_MENU);

    // Set window properties
    setWindowTitle("홈 트레이닝 시스템");
    resize(m_config.windowWidth(), m_config.windowHeight());
}

MainWindow::~MainWindow()
{
    if (m_client)
    {
        m_client->disconnectFromHost();
    }
}

void MainWindow::loadConfiguration()
{
    if (!m_config.loadFromFile("config.json"))
    {
        qDebug() << "Failed to load config.json, using defaults";
    }
    else
    {
        qDebug() << "Configuration loaded successfully";
    }
}

void MainWindow::setupPages()
{
    // Create stacked widget
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Create Main Menu Page
    m_mainMenuPage = new MainMenuPageWidget();
    m_stackedWidget->addWidget(m_mainMenuPage);

    connect(m_mainMenuPage, &MainMenuPageWidget::exerciseSelectRequested, this,
            &MainWindow::handleExerciseSelectRequested);
    connect(m_mainMenuPage, &MainMenuPageWidget::settingsRequested, this, &MainWindow::handleSettingsRequested);

    // Create Exercise Selection Page
    m_exerciseSelectionPage = new ExerciseSelectionPageWidget();
    m_stackedWidget->addWidget(m_exerciseSelectionPage);

    connect(m_exerciseSelectionPage, &ExerciseSelectionPageWidget::exerciseSelected, this,
            &MainWindow::handleExerciseSelected);
    connect(m_exerciseSelectionPage, &ExerciseSelectionPageWidget::backRequested, this,
            &MainWindow::handleExerciseSelectionBack);

    // Create Settings Page
    m_settingsPage = new SettingsPageWidget();
    m_stackedWidget->addWidget(m_settingsPage);

    connect(m_settingsPage, &SettingsPageWidget::connectRequested, this, &MainWindow::handleConnectRequested);
    connect(m_settingsPage, &SettingsPageWidget::disconnectRequested, this, &MainWindow::handleDisconnectRequested);
    connect(m_settingsPage, &SettingsPageWidget::calibrateRequested, this, &MainWindow::handleCalibrateRequested);
    connect(m_settingsPage, &SettingsPageWidget::toggleAirMouseRequested, this,
            &MainWindow::handleAirMouseToggleRequested);
    connect(m_settingsPage, &SettingsPageWidget::saveRequested, this, &MainWindow::handleSaveRequested);
    connect(m_settingsPage, &SettingsPageWidget::backRequested, this, &MainWindow::handleSettingsBackRequested);
    connect(m_settingsPage, &SettingsPageWidget::sensitivityChanged, this, &MainWindow::handleSensitivityChanged);
    connect(m_settingsPage, &SettingsPageWidget::smoothingChanged, this, &MainWindow::handleSmoothingChanged);
    connect(m_settingsPage, &SettingsPageWidget::showTrailChanged, this, &MainWindow::handleTrailChanged);

    m_settingsPage->setBroker(m_config.mqttBroker());
    m_settingsPage->setPort(m_config.mqttPort());

    // Create Workout Page
    m_workoutPage = new WorkoutPageWidget();
    m_stackedWidget->addWidget(m_workoutPage);

    connect(m_workoutPage, &WorkoutPageWidget::startRequested, this, &MainWindow::handleWorkoutStartRequested);
    connect(m_workoutPage, &WorkoutPageWidget::stopRequested, this, &MainWindow::handleWorkoutStopRequested);
    connect(m_workoutPage, &WorkoutPageWidget::backRequested, this, &MainWindow::handleWorkoutBackRequested);

    // Initialize workout timer
    m_workoutTimer = new QTimer(this);
    connect(m_workoutTimer, &QTimer::timeout, this, &MainWindow::onWorkoutTimerTimeout);

    qDebug() << "All pages setup complete";
}

void MainWindow::setupMqttClient()
{
    m_client = new QMqttClient(this);
    m_client->setHostname(m_config.mqttBroker());
    m_client->setPort(m_config.mqttPort());

    if (!m_config.mqttClientId().isEmpty())
    {
        m_client->setClientId(m_config.mqttClientId());
    }

    if (!m_config.mqttUsername().isEmpty())
    {
        m_client->setUsername(m_config.mqttUsername());
    }

    if (!m_config.mqttPassword().isEmpty())
    {
        m_client->setPassword(m_config.mqttPassword());
    }

    connect(m_client, &QMqttClient::connected, this, &MainWindow::onMqttConnected);
    connect(m_client, &QMqttClient::disconnected, this, &MainWindow::onMqttDisconnected);
    connect(m_client, &QMqttClient::messageReceived, this, &MainWindow::onMqttMessageReceived);
    connect(m_client, &QMqttClient::stateChanged, this, &MainWindow::onMqttStateChanged);
    connect(m_client, &QMqttClient::errorChanged, this, &MainWindow::onMqttError);

    if (m_shouldAutoReconnect)
    {
        attemptMqttReconnect();
    }

    qDebug() << "MQTT client setup complete";
}

void MainWindow::setupVideoWidget()
{
    if (!m_workoutPage)
    {
        return;
    }

    m_videoWidget = m_workoutPage->videoWidget();
    if (m_videoWidget)
    {
        m_videoWidget->clearFrame(tr("영상 신호 대기 중..."));
    }

    qDebug() << "Video widget initialized";
}

void MainWindow::setupAirMouse()
{
    // Create global airmouse manager
    m_airMouseManager = new AirMouseManager(this);

    // Set default settings (감도 낮춤)
    m_airMouseManager->setSensitivity(0.8); // 1.5 → 0.8 (약 절반으로 감소)
    m_airMouseManager->setSmoothing(true);
    m_airMouseManager->setShowCursor(true);

    // Initially disabled
    m_airMouseManager->setEnabled(false);
    updateAirMouseStatusIndicator(false);

    qDebug() << "AirMouse manager initialized";
}

void MainWindow::switchToPage(Page page)
{
    m_stackedWidget->setCurrentIndex(static_cast<int>(page));
    qDebug() << "Switched to page:" << page;
}

void MainWindow::subscribeToTopics()
{
    // WatchTower가 재발행하는 응답 토픽만 구독
    auto sub = m_client->subscribe(m_config.topicQtResponse());
    if (!sub)
    {
        qDebug() << "Failed to subscribe to" << m_config.topicQtResponse();
    }
    else
    {
        qDebug() << "Subscribed to" << m_config.topicQtResponse();
    }
}

void MainWindow::publishMessage(const QString &topic, const QString &message)
{
    if (m_client->state() != QMqttClient::Connected)
    {
        qDebug() << "Not connected to MQTT broker";
        return;
    }

    qint32 id = m_client->publish(topic, message.toUtf8(), 0);
    if (id == -1)
    {
        qDebug() << "Failed to publish to" << topic;
    }
    else
    {
        qDebug() << "Published to" << topic << ":" << message;
    }
}

void MainWindow::updateMqttConnectionStatus(bool connected)
{
    if (m_settingsPage)
    {
        m_settingsPage->setMqttStatus(connected);
    }

    if (m_mainMenuPage)
    {
        const QString statusText = connected ? tr("상태: MQTT 연결됨") : tr("상태: MQTT 연결 안됨");
        m_mainMenuPage->setStatusText(statusText);
    }
}

void MainWindow::updateAirMouseStatusIndicator(bool enabled)
{
    if (m_settingsPage)
    {
        m_settingsPage->setAirMouseStatus(enabled);
    }
}

void MainWindow::attemptMqttReconnect()
{
    if (!m_client)
    {
        return;
    }

    if (!m_shouldAutoReconnect && m_userRequestedDisconnect)
    {
        qDebug() << "MQTT reconnect skipped (user requested disconnect)";
        return;
    }

    if (m_client->state() == QMqttClient::Connected || m_client->state() == QMqttClient::Connecting)
    {
        return;
    }

    qDebug() << "Attempting MQTT connection to" << m_client->hostname() << ":" << m_client->port();
    m_client->connectToHost();
}

void MainWindow::scheduleMqttReconnect()
{
    if (!m_mqttReconnectTimer || m_userRequestedDisconnect || !m_shouldAutoReconnect)
    {
        return;
    }

    if (m_client && m_client->state() == QMqttClient::Connected)
    {
        return;
    }

    if (!m_mqttReconnectTimer->isActive())
    {
        qDebug() << "Scheduling MQTT reconnect in" << m_mqttReconnectTimer->interval() << "ms";
        m_mqttReconnectTimer->start();
    }
}

// ============================================================================
// Main Menu Page Handlers
// ============================================================================

void MainWindow::handleExerciseSelectRequested()
{
    switchToPage(PAGE_EXERCISE_SELECTION);
}

void MainWindow::handleSettingsRequested()
{
    switchToPage(PAGE_SETTINGS);
}

// ============================================================================
// Exercise Selection Page Handlers
// ============================================================================

void MainWindow::handleExerciseSelected(const QString &exerciseName)
{
    startWorkout(exerciseName);
}

void MainWindow::handleFeatureUnavailable(const QString &message)
{
    statusBar()->showMessage(message, 5000);
}

void MainWindow::handleExerciseSelectionBack()
{
    switchToPage(PAGE_MAIN_MENU);
}

// ============================================================================
// Settings Page Handlers
// ============================================================================

void MainWindow::handleConnectRequested()
{
    m_client->setHostname(m_settingsPage->broker());
    m_client->setPort(m_settingsPage->port());
    if (m_mqttReconnectTimer)
    {
        m_mqttReconnectTimer->stop();
    }
    m_userRequestedDisconnect = false;
    m_shouldAutoReconnect = true;
    attemptMqttReconnect();
    qDebug() << "Connecting to broker...";
}

void MainWindow::handleDisconnectRequested()
{
    if (m_mqttReconnectTimer)
    {
        m_mqttReconnectTimer->stop();
    }
    m_userRequestedDisconnect = true;
    m_shouldAutoReconnect = false;
    m_client->disconnectFromHost();
    qDebug() << "Disconnecting from broker...";
}

void MainWindow::handleCalibrateRequested()
{
    QJsonObject json;
    json["command"] = "calibrate";

    QJsonDocument doc(json);
    publishMessage(m_config.topicWatchtowerCmdJoystick(), doc.toJson(QJsonDocument::Compact));

    statusBar()->showMessage(tr("조이스틱 캘리브레이션 명령을 전송했습니다."), 5000);
}

void MainWindow::handleAirMouseToggleRequested()
{
    if (!m_airMouseManager)
    {
        return;
    }

    const bool newState = !m_airMouseManager->isEnabled();
    m_airMouseManager->setEnabled(newState);

    if (newState)
    {
        sendAirMouseModeCommand();
    }
    else
    {
        sendSensorModeCommand();
    }

    updateAirMouseStatusIndicator(newState);
    qDebug() << "AirMouse toggled. Enabled?" << newState;
}

void MainWindow::handleSaveRequested()
{
    m_config.setMqttBroker(m_settingsPage->broker());
    m_config.setMqttPort(m_settingsPage->port());

    if (m_config.saveToFile("config.json"))
    {
        statusBar()->showMessage(tr("설정이 저장되었습니다."), 5000);
    }
    else
    {
        statusBar()->showMessage(tr("설정 저장에 실패했습니다."), 5000);
        qWarning() << "설정 저장 실패";
    }
}

void MainWindow::handleSettingsBackRequested()
{
    switchToPage(PAGE_MAIN_MENU);
}

void MainWindow::handleSensitivityChanged(double value)
{
    if (m_airMouseManager)
    {
        m_airMouseManager->setSensitivity(value);
    }
}

void MainWindow::handleSmoothingChanged(bool checked)
{
    if (m_airMouseManager)
    {
        m_airMouseManager->setSmoothing(checked);
    }
}

void MainWindow::handleTrailChanged(bool checked)
{
    if (m_airMouseManager)
    {
        m_airMouseManager->setShowCursor(checked);
    }
}

// ============================================================================
// Workout Page Handlers
// ============================================================================

void MainWindow::handleWorkoutStartRequested()
{
    if (m_isWorkoutRunning)
    {
        return;
    }

    m_isWorkoutRunning = true;
    m_workoutSeconds = 0;
    m_workoutTimer->start(1000);

    // 스트리밍 재개를 위해 모드 선택을 다시 전달
    sendModeSelectCommand(m_currentMode);
    resetPoseSuccessState();
    schedulePoseAnalysis(m_currentPoseIndex);

    QJsonObject json;
    json["command"] = "start";
    json["mode"] = m_currentMode;
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtCmdStart(), doc.toJson(QJsonDocument::Compact));

    sendAirMouseModeCommand();

    if (m_workoutPage)
    {
        if (m_currentMode == "squat")
        {
            updateFeedbackLabel(squatInstructionText(m_currentPoseIndex), QString(), false);
            m_manualFeedbackActive = true;
        }
        else
        {
            updateFeedbackLabel(tr("운동을 시작합니다..."), QString(), false);
            m_manualFeedbackActive = false;
        }
    }
    m_lastServerFeedback.clear();
    qDebug() << "→ Workout started:" << m_currentExercise << "(" << m_currentMode << ")";
}

void MainWindow::handleWorkoutStopRequested()
{
    stopWorkout();
}

void MainWindow::handleWorkoutBackRequested()
{
    if (m_isWorkoutRunning)
    {
        stopWorkout();
    }

    sendSensorModeCommand();
    switchToPage(PAGE_EXERCISE_SELECTION);
}

// ============================================================================
// MQTT Slots
// ============================================================================

void MainWindow::onMqttConnected()
{
    qDebug() << "Connected to MQTT broker";
    if (m_mqttReconnectTimer)
    {
        m_mqttReconnectTimer->stop();
    }
    m_userRequestedDisconnect = false;
    updateMqttConnectionStatus(true);
    subscribeToTopics();
}

void MainWindow::onMqttDisconnected()
{
    qDebug() << "Disconnected from MQTT broker";
    updateMqttConnectionStatus(false);
    clearVideoFrame(tr("MQTT 연결이 끊어졌습니다."));
    scheduleMqttReconnect();
}

void MainWindow::onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString topicStr = topic.name();
    if (!topicStr.startsWith("qt/response/frame"))
    {
        QString messageStr = QString::fromUtf8(message);
        qDebug() << "Received from" << topicStr << ":" << messageStr;
    }

    // Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(message);
    if (doc.isNull() || !doc.isObject())
    {
        return;
    }

    QJsonObject json = doc.object();

    // WatchTower 응답 토픽 처리
    if (topicStr.startsWith("qt/response/"))
    {
        // WatchTower responses (mode_selected, analysis, error, status)
        // Extract response type from topic: qt/response/mode_selected → mode_selected
        QStringList parts = topicStr.split("/");
        if (parts.size() >= 3)
        {
            QString responseType = parts.last();
            handleQtResponse(responseType, json);
        }
    }
    else
    {
        qDebug() << "Unhandled topic (ignored):" << topicStr;
    }
}

void MainWindow::onMqttStateChanged(QMqttClient::ClientState state)
{
    QString stateStr;
    switch (state)
    {
    case QMqttClient::Disconnected:
        stateStr = "Disconnected";
        break;
    case QMqttClient::Connecting:
        stateStr = "Connecting";
        break;
    case QMqttClient::Connected:
        stateStr = "Connected";
        break;
    default:
        stateStr = "Unknown";
        break;
    }
    qDebug() << "MQTT State:" << stateStr;
}

void MainWindow::onMqttError(QMqttClient::ClientError error)
{
    if (error != QMqttClient::NoError)
    {
        qDebug() << "MQTT Error:" << error;
    }
    scheduleMqttReconnect();
}

// ============================================================================
// Helper Methods
// ============================================================================

void MainWindow::updateSensorData(const QJsonObject &data, bool isJoystick)
{
    if (isJoystick)
    {
        if (data.contains("status"))
        {
            qDebug() << "← Joystick status:" << data["status"].toString();
        }
        // 센서 모드 데이터는 향후 UI 확장 시 사용
    }
    else
    {
        // Watch heart rate
        int heartRate = -1;
        if (data.contains("heart_rate"))
        {
            heartRate = data["heart_rate"].toInt();
        }
        else if (data.contains("heartrate"))
        {
            heartRate = data["heartrate"].toInt();
        }

        if (heartRate >= 0)
        {
            if (m_workoutPage)
            {
                m_workoutPage->setHeartRate(heartRate);
            }
        }
    }
}

void MainWindow::updateAirMouseData(const QJsonObject &data)
{
    // Update global airmouse manager
    if (m_airMouseManager && m_airMouseManager->isEnabled())
    {
        m_airMouseManager->handleMouseData(data);

        // 버튼 입력 처리 (왼쪽 클릭만)
        if (data.contains("button_pressed"))
        {
            bool buttonPressed = data["button_pressed"].toBool();

            // 버튼이 눌렸을 때만 클릭 시뮬레이션
            if (buttonPressed)
            {
                m_airMouseManager->simulateClick();
                qDebug() << "Button pressed - simulating click";
            }
        }
    }
}

void MainWindow::updateWorkoutFeedback(const QJsonObject &data)
{
    if (data.contains("score"))
    {
        int score = data["score"].toInt();
        if (m_workoutPage)
        {
            m_workoutPage->setScore(score);
        }
    }

    QString rawFeedback;
    if (data.contains("feedback"))
    {
        rawFeedback = data.value("feedback").toString().trimmed();
    }

    QString translatedFeedback = translateFeedbackText(rawFeedback);
    QString displayFeedback = translatedFeedback.isEmpty() ? rawFeedback : translatedFeedback;

    if (!displayFeedback.isEmpty())
    {
        m_lastServerFeedback = displayFeedback;
    }
    else
    {
        m_lastServerFeedback.clear();
    }

    if (!m_manualFeedbackActive && m_workoutPage && !displayFeedback.isEmpty())
    {
        updateFeedbackLabel(tr("피드백: %1").arg(displayFeedback), QStringLiteral("font-size: 18px;"), false);
    }

    QString analyzedPoseName = data.value("current_pose").toString();
    QString expectedPoseName = currentPoseName();

    if (!analyzedPoseName.isEmpty() && analyzedPoseName != m_lastAnalyzedPoseName)
    {
        m_lastAnalyzedPoseName = analyzedPoseName;
        m_poseSuccessCounter = 0;
    }

    bool poseMatches = analyzedPoseName.isEmpty() || expectedPoseName.isEmpty() || analyzedPoseName == expectedPoseName;
    int poseSuccessThreshold = (m_currentMode == "squat") ? kSquatPoseSuccessThreshold : kDefaultPoseSuccessThreshold;

    if (data.contains("is_correct"))
    {
        bool isCorrect = data["is_correct"].toBool();
        if (poseMatches && isCorrect)
        {
            m_poseSuccessCounter++;

            if (m_poseSuccessCounter >= poseSuccessThreshold)
            {
                if (m_currentMode == "squat")
                {
                    handleSquatPoseSuccess();
                }
                else
                {
                    setFeedbackBanner(tr("성공! 자세가 정확합니다."), true);

                    if (isLastPose())
                    {
                        m_repCount++;
                        if (m_workoutPage)
                        {
                            m_workoutPage->setRepCount(m_repCount);
                        }
                        qDebug() << "Rep completed! Total reps:" << m_repCount;

                        m_currentPoseIndex = 0;
                        updatePoseDisplay();
                    }
                    else
                    {
                        nextPose();
                    }
                }

                m_poseSuccessCounter = 0;
                if (m_currentMode != "squat")
                {
                    m_manualFeedbackActive = false;
                }
            }
        }
        else
        {
            m_poseSuccessCounter = 0;

            QString failureMessage;
            if (m_currentMode == "squat")
            {
                failureMessage = tr("실패! 다시 시도하세요.\n%1").arg(squatInstructionText(m_currentPoseIndex));
            }
            else
            {
                failureMessage = tr("실패! 다시 시도하세요.");
            }

            setFeedbackBanner(failureMessage, false);

            if (m_isWorkoutRunning)
            {
                schedulePoseAnalysis(m_currentPoseIndex);
            }
        }
    }
    
    // Update current pose information if provided
    if (data.contains("current_pose") && data.contains("pose_description"))
    {
        QString currentPose = data["current_pose"].toString();
        QString poseDescription = data["pose_description"].toString();
        qDebug() << "Current pose:" << currentPose << "-" << poseDescription;
    }
}

void MainWindow::displayVideoFrame(const QString &base64Frame)
{
    if (!m_videoWidget)
    {
        return;
    }

    QString payload = base64Frame;
    const int headerIndex = payload.indexOf(',');
    if (headerIndex != -1)
    {
        payload = payload.mid(headerIndex + 1);
    }

    const QByteArray frameBytes = QByteArray::fromBase64(payload.toUtf8());
    if (frameBytes.isEmpty())
    {
        m_videoWidget->clearFrame(tr("영상 디코딩 실패"));
        return;
    }

    m_videoWidget->setFrame(frameBytes);
}

void MainWindow::clearVideoFrame(const QString &message)
{
    if (m_videoWidget)
    {
        m_videoWidget->clearFrame(message);
    }
}

void MainWindow::startWorkout(const QString &exerciseName)
{
    // Save exercise name (Korean, for display)
    m_currentExercise = exerciseName;

    // Convert to English mode name for MQTT communication
    m_currentMode = convertExerciseNameToMode(exerciseName);

    // Update UI
    if (m_workoutPage)
    {
        m_workoutPage->prepareForExercise(exerciseName);
    }

    clearVideoFrame(tr("영상 신호 대기 중..."));

    qDebug() << "→ Exercise selected:" << exerciseName << "→" << m_currentMode;
    switchToPage(PAGE_WORKOUT);
}

void MainWindow::stopWorkout()
{
    if (!m_isWorkoutRunning || m_currentMode.isEmpty())
    {
        m_poseAnalysisPending = false;
        return;
    }

    m_isWorkoutRunning = false;
    m_workoutTimer->stop();
    resetPoseSuccessState();
    m_manualFeedbackActive = false;
    if (m_poseAnalysisTimer)
    {
        m_poseAnalysisTimer->stop();
    }
    m_poseAnalysisPending = false;

    // Send stop command to WatchTower
    QJsonObject json;
    json["command"] = "stop";
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtCmdStop(), doc.toJson(QJsonDocument::Compact));

    if (m_workoutPage)
    {
        updateFeedbackLabel(tr("운동이 중지되었습니다"), QString(), false);
    }
    qDebug() << "Workout stopped";
}

void MainWindow::onWorkoutTimerTimeout()
{
    m_workoutSeconds++;
    updateWorkoutTimer();
}

void MainWindow::updateWorkoutTimer()
{
    int minutes = m_workoutSeconds / 60;
    int seconds = m_workoutSeconds % 60;
    if (m_workoutPage)
    {
        const QString timeText = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
        m_workoutPage->setTimerText(timeText);
    }
}

void MainWindow::sendAirMouseModeCommand()
{
    QJsonObject json;
    json["command"] = "airmouse_mode";

    QJsonDocument doc(json);
    publishMessage(m_config.topicWatchtowerCmdJoystick(), doc.toJson(QJsonDocument::Compact));
}

void MainWindow::sendSensorModeCommand()
{
    QJsonObject json;
    json["command"] = "sensor_mode";

    QJsonDocument doc(json);
    publishMessage(m_config.topicWatchtowerCmdJoystick(), doc.toJson(QJsonDocument::Compact));
}

void MainWindow::sendPoseIndex(int poseIndex)
{
    if (m_config.topicQtPoseIndex().isEmpty() || m_currentMode.isEmpty())
    {
        return;
    }

    QJsonObject json;
    json["mode"] = m_currentMode;
    json["pose_index"] = poseIndex;
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtPoseIndex(), doc.toJson(QJsonDocument::Compact));
}

void MainWindow::schedulePoseAnalysis(int poseIndex, int delayMs)
{
    if (!m_isWorkoutRunning || m_currentMode.isEmpty())
    {
        m_poseAnalysisPending = false;
        return;
    }

    if (!m_poseAnalysisTimer)
    {
        return;
    }

    if (poseIndex < 0 || poseIndex >= m_totalPoses)
    {
        qWarning() << "schedulePoseAnalysis: invalid index" << poseIndex;
        m_poseAnalysisPending = false;
        return;
    }

    m_poseAnalysisTimer->stop();
    m_poseAnalysisTargetIndex = poseIndex;
    m_poseAnalysisPending = true;
    m_poseAnalysisTimer->start(delayMs);
    qDebug() << "Pose analysis scheduled for index" << poseIndex << "after" << delayMs << "ms";
}

void MainWindow::onPoseAnalysisTimeout()
{
    if (!m_isWorkoutRunning)
    {
        return;
    }

    if (!m_poseAnalysisPending)
    {
        return;
    }

    requestPoseAnalysis(m_poseAnalysisTargetIndex);
    m_poseAnalysisPending = false;
}

void MainWindow::requestPoseAnalysis(int poseIndex)
{
    if (m_currentMode.isEmpty())
    {
        return;
    }

    sendPoseIndex(poseIndex);

    if (m_config.topicQtRequestAnalysis().isEmpty())
    {
        return;
    }

    QJsonObject json;
    json["mode"] = m_currentMode;
    json["pose_index"] = poseIndex;
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtRequestAnalysis(), doc.toJson(QJsonDocument::Compact));
    qDebug() << "→ Requested analysis for pose index" << poseIndex;
}

// ============================================================================
// MQTT Protocol Helpers (WatchTower Compatible)
// ============================================================================

QString MainWindow::convertExerciseNameToMode(const QString &exerciseName)
{
    const auto &catalog = exerciseCatalog();
    for (const ExerciseOption &option : catalog)
    {
        if (option.displayName == exerciseName)
        {
            return option.mode;
        }
    }

    qWarning() << "Unknown exercise name:" << exerciseName;
    if (!catalog.isEmpty())
    {
        return catalog.first().mode;
    }

    return QStringLiteral("squat");
}

void MainWindow::sendModeSelectCommand(const QString &mode)
{
    /**
     * WatchTower에 운동 모드 선택 명령 전송
     * Topic: qt/command/select_mode
     * Format: {"mode": "squat", "timestamp": 1234567890}
     */
    QJsonObject json;
    json["mode"] = mode;
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtCmdSelect(), doc.toJson(QJsonDocument::Compact));

    qDebug() << "→ Mode select sent:" << mode;
}

void MainWindow::handleQtResponse(const QString &responseType, const QJsonObject &data)
{
    /**
     * WatchTower로부터 받은 응답 처리
     *
     * Response types:
     * - mode_selected: 모드 선택 확인
     * - analysis: 운동 분석 결과
     * - error: 에러 메시지
     * - status: 상태 업데이트
     * - frame: 카메라 영상 프레임 (base64)
     */
    qDebug() << "← Qt Response received:" << responseType;

    if (responseType == "mode_selected")
    {
        // 모드 선택 확인
        QString mode = data["mode"].toString();
        QString status = data["status"].toString();

        if (status == "success")
        {
            qDebug() << "✓ Mode selected successfully:" << mode;
            if (m_mainMenuPage)
            {
                m_mainMenuPage->setStatusText(tr("모드 선택됨: %1").arg(mode));
            }

            // Parse pose sequence information
            if (data.contains("poses") && data["poses"].isArray())
            {
                m_poses = data["poses"].toArray();
                m_totalPoses = m_poses.size();
                m_currentPoseIndex = 0;
                m_repCount = 0;

                qDebug() << "Loaded" << m_totalPoses << "poses for mode:" << mode;

                // Display first pose information
                updatePoseDisplay();
            }
        }
        else
        {
            qWarning() << "✗ Mode selection failed:" << mode;
        }
    }
    else if (responseType == "analysis")
    {
        if (data.contains("current_pose"))
        {
            const QString poseName = data.value("current_pose").toString();
            const bool detected = poseName != "" && poseName != "none";
            if (detected)
            {
                qDebug() << "人 detected" << poseName;
            }
            else
            {
                qDebug() << "人 not detected";
            }
        }

        // 운동 분석 결과 (실시간 피드백)
        updateWorkoutFeedback(data);
        if (data.contains("frame"))
        {
            displayVideoFrame(data.value("frame").toString());
        }
    }
    else if (responseType == "error")
    {
        // 에러 메시지
        QString errorMsg = data["message"].toString();
        qWarning() << "✗ WatchTower error:" << errorMsg;

        if (m_isWorkoutRunning)
        {
            if (m_workoutPage)
            {
                updateFeedbackLabel(tr("오류: %1").arg(errorMsg), QStringLiteral("color: red;"), false);
            }
        }
    }
    else if (responseType == "status")
    {
        // 상태 업데이트
        QString status = data["status"].toString();
        qDebug() << "ℹ WatchTower status:" << status;
    }
    else if (responseType == "joystick")
    {
        if (data.contains("mode") && data["mode"].toString() == "airmouse")
        {
            updateAirMouseData(data);
        }
        else
        {
            updateSensorData(data, true);
        }
    }
    else if (responseType == "watch")
    {
        updateSensorData(data, false);
    }
    else if (responseType == "frame")
    {
        if (data.contains("frame"))
        {
            displayVideoFrame(data.value("frame").toString());
        }
        else
        {
            clearVideoFrame(tr("영상 데이터 없음"));
        }
    }
}

// ============================================================================
// Pose Sequence Helper Methods
// ============================================================================

void MainWindow::updatePoseDisplay()
{
    /**
     * 현재 포즈 정보를 UI에 표시
     * WatchTower에 pose_index 업데이트도 함께 전송할 수 있음
     */
    if (m_currentPoseIndex < 0 || m_currentPoseIndex >= m_totalPoses)
    {
        qWarning() << "Invalid pose index:" << m_currentPoseIndex;
        return;
    }

    resetPoseSuccessState();
    m_manualFeedbackActive = false;

    QJsonObject currentPose = m_poses[m_currentPoseIndex].toObject();
    const QString poseDescription = currentPose["description"].toString();

    // UI에 현재 포즈 정보 표시 (feedbackLabel 위에 추가로 표시)
    if (m_workoutPage)
    {
        m_workoutPage->setExerciseProgress(m_currentExercise, m_currentPoseIndex + 1, m_totalPoses);

        if (m_currentMode == "squat")
        {
            const QString currentFeedback = m_workoutPage->feedbackText();
            if (currentFeedback.isEmpty() || currentFeedback == tr("시작 버튼을 눌러주세요"))
            {
                updateFeedbackLabel(poseDescription, QString(), false);
            }
        }
        else
        {
            updateFeedbackLabel(poseDescription, QString(), false);
            m_manualFeedbackActive = false;
        }
    }

    qDebug() << "Pose" << (m_currentPoseIndex + 1) << "/" << m_totalPoses << ":" << poseDescription;

    if (m_isWorkoutRunning)
    {
        schedulePoseAnalysis(m_currentPoseIndex);
    }
}

void MainWindow::nextPose()
{
    /**
     * 다음 포즈로 이동
     * 마지막 포즈가 아닐 때만 증가
     */
    if (m_currentPoseIndex < m_totalPoses - 1)
    {
        m_currentPoseIndex++;
        updatePoseDisplay();
        qDebug() << "Moving to next pose:" << (m_currentPoseIndex + 1) << "/" << m_totalPoses;
    }
    else
    {
        qDebug() << "Already at last pose";
    }
}

bool MainWindow::isLastPose() const
{
    /**
     * 현재 포즈가 마지막 포즈인지 확인
     */
    return m_currentPoseIndex == m_totalPoses - 1;
}

void MainWindow::handleSquatPoseSuccess()
{
    if (m_currentPoseIndex == 0)
    {
        setFeedbackBanner(tr("성공! 이제 스쿼트 내려가기 자세를 취하세요."), true);

        m_currentPoseIndex = 1;
        updatePoseDisplay();
        sendPoseIndex(m_currentPoseIndex);
    }
    else if (m_currentPoseIndex == 1)
    {
        m_repCount++;
        if (m_workoutPage)
        {
            m_workoutPage->setRepCount(m_repCount);
        }
        setFeedbackBanner(tr("성공! 다시 서 있는 자세로 돌아오세요."), true);

        m_currentPoseIndex = 0;
        updatePoseDisplay();
        sendPoseIndex(m_currentPoseIndex);
    }
}

void MainWindow::resetPoseSuccessState()
{
    m_poseSuccessCounter = 0;
    m_lastAnalyzedPoseName.clear();
}

QString MainWindow::currentPoseName() const
{
    if (m_currentPoseIndex < 0 || m_currentPoseIndex >= m_totalPoses)
    {
        return QString();
    }

    if (m_currentPoseIndex < m_poses.size())
    {
        return m_poses[m_currentPoseIndex].toObject().value("name").toString();
    }

    return QString();
}

QString MainWindow::squatInstructionText(int poseIndex) const
{
    if (poseIndex == 0)
    {
        return tr("서 있는 자세를 유지하세요.");
    }
    if (poseIndex == 1)
    {
        return tr("스쿼트 내려가기 자세를 유지하세요.");
    }
    return tr("자세를 유지하세요.");
}

void MainWindow::setFeedbackBanner(const QString &message, bool success)
{
    if (!m_workoutPage)
    {
        return;
    }

    const QString style = success
        ? QStringLiteral("font-size: 32px; font-weight: bold; color: #2ecc71;")
        : QStringLiteral("font-size: 32px; font-weight: bold; color: #e74c3c;");

    updateFeedbackLabel(message, style, true);
    m_manualFeedbackActive = true;
}

void MainWindow::updateFeedbackLabel(const QString &baseMessage, const QString &styleSheet, bool includeServerFeedback)
{
    if (!m_workoutPage)
    {
        return;
    }

    const QString finalMessage = composeFeedbackMessage(baseMessage, includeServerFeedback);
    m_workoutPage->setFeedbackMessage(finalMessage, styleSheet);
}

QString MainWindow::composeFeedbackMessage(const QString &baseMessage, bool includeServerFeedback) const
{
    if (!includeServerFeedback || m_lastServerFeedback.isEmpty())
    {
        return baseMessage;
    }

    if (baseMessage.isEmpty())
    {
        return tr("(서버 피드백: %1)").arg(m_lastServerFeedback);
    }

    const QString marker = tr("서버 피드백");
    if (baseMessage.contains(marker) || baseMessage.contains(m_lastServerFeedback))
    {
        return baseMessage;
    }

    return baseMessage + tr("\n(서버 피드백: %1)").arg(m_lastServerFeedback);
}

QString MainWindow::translateFeedbackText(const QString &feedback) const
{
    QString text = feedback;
    if (text.isEmpty())
    {
        return text;
    }

    text.replace("Left knee", tr("왼쪽 무릎"));
    text.replace("Right knee", tr("오른쪽 무릎"));
    text.replace("Left leg", tr("왼쪽 다리"));
    text.replace("Right leg", tr("오른쪽 다리"));
    text.replace("Straighten left leg", tr("왼쪽 다리를 곧게 펴세요"));
    text.replace("Straighten right leg", tr("오른쪽 다리를 곧게 펴세요"));
    text.replace("bend more", tr("조금 더 굽히세요"));
    text.replace("bend deeper", tr("조금 더 깊게 내려가세요"));
    text.replace("too deep", tr("조금 덜 굽히세요"));
    text.replace("Lower your hips closer to your heels", tr("엉덩이를 더 아래로 내려주세요"));
    text.replace("Bend knees deeper", tr("무릎을 더 굽히세요"));
    text.replace("knees should drop lower", tr("무릎을 더 낮춰 주세요"));
    text.replace("Keep lowering your hips", tr("엉덩이를 더 낮춰 주세요"));
    text.replace("Rotate sideways or move back for better detection", tr("측면이 보이도록 서거나 카메라에서 조금 떨어져 주세요"));
    text.replace("Ready position complete!", tr("준비 자세 완료!"));
    text.replace("Great squat depth!", tr("좋은 스쿼트 깊이예요!"));
    text.replace("Perfect squat!", tr("완벽한 스쿼트입니다!"));
    text.replace("Stand in front of camera (full body)", tr("카메라 앞에서 전신이 보이게 서 주세요"));
    text.replace("No person detected", tr("사람이 감지되지 않았습니다"));
    text.replace("Angle calculation failed", tr("각도 계산에 실패했습니다"));

    return text.trimmed();
}
