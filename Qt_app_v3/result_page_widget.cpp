#include "result_page_widget.h"
#include "ui_result.h"

#include <QPushButton>
#include <QPixmap>
#include <QPainter>
#include <QFile>
#include <QDebug>
#include <cmath>

ResultPageWidget::ResultPageWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ResultPage)
{
    m_ui->setupUi(this);

    connect(m_ui->retryButton, &QPushButton::clicked,
            this, &ResultPageWidget::retryRequested);
    connect(m_ui->backButton, &QPushButton::clicked,
            this, &ResultPageWidget::backRequested);
}

ResultPageWidget::~ResultPageWidget()
{
    delete m_ui;
}

void ResultPageWidget::setResults(int totalScore, int durationSeconds, int exerciseCount)
{
    m_ui->totalScoreLabel->setText(tr("총 점수: %1").arg(totalScore));

    int minutes = durationSeconds / 60;
    int seconds = durationSeconds % 60;
    m_ui->durationLabel->setText(
        tr("소요 시간: %1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0')));

    m_ui->exerciseCountLabel->setText(tr("완료한 운동: %1개").arg(exerciseCount));

    QString rank = getRankFromScore(totalScore, exerciseCount);
    updateRankDisplay(rank);
}

void ResultPageWidget::setHeartRateStats(int minBpm, int maxBpm, int avgBpm)
{
    if (minBpm < 0 || maxBpm < 0 || avgBpm < 0) {
        m_ui->minHeartRateLabel->setText(tr("최소 심박수: -- BPM"));
        m_ui->maxHeartRateLabel->setText(tr("최대 심박수: -- BPM"));
        m_ui->avgHeartRateLabel->setText(tr("평균 심박수: -- BPM"));
    } else {
        m_ui->minHeartRateLabel->setText(tr("최소 심박수: %1 BPM").arg(minBpm));
        m_ui->maxHeartRateLabel->setText(tr("최대 심박수: %1 BPM").arg(maxBpm));
        m_ui->avgHeartRateLabel->setText(tr("평균 심박수: %1 BPM").arg(avgBpm));
    }
}

void ResultPageWidget::updateRankDisplay(const QString &rank)
{
    m_ui->rankLabel->setText(rank);

    if (rank == "PERFECT") {
        m_ui->rankLabel->setStyleSheet(
            QStringLiteral("color: #FFD700; font-size: 36pt; font-weight: bold;"));
    } else if (rank == "GREAT") {
        m_ui->rankLabel->setStyleSheet(
            QStringLiteral("color: #4CAF50; font-size: 36pt; font-weight: bold;"));
    } else if (rank == "GOOD") {
        m_ui->rankLabel->setStyleSheet(
            QStringLiteral("color: #FF9800; font-size: 36pt; font-weight: bold;"));
    } else if (rank == "BAD") {
        m_ui->rankLabel->setStyleSheet(
            QStringLiteral("color: #F44336; font-size: 36pt; font-weight: bold;"));
    }

    setRankImage(rank);
}

void ResultPageWidget::setRankImage(const QString &rank)
{
    QPixmap pixmap;
    QString imagePath;
    
    if (rank == "PERFECT") {
        imagePath = "images/perfect.png";
    } else if (rank == "GREAT") {
        imagePath = "images/great.png";
    } else if (rank == "GOOD") {
        imagePath = "images/good.png";
    } else if (rank == "BAD") {
        imagePath = "images/bad.png";
    }
    
    if (QFile::exists(imagePath)) {
        pixmap.load(imagePath);
    } else {
        pixmap = createDefaultRankImage(rank);
    }
    
    if (!pixmap.isNull()) {
        m_ui->rankImageLabel->setPixmap(pixmap.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

QString ResultPageWidget::getRankFromScore(int totalScore, int exerciseCount) const
{
    if (exerciseCount == 0) {
        return "BAD";
    }
    
    int avgScore = totalScore / exerciseCount;
    
    if (avgScore >= 90) return "PERFECT";
    if (avgScore >= 70) return "GREAT";
    if (avgScore >= 50) return "GOOD";
    return "BAD";
}

QPixmap ResultPageWidget::createDefaultRankImage(const QString &rank)
{
    QPixmap pixmap(300, 300);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor color;
    if (rank == "PERFECT") color = QColor("#FFD700");
    else if (rank == "GREAT") color = QColor("#4CAF50");
    else if (rank == "GOOD") color = QColor("#FF9800");
    else color = QColor("#F44336");
    
    painter.setPen(QPen(color, 5));
    painter.setBrush(color);
    painter.drawEllipse(50, 50, 200, 200);
    
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(48);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, rank);
    
    return pixmap;
}