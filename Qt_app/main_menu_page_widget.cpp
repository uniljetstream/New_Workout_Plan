#include "main_menu_page_widget.h"

#include "ui_main.h"

#include <QPushButton>

MainMenuPageWidget::MainMenuPageWidget(QWidget *parent) : QWidget(parent), m_ui(new Ui::MainMenuPage)
{
    m_ui->setupUi(this);

    connect(m_ui->exerciseSelectButton, &QPushButton::clicked, this, &MainMenuPageWidget::exerciseSelectRequested);
    connect(m_ui->settingsButton, &QPushButton::clicked, this, &MainMenuPageWidget::settingsRequested);
}

MainMenuPageWidget::~MainMenuPageWidget()
{
    delete m_ui;
}

void MainMenuPageWidget::setStatusText(const QString &text)
{
    m_ui->statusLabel->setText(text);
}
