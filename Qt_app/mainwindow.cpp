#include "mainwindow.h"
#include "ui_main.h"
#include "ui_exercise_selection.h"
#include "ui_settings.h"
#include "ui_workout.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QScrollBar>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stackedWidget(nullptr)
    , m_mainMenuPage(nullptr)
    , m_exerciseSelectionPage(nullptr)
    , m_settingsPage(nullptr)
    , m_workoutPage(nullptr)
    , ui_mainMenu(nullptr)
    , ui_exerciseSelection(nullptr)
    , ui_settings(nullptr)
    , ui_workout(nullptr)
    , m_client(nullptr)
    , m_config(Config::instance())
    , m_videoWidget(nullptr)
    , m_airMouseManager(nullptr)
    , m_currentExercise("")
    , m_currentMode("")
    , m_workoutTimer(nullptr)
    , m_workoutSeconds(0)
    , m_isWorkoutRunning(false)
    , m_currentPoseIndex(0)
    , m_totalPoses(0)
    , m_repCount(0)
{
    loadConfiguration();
    setupPages();
    setupMqttClient();
    setupVideoWidget();
    setupAirMouse();

    // Start with main menu
    switchToPage(PAGE_MAIN_MENU);

    // Set window properties
    setWindowTitle("홈 트레이닝 시스템");
    resize(m_config.windowWidth(), m_config.windowHeight());
}

MainWindow::~MainWindow()
{
    if (m_client) {
        m_client->disconnectFromHost();
    }

    delete ui_mainMenu;
    delete ui_exerciseSelection;
    delete ui_settings;
    delete ui_workout;
}

void MainWindow::loadConfiguration()
{
    if (!m_config.loadFromFile("config.json")) {
        qDebug() << "Failed to load config.json, using defaults";
    } else {
        qDebug() << "Configuration loaded successfully";
    }
}

void MainWindow::setupPages()
{
    // Create stacked widget
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Create Main Menu Page
    m_mainMenuPage = new QWidget();
    ui_mainMenu = new Ui::MainMenuPage();
    ui_mainMenu->setupUi(m_mainMenuPage);
    m_stackedWidget->addWidget(m_mainMenuPage);

    // Connect main menu signals
    connect(ui_mainMenu->exerciseSelectButton, &QPushButton::clicked,
            this, &MainWindow::on_exerciseSelectButton_clicked);
    connect(ui_mainMenu->settingsButton, &QPushButton::clicked,
            this, &MainWindow::on_settingsButton_clicked);

    // Create Exercise Selection Page
    m_exerciseSelectionPage = new QWidget();
    ui_exerciseSelection = new Ui::ExerciseSelectionPage();
    ui_exerciseSelection->setupUi(m_exerciseSelectionPage);
    m_stackedWidget->addWidget(m_exerciseSelectionPage);

    // Connect exercise selection signals
    connect(ui_exerciseSelection->tPoseButton, &QPushButton::clicked,
            this, &MainWindow::on_tPoseButton_clicked);
    connect(ui_exerciseSelection->squatButton, &QPushButton::clicked,
            this, &MainWindow::on_squatButton_clicked);
    connect(ui_exerciseSelection->pushupButton, &QPushButton::clicked,
            this, &MainWindow::on_pushupButton_clicked);
    connect(ui_exerciseSelection->plankButton, &QPushButton::clicked,
            this, &MainWindow::on_plankButton_clicked);
    connect(ui_exerciseSelection->lungeButton, &QPushButton::clicked,
            this, &MainWindow::on_lungeButton_clicked);
    connect(ui_exerciseSelection->jumpingJackButton, &QPushButton::clicked,
            this, &MainWindow::on_jumpingJackButton_clicked);
    connect(ui_exerciseSelection->mountainClimberButton, &QPushButton::clicked,
            this, &MainWindow::on_mountainClimberButton_clicked);
    connect(ui_exerciseSelection->burpeeButton, &QPushButton::clicked,
            this, &MainWindow::on_burpeeButton_clicked);
    connect(ui_exerciseSelection->customButton, &QPushButton::clicked,
            this, &MainWindow::on_customButton_clicked);
    connect(ui_exerciseSelection->scrollUpButton, &QPushButton::clicked,
            this, &MainWindow::on_scrollUpButton_clicked);
    connect(ui_exerciseSelection->scrollDownButton, &QPushButton::clicked,
            this, &MainWindow::on_scrollDownButton_clicked);
    connect(ui_exerciseSelection->backButton, &QPushButton::clicked,
            this, &MainWindow::on_exerciseSelection_backButton_clicked);

    // Create Settings Page
    m_settingsPage = new QWidget();
    ui_settings = new Ui::SettingsPage();
    ui_settings->setupUi(m_settingsPage);
    m_stackedWidget->addWidget(m_settingsPage);

    // Connect settings signals
    connect(ui_settings->connectButton, &QPushButton::clicked,
            this, &MainWindow::on_settings_connectButton_clicked);
    connect(ui_settings->disconnectButton, &QPushButton::clicked,
            this, &MainWindow::on_settings_disconnectButton_clicked);
    connect(ui_settings->calibrateButton, &QPushButton::clicked,
            this, &MainWindow::on_settings_calibrateButton_clicked);
    connect(ui_settings->testAirMouseButton, &QPushButton::clicked,
            this, &MainWindow::on_settings_testAirMouseButton_clicked);
    connect(ui_settings->saveButton, &QPushButton::clicked,
            this, &MainWindow::on_settings_saveButton_clicked);
    connect(ui_settings->backButton, &QPushButton::clicked,
            this, &MainWindow::on_settings_backButton_clicked);
    connect(ui_settings->sensitivitySlider, &QSlider::valueChanged,
            this, &MainWindow::on_sensitivitySlider_valueChanged);
    connect(ui_settings->smoothingCheckBox, &QCheckBox::toggled,
            this, &MainWindow::on_smoothingCheckBox_toggled);
    connect(ui_settings->trailCheckBox, &QCheckBox::toggled,
            this, &MainWindow::on_trailCheckBox_toggled);

    // Create Workout Page
    m_workoutPage = new QWidget();
    ui_workout = new Ui::WorkoutPage();
    ui_workout->setupUi(m_workoutPage);
    m_stackedWidget->addWidget(m_workoutPage);

    // Connect workout signals
    connect(ui_workout->startButton, &QPushButton::clicked,
            this, &MainWindow::on_workout_startButton_clicked);
    connect(ui_workout->stopButton, &QPushButton::clicked,
            this, &MainWindow::on_workout_stopButton_clicked);
    connect(ui_workout->backButton, &QPushButton::clicked,
            this, &MainWindow::on_workout_backButton_clicked);

    // Initialize workout timer
    m_workoutTimer = new QTimer(this);
    connect(m_workoutTimer, &QTimer::timeout, this, &MainWindow::onWorkoutTimerTimeout);

    // Apply settings from config
    ui_settings->brokerLineEdit->setText(m_config.mqttBroker());
    ui_settings->portSpinBox->setValue(m_config.mqttPort());

    qDebug() << "All pages setup complete";
}

void MainWindow::setupMqttClient()
{
    m_client = new QMqttClient(this);
    m_client->setHostname(m_config.mqttBroker());
    m_client->setPort(m_config.mqttPort());

    if (!m_config.mqttClientId().isEmpty()) {
        m_client->setClientId(m_config.mqttClientId());
    }

    if (!m_config.mqttUsername().isEmpty()) {
        m_client->setUsername(m_config.mqttUsername());
    }

    if (!m_config.mqttPassword().isEmpty()) {
        m_client->setPassword(m_config.mqttPassword());
    }

    connect(m_client, &QMqttClient::connected, this, &MainWindow::onMqttConnected);
    connect(m_client, &QMqttClient::disconnected, this, &MainWindow::onMqttDisconnected);
    connect(m_client, &QMqttClient::messageReceived, this, &MainWindow::onMqttMessageReceived);
    connect(m_client, &QMqttClient::stateChanged, this, &MainWindow::onMqttStateChanged);
    connect(m_client, &QMqttClient::errorChanged, this, &MainWindow::onMqttError);

    qDebug() << "MQTT client setup complete";
}

void MainWindow::setupVideoWidget()
{
    if (!ui_workout) {
        return;
    }

    m_videoWidget = ui_workout->videoWidget;
    if (m_videoWidget) {
        m_videoWidget->clearFrame(tr("영상 신호 대기 중..."));
    }

    qDebug() << "Video widget initialized";
}

void MainWindow::setupAirMouse()
{
    // Create global airmouse manager
    m_airMouseManager = new AirMouseManager(this);

    // Set default settings
    m_airMouseManager->setSensitivity(1.5);
    m_airMouseManager->setSmoothing(true);
    m_airMouseManager->setShowCursor(true);

    // Initially disabled
    m_airMouseManager->setEnabled(false);

    qDebug() << "AirMouse manager initialized";
}

void MainWindow::switchToPage(Page page)
{
    m_stackedWidget->setCurrentIndex(static_cast<int>(page));
    qDebug() << "Switched to page:" << page;
}

void MainWindow::subscribeToTopics()
{
    // Subscribe to joystick topics
    auto sub1 = m_client->subscribe(m_config.topicJoystickData());
    if (!sub1) {
        qDebug() << "Failed to subscribe to" << m_config.topicJoystickData();
    }

    auto sub2 = m_client->subscribe(m_config.topicJoystickStatus());
    if (!sub2) {
        qDebug() << "Failed to subscribe to" << m_config.topicJoystickStatus();
    }

    // Subscribe to watch topics
    auto sub3 = m_client->subscribe(m_config.topicWatchHeartrate());
    if (!sub3) {
        qDebug() << "Failed to subscribe to" << m_config.topicWatchHeartrate();
    }

    auto sub4 = m_client->subscribe(m_config.topicWatchStatus());
    if (!sub4) {
        qDebug() << "Failed to subscribe to" << m_config.topicWatchStatus();
    }

    // Subscribe to Qt response topics
    auto sub5 = m_client->subscribe(m_config.topicQtResponse());
    if (!sub5) {
        qDebug() << "Failed to subscribe to" << m_config.topicQtResponse();
    }

    qDebug() << "Subscribed to all topics";
}

void MainWindow::publishMessage(const QString &topic, const QString &message)
{
    if (m_client->state() != QMqttClient::Connected) {
        qDebug() << "Not connected to MQTT broker";
        return;
    }

    qint32 id = m_client->publish(topic, message.toUtf8(), 0);
    if (id == -1) {
        qDebug() << "Failed to publish to" << topic;
    } else {
        qDebug() << "Published to" << topic << ":" << message;
    }
}

void MainWindow::updateMqttConnectionStatus(bool connected)
{
    if (connected) {
        ui_settings->statusValueLabel->setText("<span style='color:green;'>연결됨</span>");
        ui_mainMenu->statusLabel->setText("상태: MQTT 연결됨");
    } else {
        ui_settings->statusValueLabel->setText("<span style='color:red;'>연결 안됨</span>");
        ui_mainMenu->statusLabel->setText("상태: MQTT 연결 안됨");
    }
}

// ============================================================================
// Main Menu Page Slots
// ============================================================================

void MainWindow::on_exerciseSelectButton_clicked()
{
    switchToPage(PAGE_EXERCISE_SELECTION);
}

void MainWindow::on_settingsButton_clicked()
{
    switchToPage(PAGE_SETTINGS);
}

// ============================================================================
// Exercise Selection Page Slots
// ============================================================================

void MainWindow::on_tPoseButton_clicked()
{
    startWorkout("스쿼트");  // 첫 번째 버튼 → 스쿼트
}

void MainWindow::on_squatButton_clicked()
{
    startWorkout("푸시업");  // 두 번째 버튼 → 푸시업
}

void MainWindow::on_pushupButton_clicked()
{
    QMessageBox::information(this, "플랭크", "플랭크 운동은 곧 출시됩니다.");
}

void MainWindow::on_plankButton_clicked()
{
    QMessageBox::information(this, "런지", "런지 운동은 곧 출시됩니다.");
}

void MainWindow::on_lungeButton_clicked()
{
    QMessageBox::information(this, "점핑잭", "점핑잭 운동은 곧 출시됩니다.");
}

void MainWindow::on_jumpingJackButton_clicked()
{
    QMessageBox::information(this, "마운틴 클라이머", "마운틴 클라이머 운동은 곧 출시됩니다.");
}

void MainWindow::on_mountainClimberButton_clicked()
{
    QMessageBox::information(this, "버피", "버피 운동은 곧 출시됩니다.");
}

void MainWindow::on_burpeeButton_clicked()
{
    QMessageBox::information(this, "사용자 정의 1", "사용자 정의 운동 기능은 곧 출시됩니다.");
}

void MainWindow::on_customButton_clicked()
{
    QMessageBox::information(this, "사용자 정의 2", "사용자 정의 운동 기능은 곧 출시됩니다.");
}

void MainWindow::on_scrollUpButton_clicked()
{
    // Scroll up by 100 pixels
    QScrollArea *scrollArea = ui_exerciseSelection->exerciseScrollArea;
    QScrollBar *scrollBar = scrollArea->verticalScrollBar();
    int currentValue = scrollBar->value();
    scrollBar->setValue(currentValue - 100);
}

void MainWindow::on_scrollDownButton_clicked()
{
    // Scroll down by 100 pixels
    QScrollArea *scrollArea = ui_exerciseSelection->exerciseScrollArea;
    QScrollBar *scrollBar = scrollArea->verticalScrollBar();
    int currentValue = scrollBar->value();
    scrollBar->setValue(currentValue + 100);
}

void MainWindow::on_exerciseSelection_backButton_clicked()
{
    switchToPage(PAGE_MAIN_MENU);
}

// ============================================================================
// Settings Page Slots
// ============================================================================

void MainWindow::on_settings_connectButton_clicked()
{
    m_client->setHostname(ui_settings->brokerLineEdit->text());
    m_client->setPort(ui_settings->portSpinBox->value());
    m_client->connectToHost();
    qDebug() << "Connecting to broker...";
}

void MainWindow::on_settings_disconnectButton_clicked()
{
    m_client->disconnectFromHost();
    qDebug() << "Disconnecting from broker...";
}

void MainWindow::on_settings_calibrateButton_clicked()
{
    QJsonObject json;
    json["command"] = "calibrate";

    QJsonDocument doc(json);
    publishMessage(m_config.topicWatchtowerCmdJoystick(), doc.toJson(QJsonDocument::Compact));

    QMessageBox::information(this, "캘리브레이션", "조이스틱 캘리브레이션 명령을 전송했습니다.");
}

void MainWindow::on_settings_testAirMouseButton_clicked()
{
    // Toggle global airmouse mode
    if (m_airMouseManager) {
        bool isEnabled = m_airMouseManager->isEnabled();
        m_airMouseManager->setEnabled(!isEnabled);

        if (!isEnabled) {
            // 에어마우스 활성화
            sendAirMouseModeCommand();
            QMessageBox::information(this, "에어마우스",
                "에어마우스가 활성화되었습니다.\n"
                "조이스틱을 움직여 모든 페이지의 UI를 제어할 수 있습니다.\n"
                "다시 클릭하면 비활성화됩니다.");
        } else {
            // 에어마우스 비활성화
            sendSensorModeCommand();
            QMessageBox::information(this, "에어마우스", "에어마우스가 비활성화되었습니다.");
        }
    }
}

void MainWindow::on_settings_saveButton_clicked()
{
    // Save config
    m_config.setMqttBroker(ui_settings->brokerLineEdit->text());
    m_config.setMqttPort(ui_settings->portSpinBox->value());

    if (m_config.saveToFile("config.json")) {
        QMessageBox::information(this, "저장 완료", "설정이 저장되었습니다.");
    } else {
        QMessageBox::warning(this, "저장 실패", "설정 저장에 실패했습니다.");
    }
}

void MainWindow::on_settings_backButton_clicked()
{
    switchToPage(PAGE_MAIN_MENU);
}

void MainWindow::on_sensitivitySlider_valueChanged(int value)
{
    double sensitivity = value / 10.0;
    if (m_airMouseManager) {
        m_airMouseManager->setSensitivity(sensitivity);
    }
    ui_settings->sensitivityValueLabel->setText(QString("%1x").arg(sensitivity, 0, 'f', 1));
}

void MainWindow::on_smoothingCheckBox_toggled(bool checked)
{
    if (m_airMouseManager) {
        m_airMouseManager->setSmoothing(checked);
    }
}

void MainWindow::on_trailCheckBox_toggled(bool checked)
{
    if (m_airMouseManager) {
        m_airMouseManager->setShowCursor(checked);
    }
}

// ============================================================================
// Workout Page Slots
// ============================================================================

void MainWindow::on_workout_startButton_clicked()
{
    if (m_isWorkoutRunning) {
        return;
    }

    m_isWorkoutRunning = true;
    m_workoutSeconds = 0;
    m_workoutTimer->start(1000); // 1 second interval

    // Send start command to WatchTower (WatchTower protocol)
    // Topic: qt/command/start
    // Format: {"command": "start", "mode": "t_pose", "timestamp": 1234567890}
    QJsonObject json;
    json["command"] = "start";
    json["mode"] = m_currentMode;  // Use converted English mode name
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtCmdStart(), doc.toJson(QJsonDocument::Compact));

    // Enable airmouse mode for cursor control
    sendAirMouseModeCommand();

    ui_workout->feedbackLabel->setText("운동을 시작합니다...");
    ui_workout->feedbackLabel->setStyleSheet("");  // Reset style
    qDebug() << "→ Workout started:" << m_currentExercise << "(" << m_currentMode << ")";
}

void MainWindow::on_workout_stopButton_clicked()
{
    stopWorkout();
}

void MainWindow::on_workout_backButton_clicked()
{
    if (m_isWorkoutRunning) {
        stopWorkout();
    }

    // Switch back to sensor mode
    sendSensorModeCommand();

    switchToPage(PAGE_EXERCISE_SELECTION);
}

// ============================================================================
// MQTT Slots
// ============================================================================

void MainWindow::onMqttConnected()
{
    qDebug() << "Connected to MQTT broker";
    updateMqttConnectionStatus(true);
    subscribeToTopics();
}

void MainWindow::onMqttDisconnected()
{
    qDebug() << "Disconnected from MQTT broker";
    updateMqttConnectionStatus(false);
    clearVideoFrame(tr("MQTT 연결이 끊어졌습니다."));
}

void MainWindow::onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString topicStr = topic.name();
    QString messageStr = QString::fromUtf8(message);

    qDebug() << "Received from" << topicStr << ":" << messageStr;

    // Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(message);
    if (doc.isNull() || !doc.isObject()) {
        return;
    }

    QJsonObject json = doc.object();

    // Update UI based on topic
    if (topicStr == m_config.topicJoystickData()) {
        // Joystick sensor/airmouse data
        if (json.contains("mode") && json["mode"].toString() == "airmouse") {
            updateAirMouseData(json);
        } else {
            updateSensorData(json, true);
        }

    } else if (topicStr == m_config.topicWatchHeartrate()) {
        // Watch heart rate data
        updateSensorData(json, false);

    } else if (topicStr == m_config.topicJoystickStatus()) {
        // Joystick status updates
        QString status = json["status"].toString();
        qDebug() << "← Joystick status:" << status;

    } else if (topicStr == m_config.topicWatchStatus()) {
        // Watch status updates
        QString status = json["status"].toString();
        qDebug() << "← Watch status:" << status;

    } else if (topicStr.startsWith("qt/response/")) {
        // WatchTower responses (mode_selected, analysis, error, status)
        // Extract response type from topic: qt/response/mode_selected → mode_selected
        QStringList parts = topicStr.split("/");
        if (parts.size() >= 3) {
            QString responseType = parts.last();
            handleQtResponse(responseType, json);
        }
    }
}

void MainWindow::onMqttStateChanged(QMqttClient::ClientState state)
{
    QString stateStr;
    switch (state) {
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
    if (error != QMqttClient::NoError) {
        qDebug() << "MQTT Error:" << error;
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

void MainWindow::updateSensorData(const QJsonObject &data, bool isJoystick)
{
    if (isJoystick) {
        // Sensor mode data (if needed in future)
    } else {
        // Watch heart rate
        if (data.contains("heartrate")) {
            int heartRate = data["heartrate"].toInt();
            ui_workout->heartRateLabel->setText(QString("심박수: %1 BPM").arg(heartRate));
        }
    }
}

void MainWindow::updateAirMouseData(const QJsonObject &data)
{
    // Update global airmouse manager
    if (m_airMouseManager && m_airMouseManager->isEnabled()) {
        m_airMouseManager->handleMouseData(data);
    }
}

void MainWindow::updateWorkoutFeedback(const QJsonObject &data)
{
    if (data.contains("score")) {
        int score = data["score"].toInt();
        ui_workout->scoreLabel->setText(QString("점수: %1").arg(score));
    }

    if (data.contains("feedback")) {
        QString feedback = data["feedback"].toString();
        ui_workout->feedbackLabel->setText(QString("피드백: %1").arg(feedback));
    }

    if (data.contains("is_correct")) {
        bool isCorrect = data["is_correct"].toBool();
        if (isCorrect) {
            ui_workout->feedbackLabel->setStyleSheet("color: green;");

            // If pose is correct and this is the last pose, increment rep count and reset
            if (isLastPose()) {
                m_repCount++;
                ui_workout->repCountLabel->setText(QString("반복 횟수: %1").arg(m_repCount));
                qDebug() << "Rep completed! Total reps:" << m_repCount;

                // Reset to first pose for next repetition
                m_currentPoseIndex = 0;
                updatePoseDisplay();
            } else {
                // Move to next pose
                nextPose();
            }
        } else {
            ui_workout->feedbackLabel->setStyleSheet("color: orange;");
        }
    }

    // Update current pose information if provided
    if (data.contains("current_pose") && data.contains("pose_description")) {
        QString currentPose = data["current_pose"].toString();
        QString poseDescription = data["pose_description"].toString();
        qDebug() << "Current pose:" << currentPose << "-" << poseDescription;
    }
}

void MainWindow::displayVideoFrame(const QString &base64Frame)
{
    if (!m_videoWidget) {
        return;
    }

    QString payload = base64Frame;
    const int headerIndex = payload.indexOf(',');
    if (headerIndex != -1) {
        payload = payload.mid(headerIndex + 1);
    }

    const QByteArray frameBytes = QByteArray::fromBase64(payload.toUtf8());
    if (frameBytes.isEmpty()) {
        m_videoWidget->clearFrame(tr("영상 디코딩 실패"));
        return;
    }

    m_videoWidget->setFrame(frameBytes);
}

void MainWindow::clearVideoFrame(const QString &message)
{
    if (m_videoWidget) {
        m_videoWidget->clearFrame(message);
    }
}

void MainWindow::startWorkout(const QString &exerciseName)
{
    // Save exercise name (Korean, for display)
    m_currentExercise = exerciseName;

    // Convert to English mode name for MQTT communication
    m_currentMode = convertExerciseNameToMode(exerciseName);

    // Send mode selection command to WatchTower first
    // Topic: qt/command/select_mode
    // Format: {"mode": "t_pose", "timestamp": 1234567890}
    sendModeSelectCommand(m_currentMode);

    // Update UI
    ui_workout->exerciseTitleLabel->setText(QString("운동: %1").arg(exerciseName));
    ui_workout->scoreLabel->setText("점수: --");
    ui_workout->feedbackLabel->setText("시작 버튼을 눌러주세요");
    ui_workout->feedbackLabel->setStyleSheet("");
    ui_workout->heartRateLabel->setText("심박수: -- BPM");
    ui_workout->repCountLabel->setText("반복 횟수: 0");
    ui_workout->timerLabel->setText("00:00");

    clearVideoFrame(tr("영상 신호 대기 중..."));

    qDebug() << "→ Exercise selected:" << exerciseName << "→" << m_currentMode;
    switchToPage(PAGE_WORKOUT);
}

void MainWindow::stopWorkout()
{
    if (!m_isWorkoutRunning) {
        return;
    }

    m_isWorkoutRunning = false;
    m_workoutTimer->stop();

    // Send stop command to WatchTower
    QJsonObject json;
    json["command"] = "stop";
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtCmdStop(), doc.toJson(QJsonDocument::Compact));

    ui_workout->feedbackLabel->setText("운동이 중지되었습니다");
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
    ui_workout->timerLabel->setText(QString("%1:%2")
                                        .arg(minutes, 2, 10, QChar('0'))
                                        .arg(seconds, 2, 10, QChar('0')));
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

// ============================================================================
// MQTT Protocol Helpers (WatchTower Compatible)
// ============================================================================

QString MainWindow::convertExerciseNameToMode(const QString &exerciseName)
{
    /**
     * 한글 운동 이름을 WatchTower가 인식하는 영어 모드명으로 변환
     * WatchTower의 SUPPORTED_MODES: ['squat', 'pushup']
     * T-Pose는 삭제됨, 나머지는 미구현
     */
    static QMap<QString, QString> exerciseMap = {
        {"스쿼트", "squat"},
        {"푸시업", "pushup"},
        {"플랭크", "plank"},        // 미구현
        {"런지", "lunge"},          // 미구현
        {"점핑잭", "jumping_jack"},  // 미구현
        {"마운틴 클라이머", "mountain_climber"},  // 미구현
        {"버피", "burpee"},         // 미구현
        {"사용자 정의 1", "custom1"},  // 미구현
        {"사용자 정의 2", "custom2"}   // 미구현
    };

    QString mode = exerciseMap.value(exerciseName, "");
    if (mode.isEmpty()) {
        qWarning() << "Unknown exercise name:" << exerciseName;
        return "squat";  // Default fallback (squat is implemented)
    }

    return mode;
}

void MainWindow::sendModeSelectCommand(const QString &mode)
{
    /**
     * WatchTower에 운동 모드 선택 명령 전송
     * Topic: qt/command/select_mode
     * Format: {"mode": "t_pose", "timestamp": 1234567890}
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

    if (responseType == "mode_selected") {
        // 모드 선택 확인
        QString mode = data["mode"].toString();
        QString status = data["status"].toString();

        if (status == "success") {
            qDebug() << "✓ Mode selected successfully:" << mode;
            ui_mainMenu->statusLabel->setText(QString("모드 선택됨: %1").arg(mode));

            // Parse pose sequence information
            if (data.contains("poses") && data["poses"].isArray()) {
                m_poses = data["poses"].toArray();
                m_totalPoses = m_poses.size();
                m_currentPoseIndex = 0;
                m_repCount = 0;

                qDebug() << "Loaded" << m_totalPoses << "poses for mode:" << mode;

                // Display first pose information
                updatePoseDisplay();
            }
        } else {
            qWarning() << "✗ Mode selection failed:" << mode;
        }

    } else if (responseType == "analysis") {
        // 운동 분석 결과 (실시간 피드백)
        updateWorkoutFeedback(data);
        if (data.contains("frame")) {
            displayVideoFrame(data.value("frame").toString());
        }

    } else if (responseType == "error") {
        // 에러 메시지
        QString errorMsg = data["message"].toString();
        qWarning() << "✗ WatchTower error:" << errorMsg;

        if (m_isWorkoutRunning) {
            ui_workout->feedbackLabel->setText(QString("오류: %1").arg(errorMsg));
            ui_workout->feedbackLabel->setStyleSheet("color: red;");
        }

    } else if (responseType == "status") {
        // 상태 업데이트
        QString status = data["status"].toString();
        qDebug() << "ℹ WatchTower status:" << status;
    } else if (responseType == "frame") {
        if (data.contains("frame")) {
            displayVideoFrame(data.value("frame").toString());
        } else {
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
    if (m_currentPoseIndex < 0 || m_currentPoseIndex >= m_totalPoses) {
        qWarning() << "Invalid pose index:" << m_currentPoseIndex;
        return;
    }

    QJsonObject currentPose = m_poses[m_currentPoseIndex].toObject();
    QString poseName = currentPose["name"].toString();
    QString poseDescription = currentPose["description"].toString();

    // UI에 현재 포즈 정보 표시 (feedbackLabel 위에 추가로 표시)
    ui_workout->exerciseTitleLabel->setText(
        QString("운동: %1 (%2/%3)")
        .arg(m_currentExercise)
        .arg(m_currentPoseIndex + 1)
        .arg(m_totalPoses)
    );

    // 포즈 설명을 상태 표시줄에 표시
    if (ui_workout->feedbackLabel->text().isEmpty() ||
        ui_workout->feedbackLabel->text() == "시작 버튼을 눌러주세요") {
        ui_workout->feedbackLabel->setText(poseDescription);
        ui_workout->feedbackLabel->setStyleSheet("");
    }

    qDebug() << "Pose" << (m_currentPoseIndex + 1) << "/" << m_totalPoses << ":" << poseDescription;
}

void MainWindow::nextPose()
{
    /**
     * 다음 포즈로 이동
     * 마지막 포즈가 아닐 때만 증가
     */
    if (m_currentPoseIndex < m_totalPoses - 1) {
        m_currentPoseIndex++;
        updatePoseDisplay();
        qDebug() << "Moving to next pose:" << (m_currentPoseIndex + 1) << "/" << m_totalPoses;
    } else {
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
