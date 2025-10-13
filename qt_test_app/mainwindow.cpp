#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_client(nullptr)
    , m_config(Config::instance())
    , m_cursorCanvas(nullptr)
    , m_testTimer(nullptr)
{
    ui->setupUi(this);
    loadConfiguration();
    setupMqttClient();
    setupCursorCanvas();
    updateConnectionStatus(false);
}

MainWindow::~MainWindow()
{
    if (m_client) {
        m_client->disconnectFromHost();
    }
    delete ui;
}

void MainWindow::loadConfiguration()
{
    // Load config from file
    if (!m_config.loadFromFile("config.json")) {
        appendLog("Failed to load config.json, using defaults", "orange");
    } else {
        appendLog("Configuration loaded successfully", "green");
    }

    // Apply UI settings
    resize(m_config.windowWidth(), m_config.windowHeight());
    ui->brokerLineEdit->setText(m_config.mqttBroker());
    ui->portSpinBox->setValue(m_config.mqttPort());

    // Load exercise modes
    loadExerciseModes();

    // Auto-connect if enabled
    if (m_config.autoConnect()) {
        QTimer::singleShot(500, this, &MainWindow::on_connectButton_clicked);
    }
}

void MainWindow::loadExerciseModes()
{
    ui->modeComboBox->clear();
    QStringList modes = m_config.exerciseModes();
    for (const QString &mode : modes) {
        ui->modeComboBox->addItem(mode);
    }
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
}

void MainWindow::setupCursorCanvas()
{
    // Create cursor canvas
    m_cursorCanvas = new CursorCanvas(this);

    // Replace the placeholder widget with cursor canvas
    QVBoxLayout *layout = new QVBoxLayout(ui->cursorCanvasWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_cursorCanvas);

    appendLog("Cursor canvas initialized", "green");
}

void MainWindow::subscribeToTopics()
{
    // Subscribe to joystick topics
    auto sub1 = m_client->subscribe(m_config.topicJoystickData());
    if (!sub1) {
        appendLog("Failed to subscribe to " + m_config.topicJoystickData(), "red");
    }

    auto sub2 = m_client->subscribe(m_config.topicJoystickStatus());
    if (!sub2) {
        appendLog("Failed to subscribe to " + m_config.topicJoystickStatus(), "red");
    }

    // Subscribe to watch topics
    auto sub3 = m_client->subscribe(m_config.topicWatchHeartrate());
    if (!sub3) {
        appendLog("Failed to subscribe to " + m_config.topicWatchHeartrate(), "red");
    }

    auto sub4 = m_client->subscribe(m_config.topicWatchStatus());
    if (!sub4) {
        appendLog("Failed to subscribe to " + m_config.topicWatchStatus(), "red");
    }

    // Subscribe to Qt response topics
    auto sub5 = m_client->subscribe(m_config.topicQtResponse());
    if (!sub5) {
        appendLog("Failed to subscribe to " + m_config.topicQtResponse(), "red");
    }

    appendLog("Subscribed to all topics", "green");
}

void MainWindow::publishMessage(const QString &topic, const QString &message)
{
    if (m_client->state() != QMqttClient::Connected) {
        appendLog("Not connected to MQTT broker", "red");
        return;
    }

    qint32 id = m_client->publish(topic, message.toUtf8(), 0);
    if (id == -1) {
        appendLog("Failed to publish to " + topic, "red");
    } else {
        appendLog("Published to " + topic + ": " + message, "blue");
    }
}

void MainWindow::appendLog(const QString &message, const QString &color)
{
    if (!m_config.enableLogging()) {
        return;
    }

    QString html;
    if (m_config.logTimestamps()) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        html = QString("<span style='color:gray;'>[%1]</span> <span style='color:%2;'>%3</span>")
                   .arg(timestamp)
                   .arg(color)
                   .arg(message);
    } else {
        html = QString("<span style='color:%1;'>%2</span>")
                   .arg(color)
                   .arg(message);
    }

    ui->logTextEdit->append(html);

    // Limit log lines
    if (ui->logTextEdit->document()->lineCount() > m_config.maxLogLines()) {
        QTextCursor cursor = ui->logTextEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor,
                           ui->logTextEdit->document()->lineCount() - m_config.maxLogLines());
        cursor.removeSelectedText();
    }
}

void MainWindow::updateSensorData(const QJsonObject &data, bool isJoystick)
{
    if (isJoystick) {
        // Check if airmouse mode
        if (data.contains("mode") && data["mode"].toString() == "airmouse") {
            ui->accelXLabel->setText(QString("Mouse X: %1").arg(data["mouse_x"].toDouble(), 0, 'f', 2));
            ui->accelYLabel->setText(QString("Mouse Y: %1").arg(data["mouse_y"].toDouble(), 0, 'f', 2));
            ui->accelZLabel->setText(QString("Scroll: %1").arg(data["scroll_delta"].toInt()));
            ui->gyroXLabel->setText("Mode: AirMouse");
            ui->gyroYLabel->clear();
            ui->gyroZLabel->clear();
        } else {
            // Sensor mode
            ui->accelXLabel->setText(QString("Accel X: %1").arg(data["accel_x"].toDouble(), 0, 'f', 2));
            ui->accelYLabel->setText(QString("Accel Y: %1").arg(data["accel_y"].toDouble(), 0, 'f', 2));
            ui->accelZLabel->setText(QString("Accel Z: %1").arg(data["accel_z"].toDouble(), 0, 'f', 2));
            ui->gyroXLabel->setText(QString("Gyro X: %1").arg(data["gyro_x"].toDouble(), 0, 'f', 2));
            ui->gyroYLabel->setText(QString("Gyro Y: %1").arg(data["gyro_y"].toDouble(), 0, 'f', 2));
            ui->gyroZLabel->setText(QString("Gyro Z: %1").arg(data["gyro_z"].toDouble(), 0, 'f', 2));
        }
    } else {
        // Watch heart rate
        if (data.contains("heartrate")) {
            ui->heartrateLabel->setText(QString("Heart Rate: %1 BPM").arg(data["heartrate"].toInt()));
        }
    }
}

void MainWindow::updateConnectionStatus(bool connected)
{
    if (connected) {
        ui->statusLabel->setText("<span style='color:green;'>Connected</span>");
        ui->connectButton->setEnabled(false);
        ui->disconnectButton->setEnabled(true);
        ui->commandGroupBox->setEnabled(true);
    } else {
        ui->statusLabel->setText("<span style='color:red;'>Disconnected</span>");
        ui->connectButton->setEnabled(true);
        ui->disconnectButton->setEnabled(false);
        ui->commandGroupBox->setEnabled(false);
    }
}

// Slot implementations
void MainWindow::on_connectButton_clicked()
{
    m_client->setHostname(ui->brokerLineEdit->text());
    m_client->setPort(ui->portSpinBox->value());
    m_client->connectToHost();
    appendLog("Connecting to broker...", "blue");
}

void MainWindow::on_disconnectButton_clicked()
{
    m_client->disconnectFromHost();
    appendLog("Disconnecting from broker...", "blue");
}

void MainWindow::on_startButton_clicked()
{
    QString mode = ui->modeComboBox->currentText();
    QJsonObject json;
    json["command"] = "start";
    json["mode"] = mode.toLower().replace(" ", "_");
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtCmdStart(), doc.toJson(QJsonDocument::Compact));

    // Also send start to devices
    QJsonObject deviceCmd;
    deviceCmd["command"] = "start";
    QJsonDocument deviceDoc(deviceCmd);
    publishMessage(m_config.topicWatchtowerCmdJoystick(), deviceDoc.toJson(QJsonDocument::Compact));
    publishMessage(m_config.topicWatchtowerCmdWatch(), deviceDoc.toJson(QJsonDocument::Compact));
}

void MainWindow::on_stopButton_clicked()
{
    QJsonObject json;
    json["command"] = "stop";
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtCmdStop(), doc.toJson(QJsonDocument::Compact));

    // Also send stop to devices
    publishMessage(m_config.topicWatchtowerCmdJoystick(), doc.toJson(QJsonDocument::Compact));
    publishMessage(m_config.topicWatchtowerCmdWatch(), doc.toJson(QJsonDocument::Compact));
}

void MainWindow::on_selectModeButton_clicked()
{
    QString mode = ui->modeComboBox->currentText();
    QJsonObject json;
    json["command"] = "select_mode";
    json["mode"] = mode.toLower().replace(" ", "_");
    json["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument doc(json);
    publishMessage(m_config.topicQtCmdSelect(), doc.toJson(QJsonDocument::Compact));
}

void MainWindow::on_airMouseModeButton_clicked()
{
    QJsonObject json;
    json["command"] = "airmouse_mode";

    QJsonDocument doc(json);
    publishMessage(m_config.topicWatchtowerCmdJoystick(), doc.toJson(QJsonDocument::Compact));
}

void MainWindow::on_sensorModeButton_clicked()
{
    QJsonObject json;
    json["command"] = "sensor_mode";

    QJsonDocument doc(json);
    publishMessage(m_config.topicWatchtowerCmdJoystick(), doc.toJson(QJsonDocument::Compact));
}

void MainWindow::on_calibrateButton_clicked()
{
    QJsonObject json;
    json["command"] = "calibrate";

    QJsonDocument doc(json);
    publishMessage(m_config.topicWatchtowerCmdJoystick(), doc.toJson(QJsonDocument::Compact));
}

void MainWindow::on_clearLogButton_clicked()
{
    ui->logTextEdit->clear();
}

// MQTT client slots
void MainWindow::onMqttConnected()
{
    appendLog("Connected to MQTT broker", "green");
    updateConnectionStatus(true);
    subscribeToTopics();
}

void MainWindow::onMqttDisconnected()
{
    appendLog("Disconnected from MQTT broker", "red");
    updateConnectionStatus(false);
}

void MainWindow::onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString topicStr = topic.name();
    QString messageStr = QString::fromUtf8(message);

    appendLog("Received from " + topicStr + ": " + messageStr, "darkgreen");

    // Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(message);
    if (doc.isNull() || !doc.isObject()) {
        return;
    }

    QJsonObject json = doc.object();

    // Update UI based on topic
    if (topicStr == m_config.topicJoystickData()) {
        // Check if it's airmouse data
        if (json.contains("mode") && json["mode"].toString() == "airmouse") {
            updateAirMouseData(json);
        } else {
            updateSensorData(json, true);
        }
    } else if (topicStr == m_config.topicWatchHeartrate()) {
        updateSensorData(json, false);
    } else if (topicStr == m_config.topicJoystickStatus()) {
        ui->joystickStatusLabel->setText("Joystick: " + json["status"].toString());
    } else if (topicStr == m_config.topicWatchStatus()) {
        ui->watchStatusLabel->setText("Watch: " + json["status"].toString());
    } else if (topicStr.startsWith("qt/response/")) {
        // Handle WatchTower responses
        QString responseType = topicStr.split("/").last();
        ui->watchtowerResponseLabel->setText("WatchTower: " + responseType + " - " + messageStr.left(50));
    }
}

void MainWindow::updateAirMouseData(const QJsonObject &data)
{
    // Extract mouse movement data
    double mouseX = data["mouse_x"].toDouble();
    double mouseY = data["mouse_y"].toDouble();
    int scrollDelta = data["scroll_delta"].toInt();

    // Update UI display
    QString dataText = QString("Mouse X: %1\nMouse Y: %2\nScroll: %3")
                           .arg(mouseX, 0, 'f', 2)
                           .arg(mouseY, 0, 'f', 2)
                           .arg(scrollDelta);
    ui->airMouseDataLabel->setText(dataText);

    // Move cursor in canvas
    if (m_cursorCanvas && (qAbs(mouseX) > 0.1 || qAbs(mouseY) > 0.1)) {
        m_cursorCanvas->moveCursor(static_cast<int>(mouseX), static_cast<int>(mouseY));
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
    appendLog("MQTT State: " + stateStr, "gray");
}

void MainWindow::onMqttError(QMqttClient::ClientError error)
{
    if (error != QMqttClient::NoError) {
        QString errorStr;
        switch (error) {
        case QMqttClient::InvalidProtocolVersion:
            errorStr = "Invalid protocol version";
            break;
        case QMqttClient::IdRejected:
            errorStr = "ID rejected";
            break;
        case QMqttClient::ServerUnavailable:
            errorStr = "Server unavailable";
            break;
        case QMqttClient::BadUsernameOrPassword:
            errorStr = "Bad username or password";
            break;
        case QMqttClient::NotAuthorized:
            errorStr = "Not authorized";
            break;
        case QMqttClient::TransportInvalid:
            errorStr = "Transport invalid";
            break;
        case QMqttClient::ProtocolViolation:
            errorStr = "Protocol violation";
            break;
        case QMqttClient::UnknownError:
            errorStr = "Unknown error";
            break;
        default:
            errorStr = "Error";
            break;
        }
        appendLog("MQTT Error: " + errorStr, "red");
    }
}

// AirMouse control slots
void MainWindow::on_resetCursorButton_clicked()
{
    if (m_cursorCanvas) {
        m_cursorCanvas->resetCursor();
        appendLog("Cursor reset to center", "blue");
    }
}

void MainWindow::on_cursorSensitivitySlider_valueChanged(int value)
{
    double sensitivity = value / 10.0;
    if (m_cursorCanvas) {
        m_cursorCanvas->setSensitivity(sensitivity);
    }
    ui->cursorSensitivityLabel->setText(QString("%1x").arg(sensitivity, 0, 'f', 1));
}

void MainWindow::on_cursorSmoothingCheckBox_toggled(bool checked)
{
    if (m_cursorCanvas) {
        m_cursorCanvas->setSmoothing(checked);
    }
}

void MainWindow::on_showTrailCheckBox_toggled(bool checked)
{
    if (m_cursorCanvas) {
        m_cursorCanvas->setShowTrail(checked);
    }
}


// Test cursor movement
void MainWindow::on_testCursorButton_clicked()
{
    if (!m_testTimer) {
        m_testTimer = new QTimer(this);
        connect(m_testTimer, &QTimer::timeout, this, &MainWindow::onTestTimerTimeout);
    }

    if (m_testTimer->isActive()) {
        m_testTimer->stop();
        ui->testCursorButton->setText("Start Test (Circle)");
        appendLog("Test cursor movement stopped", "blue");
    } else {
        m_testTimer->start(50); // 20Hz
        ui->testCursorButton->setText("Stop Test");
        appendLog("Test cursor movement started - circular pattern", "green");
    }
}

void MainWindow::onTestTimerTimeout()
{
    static double angle = 0.0;
    
    // Generate circular movement
    double radius = 15.0;
    double mouseX = radius * qCos(angle);
    double mouseY = radius * qSin(angle);
    
    angle += 0.1; // Increment angle
    if (angle > 2 * M_PI) {
        angle = 0.0;
    }
    
    // Move cursor
    if (m_cursorCanvas) {
        m_cursorCanvas->moveCursor(static_cast<int>(mouseX), static_cast<int>(mouseY));
    }
    
    // Update data label
    QString dataText = QString("Mouse X: %1\nMouse Y: %2\nScroll: 0\n(TEST MODE)")
                           .arg(mouseX, 0, 'f', 2)
                           .arg(mouseY, 0, 'f', 2);
    ui->airMouseDataLabel->setText(dataText);
}
