#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , m_mqttSub(nullptr)
    , m_cameraTimer(nullptr)
    , m_currentMode("")
    , m_isRunning(false)
{
    ui->setupUi(this);

    // 윈도우 설정
    setWindowTitle("New Workout Plan");
    resize(1280, 720);

    // 카메라 타이머 초기화
    m_cameraTimer = new QTimer(this);
    connect(m_cameraTimer, &QTimer::timeout, this, &Widget::updateFrame);

    // 초기 UI 상태
    updateUI(false);
    ui->lblStatus->setText("상태: MQTT 연결 중...");

    // MQTT 연결
    connectMqtt();
}

Widget::~Widget()
{
    stopCamera();
    disconnectMqtt();
    delete ui;
}

// ========== MQTT 관련 메서드 ==========

void Widget::connectMqtt()
{
    // mosquitto_sub 프로세스로 토픽 구독
    m_mqttSub = new QProcess(this);
    connect(m_mqttSub, &QProcess::readyReadStandardOutput, this, &Widget::onMqttSubOutput);

    QStringList args;
    args << "-h" << MQTT_BROKER;
    args << "-p" << QString::number(MQTT_PORT);
    args << "-t" << TOPIC_QT_MODE_SELECTED;
    args << "-t" << TOPIC_QT_STATUS;
    args << "-t" << TOPIC_QT_ANALYSIS;
    args << "-v";  // verbose: topic과 message를 함께 출력

    m_mqttSub->start("mosquitto_sub", args);

    if (m_mqttSub->waitForStarted()) {
        qDebug() << "✓ MQTT 구독 시작";
        ui->lblStatus->setText("상태: 대기 중");
    } else {
        qWarning() << "✗ MQTT 구독 실패";
        ui->lblStatus->setText("상태: MQTT 연결 실패");
        QMessageBox::critical(this, "연결 오류",
                              "MQTT 브로커에 연결할 수 없습니다.\n\nmosquitto가 설치되고 실행 중인지 확인하세요:\n"
                              "sudo systemctl start mosquitto");
    }
}

void Widget::disconnectMqtt()
{
    if (m_mqttSub && m_mqttSub->state() == QProcess::Running) {
        m_mqttSub->kill();
        m_mqttSub->waitForFinished();
    }
}

void Widget::publishMessage(const QString &topic, const QString &payload)
{
    // mosquitto_pub으로 메시지 발행
    QProcess *pub = new QProcess(this);

    QStringList args;
    args << "-h" << MQTT_BROKER;
    args << "-p" << QString::number(MQTT_PORT);
    args << "-t" << topic;
    args << "-m" << payload;

    pub->start("mosquitto_pub", args);
    pub->waitForFinished(1000);

    qDebug() << "→ MQTT 발행:" << topic << payload;

    pub->deleteLater();
}

void Widget::onMqttSubOutput()
{
    // mosquitto_sub의 출력 읽기 (형식: "topic message")
    while (m_mqttSub->canReadLine()) {
        QString line = QString::fromUtf8(m_mqttSub->readLine()).trimmed();

        if (line.isEmpty()) continue;

        // topic과 message 분리
        int spaceIndex = line.indexOf(' ');
        if (spaceIndex == -1) continue;

        QString topic = line.left(spaceIndex);
        QString message = line.mid(spaceIndex + 1);

        qDebug() << "← MQTT 수신:" << topic << message;

        processMqttMessage(topic, message);
    }
}

void Widget::processMqttMessage(const QString &topic, const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull()) {
        qWarning() << "✗ JSON 파싱 실패:" << message;
        return;
    }

    QJsonObject obj = doc.object();

    if (topic == TOPIC_QT_MODE_SELECTED) {
        // 모드 선택 응답
        QString mode = obj["mode"].toString();
        bool success = obj["success"].toBool();
        QString msg = obj["message"].toString();

        if (success) {
            ui->lblStatus->setText("상태: " + msg);
            QMessageBox::information(this, "운동 시작", msg + "\n분석이 시작되었습니다.");
        } else {
            ui->lblStatus->setText("상태: 오류 - " + msg);
            QMessageBox::critical(this, "오류", "운동 시작 실패\n" + msg);
            // 실패 시 UI 복원
            updateUI(false);
            stopCamera();
        }

    } else if (topic == TOPIC_QT_STATUS) {
        // 상태 업데이트
        QString status = obj["status"].toString();
        QString msg = obj["message"].toString();
        ui->lblStatus->setText("상태: " + msg);

    } else if (topic == TOPIC_QT_ANALYSIS) {
        // 분석 결과
        int score = obj["score"].toInt();
        QString feedback = obj["feedback"].toString();
        bool isCorrect = obj["is_correct"].toBool();

        // 점수 표시
        ui->lblScore->setText(QString("점수: %1%").arg(score));

        // 점수에 따른 색상
        QString color;
        if (score >= 80) {
            color = "#4CAF50";  // 녹색
        } else if (score >= 60) {
            color = "#FF9800";  // 주황색
        } else {
            color = "#f44336";  // 빨간색
        }
        ui->lblScore->setStyleSheet("QLabel { color: " + color + "; font-size: 18pt; font-weight: bold; }");

        // 피드백 표시
        QString status = isCorrect ? "✓ 정확한 자세입니다!" : "⚠ 자세를 조정하세요";
        ui->lblFeedback->setText(status + "\n" + feedback);
    }
}

// ========== 카메라 관련 메서드 ==========

bool Widget::initCamera()
{
    m_camera.open(CAMERA_ID);

    if (!m_camera.isOpened()) {
        qWarning() << "✗ 카메라 열기 실패: /dev/video" << CAMERA_ID;
        return false;
    }

    // 카메라 설정
    m_camera.set(cv::CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
    m_camera.set(cv::CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);
    m_camera.set(cv::CAP_PROP_FPS, CAMERA_FPS);

    int actualWidth = m_camera.get(cv::CAP_PROP_FRAME_WIDTH);
    int actualHeight = m_camera.get(cv::CAP_PROP_FRAME_HEIGHT);

    qDebug() << "✓ 카메라 초기화 완료:" << actualWidth << "x" << actualHeight;

    // 타이머 시작
    m_cameraTimer->start(1000 / DISPLAY_FPS);

    return true;
}

void Widget::stopCamera()
{
    if (m_cameraTimer && m_cameraTimer->isActive()) {
        m_cameraTimer->stop();
    }

    if (m_camera.isOpened()) {
        m_camera.release();
        qDebug() << "✓ 카메라 해제";
    }

    ui->lblCamera->setText("카메라 대기 중...");
}

void Widget::updateFrame()
{
    if (!m_camera.isOpened()) {
        return;
    }

    cv::Mat frame;
    m_camera >> frame;

    if (frame.empty()) {
        qWarning() << "⚠ 프레임 캡처 실패";
        return;
    }

    displayFrame(frame);
}

void Widget::displayFrame(const cv::Mat &frame)
{
    // BGR to RGB 변환
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

    // QImage로 변환
    QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);

    // QLabel 크기에 맞춰 스케일링
    QPixmap pixmap = QPixmap::fromImage(img);
    QPixmap scaled = pixmap.scaled(ui->lblCamera->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    ui->lblCamera->setPixmap(scaled);
}

// ========== 버튼 슬롯 ==========

void Widget::on_btnSquat_clicked()
{
    selectMode("squat");
}

void Widget::on_btnPushup_clicked()
{
    selectMode("pushup");
}

void Widget::on_btnLunge_clicked()
{
    selectMode("lunge");
}

void Widget::on_btnPlank_clicked()
{
    selectMode("plank");
}

void Widget::selectMode(const QString &mode)
{
    if (m_isRunning) {
        QMessageBox::warning(this, "경고", "운동 중에는 모드를 변경할 수 없습니다.");
        updateModeButtons();
        return;
    }

    m_currentMode = mode;
    updateModeButtons();

    // 선택된 운동 표시
    QMap<QString, QString> modeNames;
    modeNames["squat"] = "스쿼트";
    modeNames["pushup"] = "푸시업";
    modeNames["lunge"] = "런지";
    modeNames["plank"] = "플랭크";

    ui->lblSelectedMode->setText("선택된 운동: " + modeNames[mode]);
    ui->btnStart->setEnabled(true);

    qDebug() << "✓ 모드 선택:" << mode;
}

void Widget::updateModeButtons()
{
    ui->btnSquat->setChecked(m_currentMode == "squat");
    ui->btnPushup->setChecked(m_currentMode == "pushup");
    ui->btnLunge->setChecked(m_currentMode == "lunge");
    ui->btnPlank->setChecked(m_currentMode == "plank");
}

void Widget::on_btnStart_clicked()
{
    if (m_currentMode.isEmpty()) {
        QMessageBox::warning(this, "경고", "운동 모드를 먼저 선택하세요.");
        return;
    }

    qDebug() << "\n" << QString(60, '=');
    qDebug() << "운동 시작 요청:" << m_currentMode;
    qDebug() << QString(60, '=');

    // JSON 메시지 생성
    QJsonObject obj;
    obj["mode"] = m_currentMode;
    obj["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonDocument doc(obj);
    QString payload = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // MQTT로 모드 선택 메시지 발행
    publishMessage(TOPIC_QT_SELECT_MODE, payload);

    // UI 상태 변경
    m_isRunning = true;
    updateUI(true);
    ui->lblStatus->setText("상태: 운동 시작 중...");

    // 카메라 시작
    if (!initCamera()) {
        QMessageBox::critical(this, "오류", "카메라를 초기화할 수 없습니다.");
        m_isRunning = false;
        updateUI(false);
        return;
    }
}

void Widget::on_btnStop_clicked()
{
    qDebug() << "\n" << QString(60, '=');
    qDebug() << "운동 정지 요청";
    qDebug() << QString(60, '=');

    // JSON 메시지 생성
    QJsonObject obj;
    obj["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonDocument doc(obj);
    QString payload = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // MQTT로 정지 메시지 발행
    publishMessage(TOPIC_QT_STOP, payload);

    // 카메라 중지
    stopCamera();

    // UI 상태 변경
    m_isRunning = false;
    updateUI(false);
    ui->lblStatus->setText("상태: 대기 중");

    // 결과 초기화
    ui->lblScore->setText("점수: --");
    ui->lblScore->setStyleSheet("");
    ui->lblFeedback->setText("피드백: 운동을 시작하세요");
}

void Widget::updateUI(bool running)
{
    // 버튼 활성화/비활성화
    ui->btnSquat->setEnabled(!running);
    ui->btnPushup->setEnabled(!running);
    ui->btnLunge->setEnabled(!running);
    ui->btnPlank->setEnabled(!running);

    ui->btnStart->setEnabled(!running && !m_currentMode.isEmpty());
    ui->btnStop->setEnabled(running);
}
