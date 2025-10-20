/********************************************************************************
** Form generated from reading UI file 'exercise_selection.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EXERCISE_SELECTION_H
#define UI_EXERCISE_SELECTION_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ExerciseSelectionPage
{
public:
    QVBoxLayout *verticalLayout_main;
    QLabel *titleLabel;
    QSpacerItem *verticalSpacer_1;
    QHBoxLayout *horizontalLayout_scroll;
    QSpacerItem *horizontalSpacer_left;
    QPushButton *scrollUpButton;
    QSpacerItem *horizontalSpacer_right;
    QHBoxLayout *horizontalLayout_content;
    QSpacerItem *horizontalSpacer_left2;
    QScrollArea *exerciseScrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout_exercises;
    QPushButton *squatButton;
    QPushButton *plankButton;
    QPushButton *lungeButton;
    QPushButton *jumpingJackButton;
    QPushButton *mountainClimberButton;
    QPushButton *burpeeButton;
    QPushButton *customButton;
    QSpacerItem *verticalSpacer_bottom;
    QSpacerItem *horizontalSpacer_right2;
    QHBoxLayout *horizontalLayout_scrollDown;
    QSpacerItem *horizontalSpacer_left3;
    QPushButton *scrollDownButton;
    QSpacerItem *horizontalSpacer_right3;
    QSpacerItem *verticalSpacer_2;
    QHBoxLayout *horizontalLayout_back;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *backButton;
    QSpacerItem *horizontalSpacer_4;

    void setupUi(QWidget *ExerciseSelectionPage)
    {
        if (ExerciseSelectionPage->objectName().isEmpty())
            ExerciseSelectionPage->setObjectName(QString::fromUtf8("ExerciseSelectionPage"));
        ExerciseSelectionPage->resize(800, 600);
        verticalLayout_main = new QVBoxLayout(ExerciseSelectionPage);
        verticalLayout_main->setObjectName(QString::fromUtf8("verticalLayout_main"));
        titleLabel = new QLabel(ExerciseSelectionPage);
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

        horizontalLayout_scroll = new QHBoxLayout();
        horizontalLayout_scroll->setObjectName(QString::fromUtf8("horizontalLayout_scroll"));
        horizontalSpacer_left = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_scroll->addItem(horizontalSpacer_left);

        scrollUpButton = new QPushButton(ExerciseSelectionPage);
        scrollUpButton->setObjectName(QString::fromUtf8("scrollUpButton"));
        scrollUpButton->setMinimumSize(QSize(60, 60));
        QFont font1;
        font1.setPointSize(20);
        font1.setBold(true);
        font1.setWeight(75);
        scrollUpButton->setFont(font1);
        scrollUpButton->setStyleSheet(QString::fromUtf8("background-color: #9E9E9E; color: white; border-radius: 30px;"));

        horizontalLayout_scroll->addWidget(scrollUpButton);

        horizontalSpacer_right = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_scroll->addItem(horizontalSpacer_right);


        verticalLayout_main->addLayout(horizontalLayout_scroll);

        horizontalLayout_content = new QHBoxLayout();
        horizontalLayout_content->setObjectName(QString::fromUtf8("horizontalLayout_content"));
        horizontalSpacer_left2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_content->addItem(horizontalSpacer_left2);

        exerciseScrollArea = new QScrollArea(ExerciseSelectionPage);
        exerciseScrollArea->setObjectName(QString::fromUtf8("exerciseScrollArea"));
        exerciseScrollArea->setMinimumSize(QSize(600, 0));
        exerciseScrollArea->setWidgetResizable(true);
        exerciseScrollArea->setStyleSheet(QString::fromUtf8("QScrollArea { border: none; background-color: transparent; }\n"
"QScrollBar:vertical { width: 12px; background: #f0f0f0; }\n"
"QScrollBar::handle:vertical { background: #888; border-radius: 6px; }\n"
"QScrollBar::handle:vertical:hover { background: #555; }"));
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 600, 400));
        verticalLayout_exercises = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout_exercises->setSpacing(15);
        verticalLayout_exercises->setObjectName(QString::fromUtf8("verticalLayout_exercises"));
        squatButton = new QPushButton(scrollAreaWidgetContents);
        squatButton->setObjectName(QString::fromUtf8("squatButton"));
        squatButton->setMinimumSize(QSize(0, 80));
        QFont font2;
        font2.setPointSize(18);
        font2.setBold(true);
        font2.setWeight(75);
        squatButton->setFont(font2);
        squatButton->setStyleSheet(QString::fromUtf8("background-color: #4ECDC4; color: white; border-radius: 10px; text-align: left; padding-left: 30px;"));

        verticalLayout_exercises->addWidget(squatButton);

        plankButton = new QPushButton(scrollAreaWidgetContents);
        plankButton->setObjectName(QString::fromUtf8("plankButton"));
        plankButton->setMinimumSize(QSize(0, 80));
        plankButton->setFont(font2);
        plankButton->setStyleSheet(QString::fromUtf8("background-color: #F38181; color: white; border-radius: 10px; text-align: left; padding-left: 30px;"));

        verticalLayout_exercises->addWidget(plankButton);

        lungeButton = new QPushButton(scrollAreaWidgetContents);
        lungeButton->setObjectName(QString::fromUtf8("lungeButton"));
        lungeButton->setMinimumSize(QSize(0, 80));
        lungeButton->setFont(font2);
        lungeButton->setStyleSheet(QString::fromUtf8("background-color: #A8E6CF; color: white; border-radius: 10px; text-align: left; padding-left: 30px;"));

        verticalLayout_exercises->addWidget(lungeButton);

        jumpingJackButton = new QPushButton(scrollAreaWidgetContents);
        jumpingJackButton->setObjectName(QString::fromUtf8("jumpingJackButton"));
        jumpingJackButton->setMinimumSize(QSize(0, 80));
        jumpingJackButton->setFont(font2);
        jumpingJackButton->setStyleSheet(QString::fromUtf8("background-color: #FFD3B6; color: white; border-radius: 10px; text-align: left; padding-left: 30px;"));

        verticalLayout_exercises->addWidget(jumpingJackButton);

        mountainClimberButton = new QPushButton(scrollAreaWidgetContents);
        mountainClimberButton->setObjectName(QString::fromUtf8("mountainClimberButton"));
        mountainClimberButton->setMinimumSize(QSize(0, 80));
        mountainClimberButton->setFont(font2);
        mountainClimberButton->setStyleSheet(QString::fromUtf8("background-color: #FFAAA5; color: white; border-radius: 10px; text-align: left; padding-left: 30px;"));

        verticalLayout_exercises->addWidget(mountainClimberButton);

        burpeeButton = new QPushButton(scrollAreaWidgetContents);
        burpeeButton->setObjectName(QString::fromUtf8("burpeeButton"));
        burpeeButton->setMinimumSize(QSize(0, 80));
        burpeeButton->setFont(font2);
        burpeeButton->setStyleSheet(QString::fromUtf8("background-color: #B39CD0; color: white; border-radius: 10px; text-align: left; padding-left: 30px;"));

        verticalLayout_exercises->addWidget(burpeeButton);

        customButton = new QPushButton(scrollAreaWidgetContents);
        customButton->setObjectName(QString::fromUtf8("customButton"));
        customButton->setMinimumSize(QSize(0, 80));
        customButton->setFont(font2);
        customButton->setStyleSheet(QString::fromUtf8("background-color: #C7CEEA; color: white; border-radius: 10px; text-align: left; padding-left: 30px;"));

        verticalLayout_exercises->addWidget(customButton);

        verticalSpacer_bottom = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_exercises->addItem(verticalSpacer_bottom);

        exerciseScrollArea->setWidget(scrollAreaWidgetContents);

        horizontalLayout_content->addWidget(exerciseScrollArea);

        horizontalSpacer_right2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_content->addItem(horizontalSpacer_right2);


        verticalLayout_main->addLayout(horizontalLayout_content);

        horizontalLayout_scrollDown = new QHBoxLayout();
        horizontalLayout_scrollDown->setObjectName(QString::fromUtf8("horizontalLayout_scrollDown"));
        horizontalSpacer_left3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_scrollDown->addItem(horizontalSpacer_left3);

        scrollDownButton = new QPushButton(ExerciseSelectionPage);
        scrollDownButton->setObjectName(QString::fromUtf8("scrollDownButton"));
        scrollDownButton->setMinimumSize(QSize(60, 60));
        scrollDownButton->setFont(font1);
        scrollDownButton->setStyleSheet(QString::fromUtf8("background-color: #9E9E9E; color: white; border-radius: 30px;"));

        horizontalLayout_scrollDown->addWidget(scrollDownButton);

        horizontalSpacer_right3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_scrollDown->addItem(horizontalSpacer_right3);


        verticalLayout_main->addLayout(horizontalLayout_scrollDown);

        verticalSpacer_2 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_2);

        horizontalLayout_back = new QHBoxLayout();
        horizontalLayout_back->setObjectName(QString::fromUtf8("horizontalLayout_back"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_back->addItem(horizontalSpacer_3);

        backButton = new QPushButton(ExerciseSelectionPage);
        backButton->setObjectName(QString::fromUtf8("backButton"));
        backButton->setMinimumSize(QSize(200, 60));
        QFont font3;
        font3.setPointSize(14);
        backButton->setFont(font3);
        backButton->setStyleSheet(QString::fromUtf8("background-color: #808080; color: white; border-radius: 10px;"));

        horizontalLayout_back->addWidget(backButton);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_back->addItem(horizontalSpacer_4);


        verticalLayout_main->addLayout(horizontalLayout_back);


        retranslateUi(ExerciseSelectionPage);

        QMetaObject::connectSlotsByName(ExerciseSelectionPage);
    } // setupUi

    void retranslateUi(QWidget *ExerciseSelectionPage)
    {
        ExerciseSelectionPage->setWindowTitle(QCoreApplication::translate("ExerciseSelectionPage", "\354\232\264\353\217\231 \354\204\240\355\203\235", nullptr));
        titleLabel->setText(QCoreApplication::translate("ExerciseSelectionPage", "\354\232\264\353\217\231 \354\204\240\355\203\235", nullptr));
        scrollUpButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\342\226\262", nullptr));
        squatButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\354\212\244\354\277\274\355\212\270", nullptr));
        plankButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\355\224\214\353\236\255\355\201\254", nullptr));
        lungeButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\353\237\260\354\247\200", nullptr));
        jumpingJackButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\354\240\220\355\225\221\354\236\255", nullptr));
        mountainClimberButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\353\247\210\354\232\264\355\213\264 \355\201\264\353\235\274\354\235\264\353\250\270", nullptr));
        burpeeButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\353\262\204\355\224\274", nullptr));
        customButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\354\202\254\354\232\251\354\236\220 \354\240\225\354\235\230", nullptr));
        scrollDownButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\342\226\274", nullptr));
        backButton->setText(QCoreApplication::translate("ExerciseSelectionPage", "\353\222\244\353\241\234 \352\260\200\352\270\260", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ExerciseSelectionPage: public Ui_ExerciseSelectionPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EXERCISE_SELECTION_H
