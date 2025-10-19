#include "settings_page_widget.h"

#include "ui_settings.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

SettingsPageWidget::SettingsPageWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::SettingsPage)
{
    m_ui->setupUi(this);

    connect(m_ui->connectButton, &QPushButton::clicked,
            this, &SettingsPageWidget::connectRequested);
    connect(m_ui->disconnectButton, &QPushButton::clicked,
            this, &SettingsPageWidget::disconnectRequested);
    connect(m_ui->calibrateButton, &QPushButton::clicked,
            this, &SettingsPageWidget::calibrateRequested);
    connect(m_ui->testAirMouseButton, &QPushButton::clicked,
            this, &SettingsPageWidget::toggleAirMouseRequested);
    connect(m_ui->saveButton, &QPushButton::clicked,
            this, &SettingsPageWidget::saveRequested);
    connect(m_ui->backButton, &QPushButton::clicked,
            this, &SettingsPageWidget::backRequested);

    connect(m_ui->sensitivitySlider, &QSlider::valueChanged, this, [this](int value) {
        const double sensitivity = value / 10.0;
        m_ui->sensitivityValueLabel->setText(QString("%1x").arg(sensitivity, 0, 'f', 1));
        emit sensitivityChanged(sensitivity);
    });

    connect(m_ui->smoothingCheckBox, &QCheckBox::toggled,
            this, &SettingsPageWidget::smoothingChanged);
    connect(m_ui->trailCheckBox, &QCheckBox::toggled,
            this, &SettingsPageWidget::showTrailChanged);

    // 초기 레이블 표시 업데이트
    const double initialSensitivity = m_ui->sensitivitySlider->value() / 10.0;
    m_ui->sensitivityValueLabel->setText(QString("%1x").arg(initialSensitivity, 0, 'f', 1));
}

SettingsPageWidget::~SettingsPageWidget()
{
    delete m_ui;
}

QString SettingsPageWidget::broker() const
{
    return m_ui->brokerLineEdit->text();
}

int SettingsPageWidget::port() const
{
    return m_ui->portSpinBox->value();
}

void SettingsPageWidget::setBroker(const QString &host)
{
    m_ui->brokerLineEdit->setText(host);
}

void SettingsPageWidget::setPort(int port)
{
    m_ui->portSpinBox->setValue(port);
}

void SettingsPageWidget::setMqttStatus(bool connected)
{
    if (connected) {
        m_ui->statusValueLabel->setText(tr("<span style='color:green;'>연결됨</span>"));
    } else {
        m_ui->statusValueLabel->setText(tr("<span style='color:red;'>연결 안됨</span>"));
    }
}

void SettingsPageWidget::setAirMouseStatus(bool enabled)
{
    const QString statusText = enabled ? tr("활성화") : tr("비활성화");
    const QString color = enabled ? "#4CAF50" : "#f44336";
    m_ui->airmouseStatusValueLabel->setText(statusText);
    m_ui->airmouseStatusValueLabel->setStyleSheet(
        QStringLiteral("color: %1; font-weight: bold;").arg(color));

    updateAirMouseButton(enabled);
}

void SettingsPageWidget::updateAirMouseButton(bool enabled)
{
    if (enabled) {
        m_ui->testAirMouseButton->setText(tr("에어마우스 끄기"));
        m_ui->testAirMouseButton->setStyleSheet(QStringLiteral(
            "background-color: #f44336; color: white; padding: 5px; border-radius: 5px;"));
    } else {
        m_ui->testAirMouseButton->setText(tr("에어마우스 테스트"));
        m_ui->testAirMouseButton->setStyleSheet(QStringLiteral(
            "background-color: #2196F3; color: white; padding: 5px; border-radius: 5px;"));
    }
}
