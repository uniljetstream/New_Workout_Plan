/********************************************************************************
** Form generated from reading UI file 'result.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RESULT_H
#define UI_RESULT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ResultPage
{
public:
    QVBoxLayout *verticalLayout_main;
    QSpacerItem *verticalSpacer_top;
    QLabel *titleLabel;
    QSpacerItem *verticalSpacer_1;
    QLabel *rankImageLabel;
    QSpacerItem *verticalSpacer_2;
    QLabel *rankLabel;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_stats;
    QSpacerItem *horizontalSpacer_left;
    QVBoxLayout *verticalLayout_stats;
    QLabel *totalScoreLabel;
    QLabel *durationLabel;
    QLabel *exerciseCountLabel;
    QSpacerItem *horizontalSpacer_right;
    QSpacerItem *verticalSpacer_bottom;
    QHBoxLayout *horizontalLayout_buttons;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *retryButton;
    QPushButton *backButton;
    QSpacerItem *horizontalSpacer_4;

    void setupUi(QWidget *ResultPage)
    {
        if (ResultPage->objectName().isEmpty())
            ResultPage->setObjectName(QString::fromUtf8("ResultPage"));
        ResultPage->resize(800, 600);
        verticalLayout_main = new QVBoxLayout(ResultPage);
        verticalLayout_main->setObjectName(QString::fromUtf8("verticalLayout_main"));
        verticalSpacer_top = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_top);

        titleLabel = new QLabel(ResultPage);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont font;
        font.setPointSize(32);
        font.setBold(true);
        font.setWeight(75);
        titleLabel->setFont(font);

        verticalLayout_main->addWidget(titleLabel);

        verticalSpacer_1 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_1);

        rankImageLabel = new QLabel(ResultPage);
        rankImageLabel->setObjectName(QString::fromUtf8("rankImageLabel"));
        rankImageLabel->setMinimumSize(QSize(200, 200));
        rankImageLabel->setMaximumSize(QSize(200, 200));
        rankImageLabel->setAlignment(Qt::AlignCenter);
        rankImageLabel->setStyleSheet(QString::fromUtf8("border: 2px solid #ddd; border-radius: 10px; background-color: #f5f5f5;"));

        verticalLayout_main->addWidget(rankImageLabel);

        verticalSpacer_2 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_2);

        rankLabel = new QLabel(ResultPage);
        rankLabel->setObjectName(QString::fromUtf8("rankLabel"));
        rankLabel->setAlignment(Qt::AlignCenter);
        QFont font1;
        font1.setPointSize(36);
        font1.setBold(true);
        font1.setWeight(75);
        rankLabel->setFont(font1);
        rankLabel->setStyleSheet(QString::fromUtf8("color: #FFD700;"));

        verticalLayout_main->addWidget(rankLabel);

        verticalSpacer_3 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_3);

        horizontalLayout_stats = new QHBoxLayout();
        horizontalLayout_stats->setObjectName(QString::fromUtf8("horizontalLayout_stats"));
        horizontalSpacer_left = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_stats->addItem(horizontalSpacer_left);

        verticalLayout_stats = new QVBoxLayout();
        verticalLayout_stats->setObjectName(QString::fromUtf8("verticalLayout_stats"));
        totalScoreLabel = new QLabel(ResultPage);
        totalScoreLabel->setObjectName(QString::fromUtf8("totalScoreLabel"));
        totalScoreLabel->setAlignment(Qt::AlignCenter);
        QFont font2;
        font2.setPointSize(20);
        font2.setBold(true);
        font2.setWeight(75);
        totalScoreLabel->setFont(font2);

        verticalLayout_stats->addWidget(totalScoreLabel);

        durationLabel = new QLabel(ResultPage);
        durationLabel->setObjectName(QString::fromUtf8("durationLabel"));
        durationLabel->setAlignment(Qt::AlignCenter);
        QFont font3;
        font3.setPointSize(16);
        durationLabel->setFont(font3);

        verticalLayout_stats->addWidget(durationLabel);

        exerciseCountLabel = new QLabel(ResultPage);
        exerciseCountLabel->setObjectName(QString::fromUtf8("exerciseCountLabel"));
        exerciseCountLabel->setAlignment(Qt::AlignCenter);
        exerciseCountLabel->setFont(font3);

        verticalLayout_stats->addWidget(exerciseCountLabel);


        horizontalLayout_stats->addLayout(verticalLayout_stats);

        horizontalSpacer_right = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_stats->addItem(horizontalSpacer_right);


        verticalLayout_main->addLayout(horizontalLayout_stats);

        verticalSpacer_bottom = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_bottom);

        horizontalLayout_buttons = new QHBoxLayout();
        horizontalLayout_buttons->setObjectName(QString::fromUtf8("horizontalLayout_buttons"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_3);

        retryButton = new QPushButton(ResultPage);
        retryButton->setObjectName(QString::fromUtf8("retryButton"));
        retryButton->setMinimumSize(QSize(200, 60));
        QFont font4;
        font4.setPointSize(14);
        retryButton->setFont(font4);
        retryButton->setStyleSheet(QString::fromUtf8("background-color: #4CAF50; color: white; border-radius: 10px;"));

        horizontalLayout_buttons->addWidget(retryButton);

        backButton = new QPushButton(ResultPage);
        backButton->setObjectName(QString::fromUtf8("backButton"));
        backButton->setMinimumSize(QSize(200, 60));
        backButton->setFont(font4);
        backButton->setStyleSheet(QString::fromUtf8("background-color: #808080; color: white; border-radius: 10px;"));

        horizontalLayout_buttons->addWidget(backButton);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_4);


        verticalLayout_main->addLayout(horizontalLayout_buttons);


        retranslateUi(ResultPage);

        QMetaObject::connectSlotsByName(ResultPage);
    } // setupUi

    void retranslateUi(QWidget *ResultPage)
    {
        ResultPage->setWindowTitle(QCoreApplication::translate("ResultPage", "\354\232\264\353\217\231 \352\262\260\352\263\274", nullptr));
        titleLabel->setText(QCoreApplication::translate("ResultPage", "\354\232\264\353\217\231 \354\231\204\353\243\214!", nullptr));
        rankImageLabel->setText(QString());
        rankLabel->setText(QCoreApplication::translate("ResultPage", "PERFECT", nullptr));
        totalScoreLabel->setText(QCoreApplication::translate("ResultPage", "\354\264\235 \354\240\220\354\210\230: 0", nullptr));
        durationLabel->setText(QCoreApplication::translate("ResultPage", "\354\206\214\354\232\224 \354\213\234\352\260\204: 00:00", nullptr));
        exerciseCountLabel->setText(QCoreApplication::translate("ResultPage", "\354\231\204\353\243\214\355\225\234 \354\232\264\353\217\231: 0\352\260\234", nullptr));
        retryButton->setText(QCoreApplication::translate("ResultPage", "\353\213\244\354\213\234 \355\225\230\352\270\260", nullptr));
        backButton->setText(QCoreApplication::translate("ResultPage", "\353\251\224\354\235\270\354\234\274\353\241\234", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ResultPage: public Ui_ResultPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RESULT_H
