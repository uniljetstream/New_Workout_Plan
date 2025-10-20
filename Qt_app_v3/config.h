#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QMap>

class Config
{
public:
    static Config& instance();

    // Load configuration from file
    bool loadFromFile(const QString &filePath = "config.json");

    // Save configuration to file
    bool saveToFile(const QString &filePath = "config.json");

    // MQTT Broker settings
    QString mqttBroker() const { return m_mqttBroker; }
    int mqttPort() const { return m_mqttPort; }
    QString mqttClientId() const { return m_mqttClientId; }
    QString mqttUsername() const { return m_mqttUsername; }
    QString mqttPassword() const { return m_mqttPassword; }

    void setMqttBroker(const QString &broker) { m_mqttBroker = broker; }
    void setMqttPort(int port) { m_mqttPort = port; }
    void setMqttClientId(const QString &clientId) { m_mqttClientId = clientId; }
    void setMqttUsername(const QString &username) { m_mqttUsername = username; }
    void setMqttPassword(const QString &password) { m_mqttPassword = password; }

    // MQTT Topics
    QString topicJoystickData() const { return m_topicJoystickData; }
    QString topicJoystickStatus() const { return m_topicJoystickStatus; }
    QString topicWatchHeartrate() const { return m_topicWatchHeartrate; }
    QString topicWatchStatus() const { return m_topicWatchStatus; }
    QString topicWatchtowerCmdJoystick() const { return m_topicWatchtowerCmdJoystick; }
    QString topicWatchtowerCmdWatch() const { return m_topicWatchtowerCmdWatch; }
    QString topicQtCmdSelect() const { return m_topicQtCmdSelect; }
    QString topicQtCmdStart() const { return m_topicQtCmdStart; }
    QString topicQtCmdStop() const { return m_topicQtCmdStop; }
    QString topicQtResponse() const { return m_topicQtResponse; }
    QString topicQtPoseIndex() const { return m_topicQtPoseIndex; }
    QString topicQtRequestAnalysis() const { return m_topicQtRequestAnalysis; }

    void setTopicJoystickData(const QString &topic) { m_topicJoystickData = topic; }
    void setTopicJoystickStatus(const QString &topic) { m_topicJoystickStatus = topic; }
    void setTopicWatchHeartrate(const QString &topic) { m_topicWatchHeartrate = topic; }
    void setTopicWatchStatus(const QString &topic) { m_topicWatchStatus = topic; }
    void setTopicWatchtowerCmdJoystick(const QString &topic) { m_topicWatchtowerCmdJoystick = topic; }
    void setTopicWatchtowerCmdWatch(const QString &topic) { m_topicWatchtowerCmdWatch = topic; }
    void setTopicQtCmdSelect(const QString &topic) { m_topicQtCmdSelect = topic; }
    void setTopicQtCmdStart(const QString &topic) { m_topicQtCmdStart = topic; }
    void setTopicQtCmdStop(const QString &topic) { m_topicQtCmdStop = topic; }
    void setTopicQtResponse(const QString &topic) { m_topicQtResponse = topic; }
    void setTopicQtPoseIndex(const QString &topic) { m_topicQtPoseIndex = topic; }
    void setTopicQtRequestAnalysis(const QString &topic) { m_topicQtRequestAnalysis = topic; }

    // Exercise Modes
    QStringList exerciseModes() const { return m_exerciseModes; }
    void setExerciseModes(const QStringList &modes) { m_exerciseModes = modes; }

    // UI Settings
    int windowWidth() const { return m_windowWidth; }
    int windowHeight() const { return m_windowHeight; }
    bool autoConnect() const { return m_autoConnect; }
    bool saveWindowPosition() const { return m_saveWindowPosition; }

    void setWindowWidth(int width) { m_windowWidth = width; }
    void setWindowHeight(int height) { m_windowHeight = height; }
    void setAutoConnect(bool autoConnect) { m_autoConnect = autoConnect; }
    void setSaveWindowPosition(bool save) { m_saveWindowPosition = save; }

    // Logging Settings
    bool enableLogging() const { return m_enableLogging; }
    int maxLogLines() const { return m_maxLogLines; }
    bool logTimestamps() const { return m_logTimestamps; }

    void setEnableLogging(bool enable) { m_enableLogging = enable; }
    void setMaxLogLines(int maxLines) { m_maxLogLines = maxLines; }
    void setLogTimestamps(bool enable) { m_logTimestamps = enable; }

    // Routine Settings
    int routineRepsPerExercise() const { return m_routineRepsPerExercise; }
    void setRoutineRepsPerExercise(int reps)
    {
        m_routineRepsPerExercise = reps > 0 ? reps : 1;
    }

    // Individual Exercise Settings
    int individualDefaultReps() const { return m_individualDefaultReps; }
    void setIndividualDefaultReps(int reps)
    {
        m_individualDefaultReps = reps > 0 ? reps : 0;
    }
    void setIndividualReps(const QMap<QString, int> &repsMap) { m_individualReps = repsMap; }
    int individualRepsForMode(const QString &mode) const;
    const QMap<QString, int> &individualRepsMap() const { return m_individualReps; }

private:
    Config();
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void setDefaults();

    // MQTT Broker
    QString m_mqttBroker;
    int m_mqttPort;
    QString m_mqttClientId;
    QString m_mqttUsername;
    QString m_mqttPassword;

    // MQTT Topics
    QString m_topicJoystickData;
    QString m_topicJoystickStatus;
    QString m_topicWatchHeartrate;
    QString m_topicWatchStatus;
    QString m_topicWatchtowerCmdJoystick;
    QString m_topicWatchtowerCmdWatch;
    QString m_topicQtCmdSelect;
    QString m_topicQtCmdStart;
    QString m_topicQtCmdStop;
    QString m_topicQtResponse;
    QString m_topicQtPoseIndex;
    QString m_topicQtRequestAnalysis;

    // Exercise Modes
    QStringList m_exerciseModes;

    // UI Settings
    int m_windowWidth;
    int m_windowHeight;
    bool m_autoConnect;
    bool m_saveWindowPosition;

    // Logging Settings
    bool m_enableLogging;
    int m_maxLogLines;
    bool m_logTimestamps;

    // Routine Settings
    int m_routineRepsPerExercise;

    // Individual Exercise Settings
    int m_individualDefaultReps;
    QMap<QString, int> m_individualReps;
};

#endif // CONFIG_H
