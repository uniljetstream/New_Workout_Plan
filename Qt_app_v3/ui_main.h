/********************************************************************************
** Form generated from reading UI file 'main.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAIN_H
#define UI_MAIN_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainMenuPage
{
public:
    QVBoxLayout *verticalLayout_main;
    QSpacerItem *verticalSpacer_top;
    QLabel *titleLabel;
    QSpacerItem *verticalSpacer_1;
    QHBoxLayout *horizontalLayout_buttons;
    QSpacerItem *horizontalSpacer_left;
    QVBoxLayout *verticalLayout_buttons;
    QPushButton *exerciseSelectButton;
    QSpacerItem *verticalSpacer_2;
    QPushButton *settingsButton;
    QSpacerItem *horizontalSpacer_right;
    QSpacerItem *verticalSpacer_bottom;
    QLabel *statusLabel;

    void setupUi(QWidget *MainMenuPage)
    {
        if (MainMenuPage->objectName().isEmpty())
            MainMenuPage->setObjectName(QString::fromUtf8("MainMenuPage"));
        MainMenuPage->resize(800, 600);
        verticalLayout_main = new QVBoxLayout(MainMenuPage);
        verticalLayout_main->setObjectName(QString::fromUtf8("verticalLayout_main"));
        verticalSpacer_top = new QSpacerItem(20, 100, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_top);

        titleLabel = new QLabel(MainMenuPage);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));
        QFont font;
        font.setPointSize(32);
        font.setBold(true);
        titleLabel->setFont(font);
        titleLabel->setAlignment(Qt::AlignCenter);

        verticalLayout_main->addWidget(titleLabel);

        verticalSpacer_1 = new QSpacerItem(20, 80, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_1);

        horizontalLayout_buttons = new QHBoxLayout();
        horizontalLayout_buttons->setObjectName(QString::fromUtf8("horizontalLayout_buttons"));
        horizontalSpacer_left = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_left);

        verticalLayout_buttons = new QVBoxLayout();
        verticalLayout_buttons->setObjectName(QString::fromUtf8("verticalLayout_buttons"));
        exerciseSelectButton = new QPushButton(MainMenuPage);
        exerciseSelectButton->setObjectName(QString::fromUtf8("exerciseSelectButton"));
        exerciseSelectButton->setMinimumSize(QSize(300, 80));
        QFont font1;
        font1.setPointSize(18);
        font1.setBold(true);
        exerciseSelectButton->setFont(font1);
        exerciseSelectButton->setStyleSheet(QString::fromUtf8("background-color: #4CAF50; color: white; border-radius: 10px;"));

        verticalLayout_buttons->addWidget(exerciseSelectButton);

        verticalSpacer_2 = new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_buttons->addItem(verticalSpacer_2);

        settingsButton = new QPushButton(MainMenuPage);
        settingsButton->setObjectName(QString::fromUtf8("settingsButton"));
        settingsButton->setMinimumSize(QSize(300, 80));
        settingsButton->setFont(font1);
        settingsButton->setStyleSheet(QString::fromUtf8("background-color: #2196F3; color: white; border-radius: 10px;"));

        verticalLayout_buttons->addWidget(settingsButton);


        horizontalLayout_buttons->addLayout(verticalLayout_buttons);

        horizontalSpacer_right = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_right);


        verticalLayout_main->addLayout(horizontalLayout_buttons);

        verticalSpacer_bottom = new QSpacerItem(20, 100, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_bottom);

        statusLabel = new QLabel(MainMenuPage);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));
        statusLabel->setAlignment(Qt::AlignCenter);

        verticalLayout_main->addWidget(statusLabel);


        retranslateUi(MainMenuPage);

        QMetaObject::connectSlotsByName(MainMenuPage);
    } // setupUi

    void retranslateUi(QWidget *MainMenuPage)
    {
        MainMenuPage->setWindowTitle(QCoreApplication::translate("MainMenuPage", "Home Workout System", nullptr));
        titleLabel->setText(QCoreApplication::translate("MainMenuPage", "New Workout Plan", nullptr));
        exerciseSelectButton->setText(QCoreApplication::translate("MainMenuPage", "\354\232\264\353\217\231 \354\204\240\355\203\235", nullptr));
        settingsButton->setText(QCoreApplication::translate("MainMenuPage", "\354\204\244\354\240\225", nullptr));
        statusLabel->setText(QCoreApplication::translate("MainMenuPage", "\354\203\201\355\203\234: \354\244\200\353\271\204", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainMenuPage: public Ui_MainMenuPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAIN_H
