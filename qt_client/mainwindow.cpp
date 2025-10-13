#include "mainwindow.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtMqtt/QMqttSubscription>
#include <QVariantMap>
#include <QStringList>

static QString stateToString(QMqttClient::ClientState state)
{
    switch (state) {
    case QMqttClient::Disconnected:
        return QStringLiteral("Disconnected");
    case QMqttClient::Connecting:
        return QStringLiteral("Connecting");
    case QMqttClient::Connected:
        return QStringLiteral("Connected");
    }
    return QStringLiteral("Unknown");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("WatchTower MQTT 테스트 클라이언트"));
    resize(640, 480);

    QWidget *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    // Broker 설정 행
    auto *brokerRow = new QHBoxLayout;
    brokerRow->addWidget(new QLabel(tr("브로커 주소:"), this));
    m_hostEdit = new QLineEdit(this);
    brokerRow->addWidget(m_hostEdit);

    brokerRow->addWidget(new QLabel(tr("포트:"), this));
    m_portEdit = new QLineEdit(this);
    m_portEdit->setMaximumWidth(80);
    brokerRow->addWidget(m_portEdit);

    m_connectButton = new QPushButton(tr("연결"), this);
    brokerRow->addWidget(m_connectButton);
    brokerRow->addStretch();
    layout->addLayout(brokerRow);

    // 모드 선택 행
    auto *modeRow = new QHBoxLayout;
    modeRow->addWidget(new QLabel(tr("운동 모드:"), this));
    m_modeCombo = new QComboBox(this);
    modeRow->addWidget(m_modeCombo);

    m_selectButton = new QPushButton(tr("모드 선택 전송"), this);
    m_selectButton->setEnabled(false);
    modeRow->addWidget(m_selectButton);

    m_stopButton = new QPushButton(tr("정지 전송"), this);
    m_stopButton->setEnabled(false);
    modeRow->addWidget(m_stopButton);
    modeRow->addStretch();
    layout->addLayout(modeRow);

    // 로그 뷰
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    layout->addWidget(m_logView, 1);

    setCentralWidget(central);

    loadConfig();

    // MQTT 클라이언트 설정
    QString clientIdPrefix = m_clientIdPrefix.isEmpty()
                                 ? QStringLiteral("watchtower_qt_tester")
                                 : m_clientIdPrefix;
    m_client.setClientId(QStringLiteral("%1_%2")
                             .arg(clientIdPrefix,
                                  QString::number(QDateTime::currentMSecsSinceEpoch() & 0xFFFF)));
    m_client.setCleanSession(true);

    connect(m_connectButton, &QPushButton::clicked,
            this, &MainWindow::handleConnectButton);
    connect(m_selectButton, &QPushButton::clicked,
            this, &MainWindow::handleSelectMode);
    connect(m_stopButton, &QPushButton::clicked,
            this, &MainWindow::handleStopCommand);

    connect(&m_client, &QMqttClient::stateChanged,
            this, &MainWindow::onStateChanged);
    connect(&m_client, &QMqttClient::messageReceived,
            this, &MainWindow::onMessageReceived);
    connect(&m_client, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError error) {
        if (error != QMqttClient::NoError) {
            logMessage(tr("MQTT 오류: %1").arg(m_client.errorString()));
        }
    });
}

void MainWindow::handleConnectButton()
{
    if (m_client.state() == QMqttClient::Connected) {
        logMessage(tr("브로커 연결 해제 중..."));
        m_client.disconnectFromHost();
        return;
    }

    m_client.setHostname(m_hostEdit->text().trimmed());
    bool ok = false;
    quint16 port = m_portEdit->text().toUShort(&ok);
    if (!ok) {
        logMessage(tr("포트 번호가 올바르지 않습니다."));
        return;
    }
    m_client.setPort(port);

    logMessage(tr("브로커 연결 시도: %1:%2")
                   .arg(m_client.hostname())
                   .arg(m_client.port()));
    m_client.connectToHost();
}

void MainWindow::handleSelectMode()
{
    if (m_client.state() != QMqttClient::Connected) {
        logMessage(tr("브로커에 연결되어 있지 않습니다."));
        return;
    }

    QVariantMap payload = buildCommonPayload();
    payload.insert(QStringLiteral("mode"), m_modeCombo->currentText());

    const QJsonDocument doc(QJsonObject::fromVariantMap(payload));
    const QByteArray message = doc.toJson(QJsonDocument::Compact);

    auto result = m_client.publish(m_topicSelectMode,
                                   message,
                                   1,
                                   false);
    if (result == -1) {
        logMessage(tr("모드 선택 메시지 전송 실패"));
    } else {
        logMessage(tr("모드 선택 전송 → %1")
                       .arg(QString::fromUtf8(message)));
    }
}

void MainWindow::handleStopCommand()
{
    if (m_client.state() != QMqttClient::Connected) {
        logMessage(tr("브로커에 연결되어 있지 않습니다."));
        return;
    }

    QVariantMap payload = buildCommonPayload();
    payload.insert(QStringLiteral("command"), QStringLiteral("stop"));

    const QJsonDocument doc(QJsonObject::fromVariantMap(payload));
    const QByteArray message = doc.toJson(QJsonDocument::Compact);

    auto result = m_client.publish(m_topicStop,
                                   message,
                                   1,
                                   false);
    if (result == -1) {
        logMessage(tr("정지 메시지 전송 실패"));
    } else {
        logMessage(tr("정지 전송 → %1")
                       .arg(QString::fromUtf8(message)));
    }
}

void MainWindow::onStateChanged(QMqttClient::ClientState state)
{
    logMessage(tr("MQTT 상태: %1").arg(stateToString(state)));

    const bool connected = (state == QMqttClient::Connected);
    m_connectButton->setText(connected ? tr("연결 해제") : tr("연결"));
    m_selectButton->setEnabled(connected);
    m_stopButton->setEnabled(connected);
    m_hostEdit->setEnabled(!connected);
    m_portEdit->setEnabled(!connected);

    if (connected) {
        subscribeToTopics();
    }
}

void MainWindow::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    logMessage(tr("수신 [%1] %2")
                   .arg(topic.name(),
                        QString::fromUtf8(message)));
}

void MainWindow::logMessage(const QString &text)
{
    m_logView->append(QStringLiteral("[%1] %2")
                          .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                          .arg(text));
}

void MainWindow::subscribeToTopics()
{
    QStringList topics = m_subscribeTopics;
    if (topics.isEmpty()) {
        topics = {
            QStringLiteral("qt/response/#"),
            QStringLiteral("joystick/#"),
            QStringLiteral("watch/#")
        };
    }

    for (const QString &topic : topics) {
        auto *subscription = m_client.subscribe(topic, 1);
        if (!subscription) {
            logMessage(tr("토픽 구독 실패: %1").arg(topic));
        } else {
            logMessage(tr("토픽 구독 완료: %1").arg(topic));
        }
    }
}

QVariantMap MainWindow::buildCommonPayload() const
{
    QVariantMap payload;
    payload.insert(QStringLiteral("timestamp"),
                   static_cast<qint64>(QDateTime::currentMSecsSinceEpoch()));
    return payload;
}

void MainWindow::loadConfig()
{
    m_defaultHost = QStringLiteral("localhost");
    m_defaultPort = 1883;
    m_topicSelectMode = QStringLiteral("qt/command/select_mode");
    m_topicStop = QStringLiteral("qt/command/stop");
    m_subscribeTopics = {
        QStringLiteral("qt/response/#"),
        QStringLiteral("joystick/#"),
        QStringLiteral("watch/#")
    };
    m_availableModes = {
        QStringLiteral("t_pose"),
        QStringLiteral("squat"),
        QStringLiteral("pushup")
    };
    m_clientIdPrefix = QStringLiteral("watchtower_qt_tester");

    m_configPath = QCoreApplication::applicationDirPath()
                   + QLatin1String("/config.json");

    QFile configFile(m_configPath);
    if (configFile.exists()) {
        if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonParseError parseError{};
            const QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll(), &parseError);
            configFile.close();

            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                logMessage(tr("설정 파일 파싱 실패 (%1), 기본값 사용")
                               .arg(parseError.errorString()));
            } else {
                const QJsonObject root = doc.object();

                if (root.contains(QStringLiteral("mqtt"))) {
                    const QJsonObject mqtt = root.value(QStringLiteral("mqtt")).toObject();
                    if (mqtt.contains(QStringLiteral("host")) && mqtt.value(QStringLiteral("host")).isString()) {
                        m_defaultHost = mqtt.value(QStringLiteral("host")).toString();
                    }
                    if (mqtt.contains(QStringLiteral("port")) && mqtt.value(QStringLiteral("port")).isDouble()) {
                        const int portValue = mqtt.value(QStringLiteral("port")).toInt();
                        if (portValue > 0 && portValue <= 65535) {
                            m_defaultPort = static_cast<quint16>(portValue);
                        }
                    }
                    if (mqtt.contains(QStringLiteral("client_id_prefix")) && mqtt.value(QStringLiteral("client_id_prefix")).isString()) {
                        m_clientIdPrefix = mqtt.value(QStringLiteral("client_id_prefix")).toString();
                    }
                }

                if (root.contains(QStringLiteral("topics"))) {
                    const QJsonObject topicsObj = root.value(QStringLiteral("topics")).toObject();
                    if (topicsObj.contains(QStringLiteral("select_mode")) && topicsObj.value(QStringLiteral("select_mode")).isString()) {
                        m_topicSelectMode = topicsObj.value(QStringLiteral("select_mode")).toString();
                    }
                    if (topicsObj.contains(QStringLiteral("stop")) && topicsObj.value(QStringLiteral("stop")).isString()) {
                        m_topicStop = topicsObj.value(QStringLiteral("stop")).toString();
                    }
                    if (topicsObj.contains(QStringLiteral("subscriptions")) && topicsObj.value(QStringLiteral("subscriptions")).isArray()) {
                        const QJsonArray subArray = topicsObj.value(QStringLiteral("subscriptions")).toArray();
                        QStringList subscriptions;
                        for (const QJsonValue &value : subArray) {
                            if (value.isString()) {
                                subscriptions.append(value.toString());
                            }
                        }
                        if (!subscriptions.isEmpty()) {
                            m_subscribeTopics = subscriptions;
                        }
                    }
                }

                if (root.contains(QStringLiteral("modes")) && root.value(QStringLiteral("modes")).isArray()) {
                    const QJsonArray modesArray = root.value(QStringLiteral("modes")).toArray();
                    QStringList modes;
                    for (const QJsonValue &value : modesArray) {
                        if (value.isString()) {
                            modes.append(value.toString());
                        }
                    }
                    if (!modes.isEmpty()) {
                        m_availableModes = modes;
                    }
                }
            }
        } else {
            logMessage(tr("설정 파일을 열 수 없습니다: %1").arg(configFile.errorString()));
        }
    } else {
        logMessage(tr("설정 파일이 없어 기본값을 사용합니다. (%1)").arg(m_configPath));
    }

    m_hostEdit->setText(m_defaultHost);
    m_portEdit->setText(QString::number(m_defaultPort));
    m_modeCombo->clear();
    m_modeCombo->addItems(m_availableModes);

    logMessage(tr("설정 적용 완료: %1").arg(m_configPath));
}
