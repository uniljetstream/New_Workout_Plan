/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QHBoxLayout *horizontalLayout;
    QWidget *leftWidget;
    QVBoxLayout *leftLayout;
    QLabel *lblCamera;
    QGroupBox *groupResult;
    QVBoxLayout *resultLayout;
    QLabel *lblScore;
    QLabel *lblFeedback;
    QWidget *rightWidget;
    QVBoxLayout *rightLayout;
    QLabel *lblTitle;
    QGroupBox *groupMode;
    QVBoxLayout *modeLayout;
    QPushButton *btnSquat;
    QPushButton *btnPushup;
    QPushButton *btnLunge;
    QPushButton *btnPlank;
    QLabel *lblSelectedMode;
    QPushButton *btnStart;
    QPushButton *btnStop;
    QLabel *lblStatus;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName("Widget");
        Widget->resize(1280, 720);
        horizontalLayout = new QHBoxLayout(Widget);
        horizontalLayout->setObjectName("horizontalLayout");
        leftWidget = new QWidget(Widget);
        leftWidget->setObjectName("leftWidget");
        leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setObjectName("leftLayout");
        lblCamera = new QLabel(leftWidget);
        lblCamera->setObjectName("lblCamera");
        lblCamera->setMinimumSize(QSize(640, 480));
        lblCamera->setStyleSheet(QString::fromUtf8("background-color: black; border: 2px solid #555;"));
        lblCamera->setAlignment(Qt::AlignCenter);

        leftLayout->addWidget(lblCamera);

        groupResult = new QGroupBox(leftWidget);
        groupResult->setObjectName("groupResult");
        resultLayout = new QVBoxLayout(groupResult);
        resultLayout->setObjectName("resultLayout");
        lblScore = new QLabel(groupResult);
        lblScore->setObjectName("lblScore");
        lblScore->setStyleSheet(QString::fromUtf8("font-size: 18pt; font-weight: bold;"));

        resultLayout->addWidget(lblScore);

        lblFeedback = new QLabel(groupResult);
        lblFeedback->setObjectName("lblFeedback");
        lblFeedback->setWordWrap(true);

        resultLayout->addWidget(lblFeedback);


        leftLayout->addWidget(groupResult);


        horizontalLayout->addWidget(leftWidget);

        rightWidget = new QWidget(Widget);
        rightWidget->setObjectName("rightWidget");
        rightWidget->setMinimumSize(QSize(400, 0));
        rightWidget->setMaximumSize(QSize(450, 16777215));
        rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->setObjectName("rightLayout");
        lblTitle = new QLabel(rightWidget);
        lblTitle->setObjectName("lblTitle");
        lblTitle->setStyleSheet(QString::fromUtf8("font-size: 24pt; font-weight: bold;"));
        lblTitle->setAlignment(Qt::AlignCenter);

        rightLayout->addWidget(lblTitle);

        groupMode = new QGroupBox(rightWidget);
        groupMode->setObjectName("groupMode");
        modeLayout = new QVBoxLayout(groupMode);
        modeLayout->setObjectName("modeLayout");
        btnSquat = new QPushButton(groupMode);
        btnSquat->setObjectName("btnSquat");
        btnSquat->setMinimumSize(QSize(0, 60));
        btnSquat->setCheckable(true);
        btnSquat->setStyleSheet(QString::fromUtf8("\n"
"QPushButton {\n"
"    background-color: #f0f0f0;\n"
"    border: 2px solid #999;\n"
"    border-radius: 8px;\n"
"    padding: 10px;\n"
"    font-size: 14pt;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #e0e0e0;\n"
"}\n"
"QPushButton:checked {\n"
"    background-color: #4CAF50;\n"
"    color: white;\n"
"    border: 2px solid #45a049;\n"
"}\n"
"            "));

        modeLayout->addWidget(btnSquat);

        btnPushup = new QPushButton(groupMode);
        btnPushup->setObjectName("btnPushup");
        btnPushup->setMinimumSize(QSize(0, 60));
        btnPushup->setCheckable(true);
        btnPushup->setStyleSheet(QString::fromUtf8("\n"
"QPushButton {\n"
"    background-color: #f0f0f0;\n"
"    border: 2px solid #999;\n"
"    border-radius: 8px;\n"
"    padding: 10px;\n"
"    font-size: 14pt;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #e0e0e0;\n"
"}\n"
"QPushButton:checked {\n"
"    background-color: #4CAF50;\n"
"    color: white;\n"
"    border: 2px solid #45a049;\n"
"}\n"
"            "));

        modeLayout->addWidget(btnPushup);

        btnLunge = new QPushButton(groupMode);
        btnLunge->setObjectName("btnLunge");
        btnLunge->setMinimumSize(QSize(0, 60));
        btnLunge->setCheckable(true);
        btnLunge->setStyleSheet(QString::fromUtf8("\n"
"QPushButton {\n"
"    background-color: #f0f0f0;\n"
"    border: 2px solid #999;\n"
"    border-radius: 8px;\n"
"    padding: 10px;\n"
"    font-size: 14pt;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #e0e0e0;\n"
"}\n"
"QPushButton:checked {\n"
"    background-color: #4CAF50;\n"
"    color: white;\n"
"    border: 2px solid #45a049;\n"
"}\n"
"            "));

        modeLayout->addWidget(btnLunge);

        btnPlank = new QPushButton(groupMode);
        btnPlank->setObjectName("btnPlank");
        btnPlank->setMinimumSize(QSize(0, 60));
        btnPlank->setCheckable(true);
        btnPlank->setStyleSheet(QString::fromUtf8("\n"
"QPushButton {\n"
"    background-color: #f0f0f0;\n"
"    border: 2px solid #999;\n"
"    border-radius: 8px;\n"
"    padding: 10px;\n"
"    font-size: 14pt;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #e0e0e0;\n"
"}\n"
"QPushButton:checked {\n"
"    background-color: #4CAF50;\n"
"    color: white;\n"
"    border: 2px solid #45a049;\n"
"}\n"
"            "));

        modeLayout->addWidget(btnPlank);


        rightLayout->addWidget(groupMode);

        lblSelectedMode = new QLabel(rightWidget);
        lblSelectedMode->setObjectName("lblSelectedMode");
        lblSelectedMode->setStyleSheet(QString::fromUtf8("\n"
"background-color: #2196F3;\n"
"color: white;\n"
"padding: 15px;\n"
"border-radius: 8px;\n"
"font-size: 16pt;\n"
"font-weight: bold;\n"
"         "));
        lblSelectedMode->setAlignment(Qt::AlignCenter);

        rightLayout->addWidget(lblSelectedMode);

        btnStart = new QPushButton(rightWidget);
        btnStart->setObjectName("btnStart");
        btnStart->setMinimumSize(QSize(0, 80));
        btnStart->setEnabled(false);
        btnStart->setStyleSheet(QString::fromUtf8("\n"
"QPushButton {\n"
"    background-color: #4CAF50;\n"
"    color: white;\n"
"    border: none;\n"
"    border-radius: 8px;\n"
"    font-size: 16pt;\n"
"    font-weight: bold;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #45a049;\n"
"}\n"
"QPushButton:disabled {\n"
"    background-color: #cccccc;\n"
"    color: #666666;\n"
"}\n"
"         "));

        rightLayout->addWidget(btnStart);

        btnStop = new QPushButton(rightWidget);
        btnStop->setObjectName("btnStop");
        btnStop->setMinimumSize(QSize(0, 80));
        btnStop->setEnabled(false);
        btnStop->setStyleSheet(QString::fromUtf8("\n"
"QPushButton {\n"
"    background-color: #f44336;\n"
"    color: white;\n"
"    border: none;\n"
"    border-radius: 8px;\n"
"    font-size: 16pt;\n"
"    font-weight: bold;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #da190b;\n"
"}\n"
"QPushButton:disabled {\n"
"    background-color: #cccccc;\n"
"    color: #666666;\n"
"}\n"
"         "));

        rightLayout->addWidget(btnStop);

        lblStatus = new QLabel(rightWidget);
        lblStatus->setObjectName("lblStatus");
        lblStatus->setStyleSheet(QString::fromUtf8("\n"
"background-color: #f0f0f0;\n"
"padding: 10px;\n"
"border-radius: 5px;\n"
"font-size: 12pt;\n"
"         "));
        lblStatus->setAlignment(Qt::AlignCenter);

        rightLayout->addWidget(lblStatus);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        rightLayout->addItem(verticalSpacer);


        horizontalLayout->addWidget(rightWidget);


        retranslateUi(Widget);

        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QCoreApplication::translate("Widget", "WatchTower", nullptr));
        lblCamera->setText(QCoreApplication::translate("Widget", "\354\271\264\353\251\224\353\235\274 \353\214\200\352\270\260 \354\244\221...", nullptr));
        groupResult->setTitle(QCoreApplication::translate("Widget", "\354\213\244\354\213\234\352\260\204 \353\266\204\354\204\235 \352\262\260\352\263\274", nullptr));
        lblScore->setText(QCoreApplication::translate("Widget", "\354\240\220\354\210\230: --", nullptr));
        lblFeedback->setText(QCoreApplication::translate("Widget", "\355\224\274\353\223\234\353\260\261: \354\232\264\353\217\231\354\235\204 \354\213\234\354\236\221\355\225\230\354\204\270\354\232\224", nullptr));
        lblTitle->setText(QCoreApplication::translate("Widget", "WatchTower", nullptr));
        groupMode->setTitle(QCoreApplication::translate("Widget", "\354\232\264\353\217\231 \353\252\250\353\223\234 \354\204\240\355\203\235", nullptr));
        btnSquat->setText(QCoreApplication::translate("Widget", "\354\212\244\354\277\274\355\212\270 (Squat)", nullptr));
        btnPushup->setText(QCoreApplication::translate("Widget", "\355\221\270\354\213\234\354\227\205 (Push-up)", nullptr));
        btnLunge->setText(QCoreApplication::translate("Widget", "\353\237\260\354\247\200 (Lunge)", nullptr));
        btnPlank->setText(QCoreApplication::translate("Widget", "\355\224\214\353\236\255\355\201\254 (Plank)", nullptr));
        lblSelectedMode->setText(QCoreApplication::translate("Widget", "\354\204\240\355\203\235\353\220\234 \354\232\264\353\217\231: \354\227\206\354\235\214", nullptr));
        btnStart->setText(QCoreApplication::translate("Widget", "\354\232\264\353\217\231 \354\213\234\354\236\221", nullptr));
        btnStop->setText(QCoreApplication::translate("Widget", "\354\232\264\353\217\231 \354\240\225\354\247\200", nullptr));
        lblStatus->setText(QCoreApplication::translate("Widget", "\354\203\201\355\203\234: \353\214\200\352\270\260 \354\244\221", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H
