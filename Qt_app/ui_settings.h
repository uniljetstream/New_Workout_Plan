/********************************************************************************
** Form generated from reading UI file 'settings.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SettingsPage
{
public:
    QVBoxLayout *verticalLayout_main;
    QLabel *titleLabel;
    QSpacerItem *verticalSpacer_1;
    QHBoxLayout *horizontalLayout_content;
    QSpacerItem *horizontalSpacer_left;
    QGroupBox *mqttGroupBox;
    QFormLayout *formLayout_mqtt;
    QLabel *brokerLabel;
    QLineEdit *brokerLineEdit;
    QLabel *portLabel;
    QSpinBox *portSpinBox;
    QLabel *connectionStatusLabel;
    QLabel *statusValueLabel;
    QHBoxLayout *horizontalLayout_connect;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QSpacerItem *horizontalSpacer_right;
    QHBoxLayout *horizontalLayout_airmouse;
    QSpacerItem *horizontalSpacer_3;
    QGroupBox *airmouseGroupBox;
    QVBoxLayout *verticalLayout_airmouse;
    QHBoxLayout *horizontalLayout_sensitivity;
    QLabel *sensitivityLabel;
    QSlider *sensitivitySlider;
    QLabel *sensitivityValueLabel;
    QCheckBox *smoothingCheckBox;
    QCheckBox *trailCheckBox;
    QHBoxLayout *horizontalLayout_airmouseStatus;
    QLabel *airmouseStatusLabel;
    QLabel *airmouseStatusValueLabel;
    QSpacerItem *horizontalSpacer_airmouseStatus;
    QHBoxLayout *horizontalLayout_calibrate;
    QPushButton *calibrateButton;
    QPushButton *testAirMouseButton;
    QSpacerItem *horizontalSpacer_4;
    QSpacerItem *verticalSpacer_2;
    QHBoxLayout *horizontalLayout_buttons;
    QSpacerItem *horizontalSpacer_5;
    QPushButton *saveButton;
    QPushButton *backButton;
    QSpacerItem *horizontalSpacer_6;

    void setupUi(QWidget *SettingsPage)
    {
        if (SettingsPage->objectName().isEmpty())
            SettingsPage->setObjectName(QString::fromUtf8("SettingsPage"));
        SettingsPage->resize(800, 600);
        verticalLayout_main = new QVBoxLayout(SettingsPage);
        verticalLayout_main->setObjectName(QString::fromUtf8("verticalLayout_main"));
        titleLabel = new QLabel(SettingsPage);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont font;
        font.setPointSize(28);
        font.setBold(true);
        font.setWeight(75);
        titleLabel->setFont(font);

        verticalLayout_main->addWidget(titleLabel);

        verticalSpacer_1 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_1);

        horizontalLayout_content = new QHBoxLayout();
        horizontalLayout_content->setObjectName(QString::fromUtf8("horizontalLayout_content"));
        horizontalSpacer_left = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_content->addItem(horizontalSpacer_left);

        mqttGroupBox = new QGroupBox(SettingsPage);
        mqttGroupBox->setObjectName(QString::fromUtf8("mqttGroupBox"));
        QFont font1;
        font1.setPointSize(14);
        font1.setBold(true);
        font1.setWeight(75);
        mqttGroupBox->setFont(font1);
        formLayout_mqtt = new QFormLayout(mqttGroupBox);
        formLayout_mqtt->setObjectName(QString::fromUtf8("formLayout_mqtt"));
        brokerLabel = new QLabel(mqttGroupBox);
        brokerLabel->setObjectName(QString::fromUtf8("brokerLabel"));

        formLayout_mqtt->setWidget(0, QFormLayout::LabelRole, brokerLabel);

        brokerLineEdit = new QLineEdit(mqttGroupBox);
        brokerLineEdit->setObjectName(QString::fromUtf8("brokerLineEdit"));

        formLayout_mqtt->setWidget(0, QFormLayout::FieldRole, brokerLineEdit);

        portLabel = new QLabel(mqttGroupBox);
        portLabel->setObjectName(QString::fromUtf8("portLabel"));

        formLayout_mqtt->setWidget(1, QFormLayout::LabelRole, portLabel);

        portSpinBox = new QSpinBox(mqttGroupBox);
        portSpinBox->setObjectName(QString::fromUtf8("portSpinBox"));
        portSpinBox->setMinimum(1);
        portSpinBox->setMaximum(65535);
        portSpinBox->setValue(1883);

        formLayout_mqtt->setWidget(1, QFormLayout::FieldRole, portSpinBox);

        connectionStatusLabel = new QLabel(mqttGroupBox);
        connectionStatusLabel->setObjectName(QString::fromUtf8("connectionStatusLabel"));

        formLayout_mqtt->setWidget(2, QFormLayout::LabelRole, connectionStatusLabel);

        statusValueLabel = new QLabel(mqttGroupBox);
        statusValueLabel->setObjectName(QString::fromUtf8("statusValueLabel"));

        formLayout_mqtt->setWidget(2, QFormLayout::FieldRole, statusValueLabel);

        horizontalLayout_connect = new QHBoxLayout();
        horizontalLayout_connect->setObjectName(QString::fromUtf8("horizontalLayout_connect"));
        connectButton = new QPushButton(mqttGroupBox);
        connectButton->setObjectName(QString::fromUtf8("connectButton"));
        connectButton->setStyleSheet(QString::fromUtf8("background-color: #4CAF50; color: white; padding: 5px; border-radius: 5px;"));

        horizontalLayout_connect->addWidget(connectButton);

        disconnectButton = new QPushButton(mqttGroupBox);
        disconnectButton->setObjectName(QString::fromUtf8("disconnectButton"));
        disconnectButton->setStyleSheet(QString::fromUtf8("background-color: #f44336; color: white; padding: 5px; border-radius: 5px;"));

        horizontalLayout_connect->addWidget(disconnectButton);


        formLayout_mqtt->setLayout(3, QFormLayout::FieldRole, horizontalLayout_connect);


        horizontalLayout_content->addWidget(mqttGroupBox);

        horizontalSpacer_right = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_content->addItem(horizontalSpacer_right);


        verticalLayout_main->addLayout(horizontalLayout_content);

        horizontalLayout_airmouse = new QHBoxLayout();
        horizontalLayout_airmouse->setObjectName(QString::fromUtf8("horizontalLayout_airmouse"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_airmouse->addItem(horizontalSpacer_3);

        airmouseGroupBox = new QGroupBox(SettingsPage);
        airmouseGroupBox->setObjectName(QString::fromUtf8("airmouseGroupBox"));
        airmouseGroupBox->setFont(font1);
        verticalLayout_airmouse = new QVBoxLayout(airmouseGroupBox);
        verticalLayout_airmouse->setObjectName(QString::fromUtf8("verticalLayout_airmouse"));
        horizontalLayout_sensitivity = new QHBoxLayout();
        horizontalLayout_sensitivity->setObjectName(QString::fromUtf8("horizontalLayout_sensitivity"));
        sensitivityLabel = new QLabel(airmouseGroupBox);
        sensitivityLabel->setObjectName(QString::fromUtf8("sensitivityLabel"));
        QFont font2;
        font2.setBold(false);
        font2.setWeight(50);
        sensitivityLabel->setFont(font2);

        horizontalLayout_sensitivity->addWidget(sensitivityLabel);

        sensitivitySlider = new QSlider(airmouseGroupBox);
        sensitivitySlider->setObjectName(QString::fromUtf8("sensitivitySlider"));
        sensitivitySlider->setMinimum(1);
        sensitivitySlider->setMaximum(50);
        sensitivitySlider->setValue(10);
        sensitivitySlider->setOrientation(Qt::Horizontal);

        horizontalLayout_sensitivity->addWidget(sensitivitySlider);

        sensitivityValueLabel = new QLabel(airmouseGroupBox);
        sensitivityValueLabel->setObjectName(QString::fromUtf8("sensitivityValueLabel"));
        sensitivityValueLabel->setFont(font2);

        horizontalLayout_sensitivity->addWidget(sensitivityValueLabel);


        verticalLayout_airmouse->addLayout(horizontalLayout_sensitivity);

        smoothingCheckBox = new QCheckBox(airmouseGroupBox);
        smoothingCheckBox->setObjectName(QString::fromUtf8("smoothingCheckBox"));
        smoothingCheckBox->setChecked(true);
        smoothingCheckBox->setFont(font2);

        verticalLayout_airmouse->addWidget(smoothingCheckBox);

        trailCheckBox = new QCheckBox(airmouseGroupBox);
        trailCheckBox->setObjectName(QString::fromUtf8("trailCheckBox"));
        trailCheckBox->setChecked(true);
        trailCheckBox->setFont(font2);

        verticalLayout_airmouse->addWidget(trailCheckBox);

        horizontalLayout_airmouseStatus = new QHBoxLayout();
        horizontalLayout_airmouseStatus->setObjectName(QString::fromUtf8("horizontalLayout_airmouseStatus"));
        airmouseStatusLabel = new QLabel(airmouseGroupBox);
        airmouseStatusLabel->setObjectName(QString::fromUtf8("airmouseStatusLabel"));
        airmouseStatusLabel->setFont(font2);

        horizontalLayout_airmouseStatus->addWidget(airmouseStatusLabel);

        airmouseStatusValueLabel = new QLabel(airmouseGroupBox);
        airmouseStatusValueLabel->setObjectName(QString::fromUtf8("airmouseStatusValueLabel"));
        airmouseStatusValueLabel->setStyleSheet(QString::fromUtf8("color: #f44336; font-weight: bold;"));

        horizontalLayout_airmouseStatus->addWidget(airmouseStatusValueLabel);

        horizontalSpacer_airmouseStatus = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_airmouseStatus->addItem(horizontalSpacer_airmouseStatus);


        verticalLayout_airmouse->addLayout(horizontalLayout_airmouseStatus);

        horizontalLayout_calibrate = new QHBoxLayout();
        horizontalLayout_calibrate->setObjectName(QString::fromUtf8("horizontalLayout_calibrate"));
        calibrateButton = new QPushButton(airmouseGroupBox);
        calibrateButton->setObjectName(QString::fromUtf8("calibrateButton"));
        calibrateButton->setStyleSheet(QString::fromUtf8("background-color: #FF9800; color: white; padding: 5px; border-radius: 5px;"));

        horizontalLayout_calibrate->addWidget(calibrateButton);

        testAirMouseButton = new QPushButton(airmouseGroupBox);
        testAirMouseButton->setObjectName(QString::fromUtf8("testAirMouseButton"));
        testAirMouseButton->setStyleSheet(QString::fromUtf8("background-color: #2196F3; color: white; padding: 5px; border-radius: 5px;"));

        horizontalLayout_calibrate->addWidget(testAirMouseButton);


        verticalLayout_airmouse->addLayout(horizontalLayout_calibrate);


        horizontalLayout_airmouse->addWidget(airmouseGroupBox);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_airmouse->addItem(horizontalSpacer_4);


        verticalLayout_main->addLayout(horizontalLayout_airmouse);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_2);

        horizontalLayout_buttons = new QHBoxLayout();
        horizontalLayout_buttons->setObjectName(QString::fromUtf8("horizontalLayout_buttons"));
        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_5);

        saveButton = new QPushButton(SettingsPage);
        saveButton->setObjectName(QString::fromUtf8("saveButton"));
        saveButton->setMinimumSize(QSize(150, 50));
        QFont font3;
        font3.setPointSize(14);
        saveButton->setFont(font3);
        saveButton->setStyleSheet(QString::fromUtf8("background-color: #4CAF50; color: white; border-radius: 10px;"));

        horizontalLayout_buttons->addWidget(saveButton);

        backButton = new QPushButton(SettingsPage);
        backButton->setObjectName(QString::fromUtf8("backButton"));
        backButton->setMinimumSize(QSize(150, 50));
        backButton->setFont(font3);
        backButton->setStyleSheet(QString::fromUtf8("background-color: #808080; color: white; border-radius: 10px;"));

        horizontalLayout_buttons->addWidget(backButton);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_6);


        verticalLayout_main->addLayout(horizontalLayout_buttons);


        retranslateUi(SettingsPage);

        QMetaObject::connectSlotsByName(SettingsPage);
    } // setupUi

    void retranslateUi(QWidget *SettingsPage)
    {
        SettingsPage->setWindowTitle(QCoreApplication::translate("SettingsPage", "\354\204\244\354\240\225", nullptr));
        titleLabel->setText(QCoreApplication::translate("SettingsPage", "\354\204\244\354\240\225", nullptr));
        mqttGroupBox->setTitle(QCoreApplication::translate("SettingsPage", "MQTT \354\204\244\354\240\225", nullptr));
        brokerLabel->setText(QCoreApplication::translate("SettingsPage", "\353\270\214\353\241\234\354\273\244 \354\243\274\354\206\214:", nullptr));
        brokerLineEdit->setText(QCoreApplication::translate("SettingsPage", "localhost", nullptr));
        portLabel->setText(QCoreApplication::translate("SettingsPage", "\355\217\254\355\212\270:", nullptr));
        connectionStatusLabel->setText(QCoreApplication::translate("SettingsPage", "\354\227\260\352\262\260 \354\203\201\355\203\234:", nullptr));
        statusValueLabel->setText(QCoreApplication::translate("SettingsPage", "\354\227\260\352\262\260 \354\225\210\353\220\250", nullptr));
        connectButton->setText(QCoreApplication::translate("SettingsPage", "\354\227\260\352\262\260", nullptr));
        disconnectButton->setText(QCoreApplication::translate("SettingsPage", "\354\227\260\352\262\260 \355\225\264\354\240\234", nullptr));
        airmouseGroupBox->setTitle(QCoreApplication::translate("SettingsPage", "\354\227\220\354\226\264\353\247\210\354\232\260\354\212\244 \354\204\244\354\240\225", nullptr));
        sensitivityLabel->setText(QCoreApplication::translate("SettingsPage", "\352\260\220\353\217\204:", nullptr));
        sensitivityValueLabel->setText(QCoreApplication::translate("SettingsPage", "1.0x", nullptr));
        smoothingCheckBox->setText(QCoreApplication::translate("SettingsPage", "\354\212\244\353\254\264\353\224\251 \354\202\254\354\232\251", nullptr));
        trailCheckBox->setText(QCoreApplication::translate("SettingsPage", "\354\273\244\354\204\234 \352\266\244\354\240\201 \355\221\234\354\213\234", nullptr));
        airmouseStatusLabel->setText(QCoreApplication::translate("SettingsPage", "\354\227\220\354\226\264\353\247\210\354\232\260\354\212\244 \354\203\201\355\203\234:", nullptr));
        airmouseStatusValueLabel->setText(QCoreApplication::translate("SettingsPage", "\353\271\204\355\231\234\354\204\261\355\231\224", nullptr));
        calibrateButton->setText(QCoreApplication::translate("SettingsPage", "\354\272\230\353\246\254\353\270\214\353\240\210\354\235\264\354\205\230", nullptr));
        testAirMouseButton->setText(QCoreApplication::translate("SettingsPage", "\354\227\220\354\226\264\353\247\210\354\232\260\354\212\244 \355\205\214\354\212\244\355\212\270", nullptr));
        saveButton->setText(QCoreApplication::translate("SettingsPage", "\354\240\200\354\236\245", nullptr));
        backButton->setText(QCoreApplication::translate("SettingsPage", "\353\222\244\353\241\234 \352\260\200\352\270\260", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SettingsPage: public Ui_SettingsPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGS_H
