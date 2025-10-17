/********************************************************************************
** Form generated from reading UI file 'workout.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WORKOUT_H
#define UI_WORKOUT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "videoframewidget.h"

QT_BEGIN_NAMESPACE

class Ui_WorkoutPage
{
public:
    QVBoxLayout *verticalLayout_main;
    QHBoxLayout *horizontalLayout_header;
    QLabel *exerciseTitleLabel;
    QSpacerItem *horizontalSpacer_header;
    QLabel *timerLabel;
    QHBoxLayout *horizontalLayout_content;
    QVBoxLayout *verticalLayout_left;
    QGroupBox *statusGroupBox;
    QVBoxLayout *verticalLayout_status;
    QLabel *scoreLabel;
    QLabel *routineInfoLabel;
    QLabel *currentPoseLabel;
    QLabel *feedbackLabel;
    QFrame *line;
    QLabel *heartRateLabel;
    QLabel *repCountLabel;
    QSpacerItem *verticalSpacer_status;
    QGroupBox *controlGroupBox;
    QVBoxLayout *verticalLayout_control;
    QPushButton *skipButton;
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *backButton;
    VideoFrameWidget *videoWidget;

    void setupUi(QWidget *WorkoutPage)
    {
        if (WorkoutPage->objectName().isEmpty())
            WorkoutPage->setObjectName(QString::fromUtf8("WorkoutPage"));
        WorkoutPage->resize(800, 600);
        verticalLayout_main = new QVBoxLayout(WorkoutPage);
        verticalLayout_main->setObjectName(QString::fromUtf8("verticalLayout_main"));
        horizontalLayout_header = new QHBoxLayout();
        horizontalLayout_header->setObjectName(QString::fromUtf8("horizontalLayout_header"));
        exerciseTitleLabel = new QLabel(WorkoutPage);
        exerciseTitleLabel->setObjectName(QString::fromUtf8("exerciseTitleLabel"));
        QFont font;
        font.setPointSize(20);
        font.setBold(true);
        font.setWeight(75);
        exerciseTitleLabel->setFont(font);

        horizontalLayout_header->addWidget(exerciseTitleLabel);

        horizontalSpacer_header = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_header->addItem(horizontalSpacer_header);

        timerLabel = new QLabel(WorkoutPage);
        timerLabel->setObjectName(QString::fromUtf8("timerLabel"));
        timerLabel->setFont(font);

        horizontalLayout_header->addWidget(timerLabel);


        verticalLayout_main->addLayout(horizontalLayout_header);

        horizontalLayout_content = new QHBoxLayout();
        horizontalLayout_content->setObjectName(QString::fromUtf8("horizontalLayout_content"));
        verticalLayout_left = new QVBoxLayout();
        verticalLayout_left->setObjectName(QString::fromUtf8("verticalLayout_left"));
        statusGroupBox = new QGroupBox(WorkoutPage);
        statusGroupBox->setObjectName(QString::fromUtf8("statusGroupBox"));
        QFont font1;
        font1.setPointSize(12);
        font1.setBold(true);
        font1.setWeight(75);
        statusGroupBox->setFont(font1);
        verticalLayout_status = new QVBoxLayout(statusGroupBox);
        verticalLayout_status->setObjectName(QString::fromUtf8("verticalLayout_status"));
        scoreLabel = new QLabel(statusGroupBox);
        scoreLabel->setObjectName(QString::fromUtf8("scoreLabel"));
        QFont font2;
        font2.setPointSize(16);
        font2.setBold(false);
        font2.setWeight(50);
        scoreLabel->setFont(font2);

        verticalLayout_status->addWidget(scoreLabel);

        routineInfoLabel = new QLabel(statusGroupBox);
        routineInfoLabel->setObjectName(QString::fromUtf8("routineInfoLabel"));
        QFont font3;
        font3.setPointSize(14);
        font3.setBold(false);
        font3.setWeight(50);
        routineInfoLabel->setFont(font3);
        routineInfoLabel->setStyleSheet(QString::fromUtf8("color: #2196F3;"));

        verticalLayout_status->addWidget(routineInfoLabel);

        currentPoseLabel = new QLabel(statusGroupBox);
        currentPoseLabel->setObjectName(QString::fromUtf8("currentPoseLabel"));
        QFont font4;
        font4.setPointSize(13);
        font4.setBold(false);
        font4.setWeight(50);
        currentPoseLabel->setFont(font4);
        currentPoseLabel->setStyleSheet(QString::fromUtf8("color: #FF9800; font-weight: bold;"));
        currentPoseLabel->setWordWrap(true);

        verticalLayout_status->addWidget(currentPoseLabel);

        feedbackLabel = new QLabel(statusGroupBox);
        feedbackLabel->setObjectName(QString::fromUtf8("feedbackLabel"));
        feedbackLabel->setWordWrap(true);
        QFont font5;
        font5.setPointSize(12);
        font5.setBold(false);
        font5.setWeight(50);
        feedbackLabel->setFont(font5);

        verticalLayout_status->addWidget(feedbackLabel);

        line = new QFrame(statusGroupBox);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout_status->addWidget(line);

        heartRateLabel = new QLabel(statusGroupBox);
        heartRateLabel->setObjectName(QString::fromUtf8("heartRateLabel"));
        heartRateLabel->setFont(font3);

        verticalLayout_status->addWidget(heartRateLabel);

        repCountLabel = new QLabel(statusGroupBox);
        repCountLabel->setObjectName(QString::fromUtf8("repCountLabel"));
        repCountLabel->setFont(font3);

        verticalLayout_status->addWidget(repCountLabel);

        verticalSpacer_status = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_status->addItem(verticalSpacer_status);


        verticalLayout_left->addWidget(statusGroupBox);

        controlGroupBox = new QGroupBox(WorkoutPage);
        controlGroupBox->setObjectName(QString::fromUtf8("controlGroupBox"));
        controlGroupBox->setFont(font1);
        verticalLayout_control = new QVBoxLayout(controlGroupBox);
        verticalLayout_control->setObjectName(QString::fromUtf8("verticalLayout_control"));
        skipButton = new QPushButton(controlGroupBox);
        skipButton->setObjectName(QString::fromUtf8("skipButton"));
        skipButton->setMinimumSize(QSize(0, 50));
        skipButton->setFont(font3);
        skipButton->setStyleSheet(QString::fromUtf8("background-color: #FF9800; color: white; border-radius: 5px;"));
        skipButton->setVisible(false);

        verticalLayout_control->addWidget(skipButton);

        startButton = new QPushButton(controlGroupBox);
        startButton->setObjectName(QString::fromUtf8("startButton"));
        startButton->setMinimumSize(QSize(0, 50));
        startButton->setFont(font3);
        startButton->setStyleSheet(QString::fromUtf8("background-color: #4CAF50; color: white; border-radius: 5px;"));

        verticalLayout_control->addWidget(startButton);

        stopButton = new QPushButton(controlGroupBox);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));
        stopButton->setMinimumSize(QSize(0, 50));
        stopButton->setFont(font3);
        stopButton->setStyleSheet(QString::fromUtf8("background-color: #f44336; color: white; border-radius: 5px;"));

        verticalLayout_control->addWidget(stopButton);

        backButton = new QPushButton(controlGroupBox);
        backButton->setObjectName(QString::fromUtf8("backButton"));
        backButton->setMinimumSize(QSize(0, 50));
        backButton->setFont(font3);
        backButton->setStyleSheet(QString::fromUtf8("background-color: #808080; color: white; border-radius: 5px;"));

        verticalLayout_control->addWidget(backButton);


        verticalLayout_left->addWidget(controlGroupBox);


        horizontalLayout_content->addLayout(verticalLayout_left);

        videoWidget = new VideoFrameWidget(WorkoutPage);
        videoWidget->setObjectName(QString::fromUtf8("videoWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(videoWidget->sizePolicy().hasHeightForWidth());
        videoWidget->setSizePolicy(sizePolicy);
        videoWidget->setMinimumSize(QSize(400, 400));

        horizontalLayout_content->addWidget(videoWidget);


        verticalLayout_main->addLayout(horizontalLayout_content);


        retranslateUi(WorkoutPage);

        QMetaObject::connectSlotsByName(WorkoutPage);
    } // setupUi

    void retranslateUi(QWidget *WorkoutPage)
    {
        WorkoutPage->setWindowTitle(QCoreApplication::translate("WorkoutPage", "\354\232\264\353\217\231 \354\244\221", nullptr));
        exerciseTitleLabel->setText(QCoreApplication::translate("WorkoutPage", "\354\232\264\353\217\231: --", nullptr));
        timerLabel->setText(QCoreApplication::translate("WorkoutPage", "00:00", nullptr));
        statusGroupBox->setTitle(QCoreApplication::translate("WorkoutPage", "\354\203\201\355\203\234 \354\240\225\353\263\264", nullptr));
        scoreLabel->setText(QCoreApplication::translate("WorkoutPage", "\354\240\220\354\210\230: --", nullptr));
        routineInfoLabel->setText(QString());
        currentPoseLabel->setText(QCoreApplication::translate("WorkoutPage", "\355\230\204\354\236\254 \354\236\220\354\204\270: \353\214\200\352\270\260 \354\244\221...", nullptr));
        feedbackLabel->setText(QCoreApplication::translate("WorkoutPage", "\355\224\274\353\223\234\353\260\261: \353\214\200\352\270\260 \354\244\221...", nullptr));
        heartRateLabel->setText(QCoreApplication::translate("WorkoutPage", "\354\213\254\353\260\225\354\210\230: -- BPM", nullptr));
        repCountLabel->setText(QCoreApplication::translate("WorkoutPage", "\353\260\230\353\263\265 \355\232\237\354\210\230: 0", nullptr));
        controlGroupBox->setTitle(QCoreApplication::translate("WorkoutPage", "\354\240\234\354\226\264", nullptr));
        skipButton->setText(QCoreApplication::translate("WorkoutPage", "\354\235\264 \354\232\264\353\217\231 \354\212\244\355\202\265", nullptr));
        startButton->setText(QCoreApplication::translate("WorkoutPage", "\354\213\234\354\236\221", nullptr));
        stopButton->setText(QCoreApplication::translate("WorkoutPage", "\354\244\221\354\247\200", nullptr));
        backButton->setText(QCoreApplication::translate("WorkoutPage", "\353\222\244\353\241\234 \352\260\200\352\270\260", nullptr));
    } // retranslateUi

};

namespace Ui {
    class WorkoutPage: public Ui_WorkoutPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WORKOUT_H
