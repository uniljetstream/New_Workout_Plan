#pragma once

#include <QMainWindow>
#include <QtMqtt/QMqttClient>
#include <QString>
#include <QStringList>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void handleConnectButton();
    void handleSelectMode();
    void handleStopCommand();
    void onStateChanged(QMqttClient::ClientState state);
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);

private:
    void logMessage(const QString &text);
    void subscribeToTopics();
    QVariantMap buildCommonPayload() const;
    void loadConfig();

    QMqttClient m_client;
    QLineEdit *m_hostEdit;
    QLineEdit *m_portEdit;
    QComboBox *m_modeCombo;
    QPushButton *m_connectButton;
    QPushButton *m_selectButton;
    QPushButton *m_stopButton;
    QTextEdit *m_logView;

    QString m_topicSelectMode;
    QString m_topicStop;
    QStringList m_subscribeTopics;
    QStringList m_availableModes;
    QString m_clientIdPrefix;
    QString m_configPath;
    QString m_defaultHost;
    quint16 m_defaultPort = 1883;
};
