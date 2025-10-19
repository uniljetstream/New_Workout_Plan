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
    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer_top;
    QHBoxLayout *horizontalLayout_main;
    QSpacerItem *horizontalSpacer_left;
    QVBoxLayout *verticalLayout_content;
    QLabel *titleLabel;
    QLabel *rankImageLabel;
    QLabel *rankLabel;
    QLabel *totalScoreLabel;
    QLabel *durationLabel;
    QLabel *exerciseCountLabel;
    QSpacerItem *verticalSpacer_hr;
    QLabel *heartRateTitleLabel;
    QLabel *maxHeartRateLabel;
    QLabel *minHeartRateLabel;
    QLabel *avgHeartRateLabel;
    QSpacerItem *horizontalSpacer_right;
    QSpacerItem *verticalSpacer_bottom;
    QHBoxLayout *horizontalLayout_buttons;
    QSpacerItem *horizontalSpacer_button_left;
    QPushButton *retryButton;
    QPushButton *backButton;
    QSpacerItem *horizontalSpacer_button_right;

    void setupUi(QWidget *ResultPage)
    {
        if (ResultPage->objectName().isEmpty())
            ResultPage->setObjectName(QString::fromUtf8("ResultPage"));
        ResultPage->resize(800, 600);
        verticalLayout = new QVBoxLayout(ResultPage);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalSpacer_top = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_top);

        horizontalLayout_main = new QHBoxLayout();
        horizontalLayout_main->setObjectName(QString::fromUtf8("horizontalLayout_main"));
        horizontalSpacer_left = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_main->addItem(horizontalSpacer_left);

        verticalLayout_content = new QVBoxLayout();
        verticalLayout_content->setObjectName(QString::fromUtf8("verticalLayout_content"));
        titleLabel = new QLabel(ResultPage);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont font;
        font.setPointSize(28);
        font.setBold(true);
        font.setWeight(75);
        titleLabel->setFont(font);

        verticalLayout_content->addWidget(titleLabel);

        rankImageLabel = new QLabel(ResultPage);
        rankImageLabel->setObjectName(QString::fromUtf8("rankImageLabel"));
        rankImageLabel->setAlignment(Qt::AlignCenter);
        rankImageLabel->setMinimumSize(QSize(300, 300));

        verticalLayout_content->addWidget(rankImageLabel);

        rankLabel = new QLabel(ResultPage);
        rankLabel->setObjectName(QString::fromUtf8("rankLabel"));
        rankLabel->setAlignment(Qt::AlignCenter);
        QFont font1;
        font1.setPointSize(36);
        font1.setBold(true);
        font1.setWeight(75);
        rankLabel->setFont(font1);

        verticalLayout_content->addWidget(rankLabel);

        totalScoreLabel = new QLabel(ResultPage);
        totalScoreLabel->setObjectName(QString::fromUtf8("totalScoreLabel"));
        totalScoreLabel->setAlignment(Qt::AlignCenter);
        QFont font2;
        font2.setPointSize(20);
        font2.setBold(true);
        font2.setWeight(75);
        totalScoreLabel->setFont(font2);

        verticalLayout_content->addWidget(totalScoreLabel);

        durationLabel = new QLabel(ResultPage);
        durationLabel->setObjectName(QString::fromUtf8("durationLabel"));
        durationLabel->setAlignment(Qt::AlignCenter);
        QFont font3;
        font3.setPointSize(16);
        durationLabel->setFont(font3);

        verticalLayout_content->addWidget(durationLabel);

        exerciseCountLabel = new QLabel(ResultPage);
        exerciseCountLabel->setObjectName(QString::fromUtf8("exerciseCountLabel"));
        exerciseCountLabel->setAlignment(Qt::AlignCenter);
        exerciseCountLabel->setFont(font3);

        verticalLayout_content->addWidget(exerciseCountLabel);

        verticalSpacer_hr = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_content->addItem(verticalSpacer_hr);

        heartRateTitleLabel = new QLabel(ResultPage);
        heartRateTitleLabel->setObjectName(QString::fromUtf8("heartRateTitleLabel"));
        heartRateTitleLabel->setAlignment(Qt::AlignCenter);
        QFont font4;
        font4.setPointSize(14);
        font4.setBold(true);
        font4.setWeight(75);
        heartRateTitleLabel->setFont(font4);

        verticalLayout_content->addWidget(heartRateTitleLabel);

        maxHeartRateLabel = new QLabel(ResultPage);
        maxHeartRateLabel->setObjectName(QString::fromUtf8("maxHeartRateLabel"));
        maxHeartRateLabel->setAlignment(Qt::AlignCenter);
        QFont font5;
        font5.setPointSize(14);
        maxHeartRateLabel->setFont(font5);

        verticalLayout_content->addWidget(maxHeartRateLabel);

        minHeartRateLabel = new QLabel(ResultPage);
        minHeartRateLabel->setObjectName(QString::fromUtf8("minHeartRateLabel"));
        minHeartRateLabel->setAlignment(Qt::AlignCenter);
        minHeartRateLabel->setFont(font5);

        verticalLayout_content->addWidget(minHeartRateLabel);

        avgHeartRateLabel = new QLabel(ResultPage);
        avgHeartRateLabel->setObjectName(QString::fromUtf8("avgHeartRateLabel"));
        avgHeartRateLabel->setAlignment(Qt::AlignCenter);
        avgHeartRateLabel->setFont(font4);

        verticalLayout_content->addWidget(avgHeartRateLabel);


        horizontalLayout_main->addLayout(verticalLayout_content);

        horizontalSpacer_right = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_main->addItem(horizontalSpacer_right);


        verticalLayout->addLayout(horizontalLayout_main);

        verticalSpacer_bottom = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_bottom);

        horizontalLayout_buttons = new QHBoxLayout();
        horizontalLayout_buttons->setObjectName(QString::fromUtf8("horizontalLayout_buttons"));
        horizontalSpacer_button_left = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_button_left);

        retryButton = new QPushButton(ResultPage);
        retryButton->setObjectName(QString::fromUtf8("retryButton"));
        retryButton->setMinimumSize(QSize(150, 50));
        retryButton->setFont(font5);

        horizontalLayout_buttons->addWidget(retryButton);

        backButton = new QPushButton(ResultPage);
        backButton->setObjectName(QString::fromUtf8("backButton"));
        backButton->setMinimumSize(QSize(150, 50));
        backButton->setFont(font5);

        horizontalLayout_buttons->addWidget(backButton);

        horizontalSpacer_button_right = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_button_right);


        verticalLayout->addLayout(horizontalLayout_buttons);


        retranslateUi(ResultPage);

        QMetaObject::connectSlotsByName(ResultPage);
    } // setupUi

    void retranslateUi(QWidget *ResultPage)
    {
        ResultPage->setWindowTitle(QCoreApplication::translate("ResultPage", "\354\232\264\353\217\231 \352\262\260\352\263\274", nullptr));
        titleLabel->setText(QCoreApplication::translate("ResultPage", "\354\232\264\353\217\231 \354\231\204\353\243\214!", nullptr));
        rankImageLabel->setText(QString());
        rankLabel->setText(QCoreApplication::translate("ResultPage", "RANK", nullptr));
        totalScoreLabel->setText(QCoreApplication::translate("ResultPage", "\354\264\235 \354\240\220\354\210\230: 0", nullptr));
        durationLabel->setText(QCoreApplication::translate("ResultPage", "\354\206\214\354\232\224 \354\213\234\352\260\204: 00:00", nullptr));
        exerciseCountLabel->setText(QCoreApplication::translate("ResultPage", "\354\231\204\353\243\214\355\225\234 \354\232\264\353\217\231: 0\352\260\234", nullptr));
        heartRateTitleLabel->setText(QCoreApplication::translate("ResultPage", "\342\224\201\342\224\201\342\224\201 \354\213\254\353\260\225\354\210\230 \355\206\265\352\263\204 \342\224\201\342\224\201\342\224\201", nullptr));
        heartRateTitleLabel->setStyleSheet(QCoreApplication::translate("ResultPage", "color: #2196F3;", nullptr));
        maxHeartRateLabel->setText(QCoreApplication::translate("ResultPage", "\354\265\234\353\214\200 \354\213\254\353\260\225\354\210\230: -- BPM", nullptr));
        minHeartRateLabel->setText(QCoreApplication::translate("ResultPage", "\354\265\234\354\206\214 \354\213\254\353\260\225\354\210\230: -- BPM", nullptr));
        avgHeartRateLabel->setText(QCoreApplication::translate("ResultPage", "\355\217\211\352\267\240 \354\213\254\353\260\225\354\210\230: -- BPM", nullptr));
        retryButton->setText(QCoreApplication::translate("ResultPage", "\353\213\244\354\213\234 \355\225\230\352\270\260", nullptr));
        backButton->setText(QCoreApplication::translate("ResultPage", "\353\251\224\354\235\270 \353\251\224\353\211\264", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ResultPage: public Ui_ResultPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RESULT_H
