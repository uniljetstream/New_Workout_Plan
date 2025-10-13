/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QGroupBox *connectionGroupBox;
    QHBoxLayout *horizontalLayout_conn;
    QLabel *label;
    QLineEdit *brokerLineEdit;
    QLabel *label_2;
    QSpinBox *portSpinBox;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QLabel *statusLabel;
    QTabWidget *tabWidget;
    QWidget *airMouseTab;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_2;
    QGroupBox *airMouseControlGroup;
    QVBoxLayout *verticalLayout_3;
    QPushButton *airMouseModeButton;
    QPushButton *sensorModeButton;
    QPushButton *calibrateButton;
    QPushButton *resetCursorButton;
    QPushButton *testCursorButton;
    QFrame *line;
    QLabel *label_3;
    QSlider *cursorSensitivitySlider;
    QLabel *cursorSensitivityLabel;
    QCheckBox *cursorSmoothingCheckBox;
    QCheckBox *showTrailCheckBox;
    QSpacerItem *verticalSpacer;
    QLabel *airMouseDataLabel;
    QWidget *cursorCanvasWidget;
    QWidget *sensorDataTab;
    QVBoxLayout *verticalLayout_4;
    QHBoxLayout *horizontalLayout_3;
    QGroupBox *joystickGroupBox;
    QVBoxLayout *verticalLayout_5;
    QLabel *accelXLabel;
    QLabel *accelYLabel;
    QLabel *accelZLabel;
    QLabel *gyroXLabel;
    QLabel *gyroYLabel;
    QLabel *gyroZLabel;
    QLabel *joystickStatusLabel;
    QGroupBox *watchGroupBox;
    QVBoxLayout *verticalLayout_6;
    QLabel *heartrateLabel;
    QLabel *watchStatusLabel;
    QWidget *watchtowerTab;
    QVBoxLayout *verticalLayout_7;
    QGroupBox *commandGroupBox;
    QGridLayout *gridLayout;
    QLabel *label_4;
    QComboBox *modeComboBox;
    QPushButton *selectModeButton;
    QPushButton *startButton;
    QPushButton *stopButton;
    QGroupBox *watchtowerGroupBox;
    QVBoxLayout *verticalLayout_8;
    QLabel *watchtowerResponseLabel;
    QGroupBox *logGroupBox;
    QVBoxLayout *verticalLayout_9;
    QTextEdit *logTextEdit;
    QPushButton *clearLogButton;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1000, 800);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        connectionGroupBox = new QGroupBox(centralwidget);
        connectionGroupBox->setObjectName(QString::fromUtf8("connectionGroupBox"));
        horizontalLayout_conn = new QHBoxLayout(connectionGroupBox);
        horizontalLayout_conn->setObjectName(QString::fromUtf8("horizontalLayout_conn"));
        label = new QLabel(connectionGroupBox);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout_conn->addWidget(label);

        brokerLineEdit = new QLineEdit(connectionGroupBox);
        brokerLineEdit->setObjectName(QString::fromUtf8("brokerLineEdit"));

        horizontalLayout_conn->addWidget(brokerLineEdit);

        label_2 = new QLabel(connectionGroupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_conn->addWidget(label_2);

        portSpinBox = new QSpinBox(connectionGroupBox);
        portSpinBox->setObjectName(QString::fromUtf8("portSpinBox"));
        portSpinBox->setMinimum(1);
        portSpinBox->setMaximum(65535);
        portSpinBox->setValue(1883);

        horizontalLayout_conn->addWidget(portSpinBox);

        connectButton = new QPushButton(connectionGroupBox);
        connectButton->setObjectName(QString::fromUtf8("connectButton"));

        horizontalLayout_conn->addWidget(connectButton);

        disconnectButton = new QPushButton(connectionGroupBox);
        disconnectButton->setObjectName(QString::fromUtf8("disconnectButton"));

        horizontalLayout_conn->addWidget(disconnectButton);

        statusLabel = new QLabel(connectionGroupBox);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));

        horizontalLayout_conn->addWidget(statusLabel);


        verticalLayout->addWidget(connectionGroupBox);

        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        airMouseTab = new QWidget();
        airMouseTab->setObjectName(QString::fromUtf8("airMouseTab"));
        verticalLayout_2 = new QVBoxLayout(airMouseTab);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        airMouseControlGroup = new QGroupBox(airMouseTab);
        airMouseControlGroup->setObjectName(QString::fromUtf8("airMouseControlGroup"));
        verticalLayout_3 = new QVBoxLayout(airMouseControlGroup);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        airMouseModeButton = new QPushButton(airMouseControlGroup);
        airMouseModeButton->setObjectName(QString::fromUtf8("airMouseModeButton"));

        verticalLayout_3->addWidget(airMouseModeButton);

        sensorModeButton = new QPushButton(airMouseControlGroup);
        sensorModeButton->setObjectName(QString::fromUtf8("sensorModeButton"));

        verticalLayout_3->addWidget(sensorModeButton);

        calibrateButton = new QPushButton(airMouseControlGroup);
        calibrateButton->setObjectName(QString::fromUtf8("calibrateButton"));

        verticalLayout_3->addWidget(calibrateButton);

        resetCursorButton = new QPushButton(airMouseControlGroup);
        resetCursorButton->setObjectName(QString::fromUtf8("resetCursorButton"));

        verticalLayout_3->addWidget(resetCursorButton);

        testCursorButton = new QPushButton(airMouseControlGroup);
        testCursorButton->setObjectName(QString::fromUtf8("testCursorButton"));
        testCursorButton->setStyleSheet(QString::fromUtf8("background-color: orange; color: white; font-weight: bold;"));

        verticalLayout_3->addWidget(testCursorButton);

        line = new QFrame(airMouseControlGroup);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout_3->addWidget(line);

        label_3 = new QLabel(airMouseControlGroup);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        verticalLayout_3->addWidget(label_3);

        cursorSensitivitySlider = new QSlider(airMouseControlGroup);
        cursorSensitivitySlider->setObjectName(QString::fromUtf8("cursorSensitivitySlider"));
        cursorSensitivitySlider->setMinimum(1);
        cursorSensitivitySlider->setMaximum(50);
        cursorSensitivitySlider->setValue(10);
        cursorSensitivitySlider->setOrientation(Qt::Horizontal);

        verticalLayout_3->addWidget(cursorSensitivitySlider);

        cursorSensitivityLabel = new QLabel(airMouseControlGroup);
        cursorSensitivityLabel->setObjectName(QString::fromUtf8("cursorSensitivityLabel"));

        verticalLayout_3->addWidget(cursorSensitivityLabel);

        cursorSmoothingCheckBox = new QCheckBox(airMouseControlGroup);
        cursorSmoothingCheckBox->setObjectName(QString::fromUtf8("cursorSmoothingCheckBox"));
        cursorSmoothingCheckBox->setChecked(true);

        verticalLayout_3->addWidget(cursorSmoothingCheckBox);

        showTrailCheckBox = new QCheckBox(airMouseControlGroup);
        showTrailCheckBox->setObjectName(QString::fromUtf8("showTrailCheckBox"));
        showTrailCheckBox->setChecked(true);

        verticalLayout_3->addWidget(showTrailCheckBox);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);

        airMouseDataLabel = new QLabel(airMouseControlGroup);
        airMouseDataLabel->setObjectName(QString::fromUtf8("airMouseDataLabel"));

        verticalLayout_3->addWidget(airMouseDataLabel);


        horizontalLayout_2->addWidget(airMouseControlGroup);

        cursorCanvasWidget = new QWidget(airMouseTab);
        cursorCanvasWidget->setObjectName(QString::fromUtf8("cursorCanvasWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(cursorCanvasWidget->sizePolicy().hasHeightForWidth());
        cursorCanvasWidget->setSizePolicy(sizePolicy);

        horizontalLayout_2->addWidget(cursorCanvasWidget);


        verticalLayout_2->addLayout(horizontalLayout_2);

        tabWidget->addTab(airMouseTab, QString());
        sensorDataTab = new QWidget();
        sensorDataTab->setObjectName(QString::fromUtf8("sensorDataTab"));
        verticalLayout_4 = new QVBoxLayout(sensorDataTab);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        joystickGroupBox = new QGroupBox(sensorDataTab);
        joystickGroupBox->setObjectName(QString::fromUtf8("joystickGroupBox"));
        verticalLayout_5 = new QVBoxLayout(joystickGroupBox);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        accelXLabel = new QLabel(joystickGroupBox);
        accelXLabel->setObjectName(QString::fromUtf8("accelXLabel"));

        verticalLayout_5->addWidget(accelXLabel);

        accelYLabel = new QLabel(joystickGroupBox);
        accelYLabel->setObjectName(QString::fromUtf8("accelYLabel"));

        verticalLayout_5->addWidget(accelYLabel);

        accelZLabel = new QLabel(joystickGroupBox);
        accelZLabel->setObjectName(QString::fromUtf8("accelZLabel"));

        verticalLayout_5->addWidget(accelZLabel);

        gyroXLabel = new QLabel(joystickGroupBox);
        gyroXLabel->setObjectName(QString::fromUtf8("gyroXLabel"));

        verticalLayout_5->addWidget(gyroXLabel);

        gyroYLabel = new QLabel(joystickGroupBox);
        gyroYLabel->setObjectName(QString::fromUtf8("gyroYLabel"));

        verticalLayout_5->addWidget(gyroYLabel);

        gyroZLabel = new QLabel(joystickGroupBox);
        gyroZLabel->setObjectName(QString::fromUtf8("gyroZLabel"));

        verticalLayout_5->addWidget(gyroZLabel);

        joystickStatusLabel = new QLabel(joystickGroupBox);
        joystickStatusLabel->setObjectName(QString::fromUtf8("joystickStatusLabel"));

        verticalLayout_5->addWidget(joystickStatusLabel);


        horizontalLayout_3->addWidget(joystickGroupBox);

        watchGroupBox = new QGroupBox(sensorDataTab);
        watchGroupBox->setObjectName(QString::fromUtf8("watchGroupBox"));
        verticalLayout_6 = new QVBoxLayout(watchGroupBox);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        heartrateLabel = new QLabel(watchGroupBox);
        heartrateLabel->setObjectName(QString::fromUtf8("heartrateLabel"));
        QFont font;
        font.setPointSize(16);
        font.setBold(true);
        font.setWeight(75);
        heartrateLabel->setFont(font);

        verticalLayout_6->addWidget(heartrateLabel);

        watchStatusLabel = new QLabel(watchGroupBox);
        watchStatusLabel->setObjectName(QString::fromUtf8("watchStatusLabel"));

        verticalLayout_6->addWidget(watchStatusLabel);


        horizontalLayout_3->addWidget(watchGroupBox);


        verticalLayout_4->addLayout(horizontalLayout_3);

        tabWidget->addTab(sensorDataTab, QString());
        watchtowerTab = new QWidget();
        watchtowerTab->setObjectName(QString::fromUtf8("watchtowerTab"));
        verticalLayout_7 = new QVBoxLayout(watchtowerTab);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        commandGroupBox = new QGroupBox(watchtowerTab);
        commandGroupBox->setObjectName(QString::fromUtf8("commandGroupBox"));
        commandGroupBox->setEnabled(false);
        gridLayout = new QGridLayout(commandGroupBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_4 = new QLabel(commandGroupBox);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 0, 0, 1, 1);

        modeComboBox = new QComboBox(commandGroupBox);
        modeComboBox->setObjectName(QString::fromUtf8("modeComboBox"));

        gridLayout->addWidget(modeComboBox, 0, 1, 1, 1);

        selectModeButton = new QPushButton(commandGroupBox);
        selectModeButton->setObjectName(QString::fromUtf8("selectModeButton"));

        gridLayout->addWidget(selectModeButton, 0, 2, 1, 1);

        startButton = new QPushButton(commandGroupBox);
        startButton->setObjectName(QString::fromUtf8("startButton"));
        startButton->setStyleSheet(QString::fromUtf8("background-color: green; color: white; font-weight: bold;"));

        gridLayout->addWidget(startButton, 1, 0, 1, 1);

        stopButton = new QPushButton(commandGroupBox);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));
        stopButton->setStyleSheet(QString::fromUtf8("background-color: red; color: white; font-weight: bold;"));

        gridLayout->addWidget(stopButton, 1, 1, 1, 1);


        verticalLayout_7->addWidget(commandGroupBox);

        watchtowerGroupBox = new QGroupBox(watchtowerTab);
        watchtowerGroupBox->setObjectName(QString::fromUtf8("watchtowerGroupBox"));
        verticalLayout_8 = new QVBoxLayout(watchtowerGroupBox);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        watchtowerResponseLabel = new QLabel(watchtowerGroupBox);
        watchtowerResponseLabel->setObjectName(QString::fromUtf8("watchtowerResponseLabel"));
        watchtowerResponseLabel->setWordWrap(true);

        verticalLayout_8->addWidget(watchtowerResponseLabel);


        verticalLayout_7->addWidget(watchtowerGroupBox);

        tabWidget->addTab(watchtowerTab, QString());

        verticalLayout->addWidget(tabWidget);

        logGroupBox = new QGroupBox(centralwidget);
        logGroupBox->setObjectName(QString::fromUtf8("logGroupBox"));
        verticalLayout_9 = new QVBoxLayout(logGroupBox);
        verticalLayout_9->setObjectName(QString::fromUtf8("verticalLayout_9"));
        logTextEdit = new QTextEdit(logGroupBox);
        logTextEdit->setObjectName(QString::fromUtf8("logTextEdit"));
        logTextEdit->setMaximumSize(QSize(16777215, 150));
        logTextEdit->setReadOnly(true);

        verticalLayout_9->addWidget(logTextEdit);

        clearLogButton = new QPushButton(logGroupBox);
        clearLogButton->setObjectName(QString::fromUtf8("clearLogButton"));

        verticalLayout_9->addWidget(clearLogButton);


        verticalLayout->addWidget(logGroupBox);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1000, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MPU6050 & WatchTower Unified Test App", nullptr));
        connectionGroupBox->setTitle(QCoreApplication::translate("MainWindow", "MQTT Connection", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Broker:", nullptr));
        brokerLineEdit->setText(QCoreApplication::translate("MainWindow", "localhost", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Port:", nullptr));
        connectButton->setText(QCoreApplication::translate("MainWindow", "Connect", nullptr));
        disconnectButton->setText(QCoreApplication::translate("MainWindow", "Disconnect", nullptr));
        statusLabel->setText(QCoreApplication::translate("MainWindow", "Disconnected", nullptr));
        airMouseControlGroup->setTitle(QCoreApplication::translate("MainWindow", "AirMouse Control", nullptr));
        airMouseModeButton->setText(QCoreApplication::translate("MainWindow", "Enable AirMouse Mode", nullptr));
        sensorModeButton->setText(QCoreApplication::translate("MainWindow", "Sensor Mode", nullptr));
        calibrateButton->setText(QCoreApplication::translate("MainWindow", "Calibrate", nullptr));
        resetCursorButton->setText(QCoreApplication::translate("MainWindow", "Reset Cursor", nullptr));
        testCursorButton->setText(QCoreApplication::translate("MainWindow", "Start Test (Circle)", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Sensitivity:", nullptr));
        cursorSensitivityLabel->setText(QCoreApplication::translate("MainWindow", "1.0x", nullptr));
        cursorSmoothingCheckBox->setText(QCoreApplication::translate("MainWindow", "Enable Smoothing", nullptr));
        showTrailCheckBox->setText(QCoreApplication::translate("MainWindow", "Show Trail", nullptr));
        airMouseDataLabel->setText(QCoreApplication::translate("MainWindow", "Mouse X: ---\n"
"Mouse Y: ---\n"
"Scroll: ---", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(airMouseTab), QCoreApplication::translate("MainWindow", "AirMouse Test", nullptr));
        joystickGroupBox->setTitle(QCoreApplication::translate("MainWindow", "Joystick (MPU6050) Data", nullptr));
        accelXLabel->setText(QCoreApplication::translate("MainWindow", "Accel X: ---", nullptr));
        accelYLabel->setText(QCoreApplication::translate("MainWindow", "Accel Y: ---", nullptr));
        accelZLabel->setText(QCoreApplication::translate("MainWindow", "Accel Z: ---", nullptr));
        gyroXLabel->setText(QCoreApplication::translate("MainWindow", "Gyro X: ---", nullptr));
        gyroYLabel->setText(QCoreApplication::translate("MainWindow", "Gyro Y: ---", nullptr));
        gyroZLabel->setText(QCoreApplication::translate("MainWindow", "Gyro Z: ---", nullptr));
        joystickStatusLabel->setText(QCoreApplication::translate("MainWindow", "Joystick: ---", nullptr));
        watchGroupBox->setTitle(QCoreApplication::translate("MainWindow", "Smart Watch Data", nullptr));
        heartrateLabel->setText(QCoreApplication::translate("MainWindow", "Heart Rate: ---", nullptr));
        watchStatusLabel->setText(QCoreApplication::translate("MainWindow", "Watch: ---", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(sensorDataTab), QCoreApplication::translate("MainWindow", "Sensor Data", nullptr));
        commandGroupBox->setTitle(QCoreApplication::translate("MainWindow", "WatchTower Commands", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Exercise Mode:", nullptr));
        selectModeButton->setText(QCoreApplication::translate("MainWindow", "Select Mode", nullptr));
        startButton->setText(QCoreApplication::translate("MainWindow", "Start Workout", nullptr));
        stopButton->setText(QCoreApplication::translate("MainWindow", "Stop Workout", nullptr));
        watchtowerGroupBox->setTitle(QCoreApplication::translate("MainWindow", "WatchTower Response", nullptr));
        watchtowerResponseLabel->setText(QCoreApplication::translate("MainWindow", "No response yet", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(watchtowerTab), QCoreApplication::translate("MainWindow", "WatchTower Commands", nullptr));
        logGroupBox->setTitle(QCoreApplication::translate("MainWindow", "Message Log", nullptr));
        clearLogButton->setText(QCoreApplication::translate("MainWindow", "Clear Log", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
