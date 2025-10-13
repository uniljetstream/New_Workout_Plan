#include "config.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

Config::Config()
{
    setDefaults();
}

Config& Config::instance()
{
    static Config instance;
    return instance;
}

void Config::setDefaults()
{
    // MQTT Broker defaults
    m_mqttBroker = "10.10.16.111";
    m_mqttPort = 1883;
    m_mqttClientId = "qt_test_app";
    m_mqttUsername = "";
    m_mqttPassword = "";

    // MQTT Topics defaults
    m_topicJoystickData = "joystick/sensor/data";
    m_topicJoystickStatus = "joystick/status";
    m_topicWatchHeartrate = "watch/sensor/heartrate";
    m_topicWatchStatus = "watch/status";
    m_topicWatchtowerCmdJoystick = "watchtower/command/joystick";
    m_topicWatchtowerCmdWatch = "watchtower/command/watch";
    m_topicQtCmdSelect = "qt/command/select_mode";
    m_topicQtCmdStart = "qt/command/start";
    m_topicQtCmdStop = "qt/command/stop";
    m_topicQtResponse = "qt/response/#";

    // Exercise Modes defaults
    m_exerciseModes = QStringList() << "Squat" << "Pushup";

    // UI Settings defaults
    m_windowWidth = 900;
    m_windowHeight = 700;
    m_autoConnect = false;
    m_saveWindowPosition = true;

    // Logging Settings defaults
    m_enableLogging = true;
    m_maxLogLines = 1000;
    m_logTimestamps = true;
}

bool Config::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open config file:" << filePath;
        qWarning() << "Using default configuration";
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in config file:" << filePath;
        return false;
    }

    QJsonObject json = doc.object();

    // Load MQTT Broker settings
    if (json.contains("mqtt_broker")) {
        QJsonObject mqtt = json["mqtt_broker"].toObject();
        if (mqtt.contains("host")) m_mqttBroker = mqtt["host"].toString();
        if (mqtt.contains("port")) m_mqttPort = mqtt["port"].toInt();
        if (mqtt.contains("client_id")) m_mqttClientId = mqtt["client_id"].toString();
        if (mqtt.contains("username")) m_mqttUsername = mqtt["username"].toString();
        if (mqtt.contains("password")) m_mqttPassword = mqtt["password"].toString();
    }

    // Load MQTT Topics
    if (json.contains("mqtt_topics")) {
        QJsonObject topics = json["mqtt_topics"].toObject();
        if (topics.contains("joystick_data")) m_topicJoystickData = topics["joystick_data"].toString();
        if (topics.contains("joystick_status")) m_topicJoystickStatus = topics["joystick_status"].toString();
        if (topics.contains("watch_heartrate")) m_topicWatchHeartrate = topics["watch_heartrate"].toString();
        if (topics.contains("watch_status")) m_topicWatchStatus = topics["watch_status"].toString();
        if (topics.contains("watchtower_cmd_joystick")) m_topicWatchtowerCmdJoystick = topics["watchtower_cmd_joystick"].toString();
        if (topics.contains("watchtower_cmd_watch")) m_topicWatchtowerCmdWatch = topics["watchtower_cmd_watch"].toString();
        if (topics.contains("qt_cmd_select")) m_topicQtCmdSelect = topics["qt_cmd_select"].toString();
        if (topics.contains("qt_cmd_start")) m_topicQtCmdStart = topics["qt_cmd_start"].toString();
        if (topics.contains("qt_cmd_stop")) m_topicQtCmdStop = topics["qt_cmd_stop"].toString();
        if (topics.contains("qt_response")) m_topicQtResponse = topics["qt_response"].toString();
    }

    // Load Exercise Modes
    if (json.contains("exercise_modes")) {
        QJsonArray modes = json["exercise_modes"].toArray();
        m_exerciseModes.clear();
        for (const QJsonValue &value : modes) {
            m_exerciseModes.append(value.toString());
        }
    }

    // Load UI Settings
    if (json.contains("ui_settings")) {
        QJsonObject ui = json["ui_settings"].toObject();
        if (ui.contains("window_width")) m_windowWidth = ui["window_width"].toInt();
        if (ui.contains("window_height")) m_windowHeight = ui["window_height"].toInt();
        if (ui.contains("auto_connect")) m_autoConnect = ui["auto_connect"].toBool();
        if (ui.contains("save_window_position")) m_saveWindowPosition = ui["save_window_position"].toBool();
    }

    // Load Logging Settings
    if (json.contains("logging")) {
        QJsonObject logging = json["logging"].toObject();
        if (logging.contains("enabled")) m_enableLogging = logging["enabled"].toBool();
        if (logging.contains("max_log_lines")) m_maxLogLines = logging["max_log_lines"].toInt();
        if (logging.contains("timestamps")) m_logTimestamps = logging["timestamps"].toBool();
    }

    qDebug() << "Configuration loaded from:" << filePath;
    return true;
}

bool Config::saveToFile(const QString &filePath)
{
    QJsonObject json;

    // Save MQTT Broker settings
    QJsonObject mqtt;
    mqtt["host"] = m_mqttBroker;
    mqtt["port"] = m_mqttPort;
    mqtt["client_id"] = m_mqttClientId;
    mqtt["username"] = m_mqttUsername;
    mqtt["password"] = m_mqttPassword;
    json["mqtt_broker"] = mqtt;

    // Save MQTT Topics
    QJsonObject topics;
    topics["joystick_data"] = m_topicJoystickData;
    topics["joystick_status"] = m_topicJoystickStatus;
    topics["watch_heartrate"] = m_topicWatchHeartrate;
    topics["watch_status"] = m_topicWatchStatus;
    topics["watchtower_cmd_joystick"] = m_topicWatchtowerCmdJoystick;
    topics["watchtower_cmd_watch"] = m_topicWatchtowerCmdWatch;
    topics["qt_cmd_select"] = m_topicQtCmdSelect;
    topics["qt_cmd_start"] = m_topicQtCmdStart;
    topics["qt_cmd_stop"] = m_topicQtCmdStop;
    topics["qt_response"] = m_topicQtResponse;
    json["mqtt_topics"] = topics;

    // Save Exercise Modes
    QJsonArray modes;
    for (const QString &mode : m_exerciseModes) {
        modes.append(mode);
    }
    json["exercise_modes"] = modes;

    // Save UI Settings
    QJsonObject ui;
    ui["window_width"] = m_windowWidth;
    ui["window_height"] = m_windowHeight;
    ui["auto_connect"] = m_autoConnect;
    ui["save_window_position"] = m_saveWindowPosition;
    json["ui_settings"] = ui;

    // Save Logging Settings
    QJsonObject logging;
    logging["enabled"] = m_enableLogging;
    logging["max_log_lines"] = m_maxLogLines;
    logging["timestamps"] = m_logTimestamps;
    json["logging"] = logging;

    QJsonDocument doc(json);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open config file for writing:" << filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "Configuration saved to:" << filePath;
    return true;
}
