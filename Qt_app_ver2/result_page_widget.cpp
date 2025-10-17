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
    
    // 이미지 파일 경로 설정
    if (rank == "PERFECT") {
        imagePath = "images/perfect.png";
    } else if (rank == "GREAT") {
        imagePath = "images/great.png";
    } else if (rank == "GOOD") {
        imagePath = "images/good.png";
    } else if (rank == "BAD") {
        imagePath = "images/bad.png";
    }
    
    // 이미지 로드 시도
    if (QFile::exists(imagePath)) {
        pixmap.load(imagePath);
        qDebug() << "✓ Loaded image:" << imagePath;
    }
    
    // 이미지 로드 실패 시 기본 그래픽 생성
    if (pixmap.isNull()) {
        qDebug() << "✗ Image not found, creating default graphic for:" << rank;
        pixmap = createDefaultRankImage(rank);
    }
    
    // 200x200 크기로 스케일링하여 표시
    m_ui->rankImageLabel->setPixmap(
        pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QPixmap ResultPageWidget::createDefaultRankImage(const QString &rank)
{
    QPixmap pixmap(200, 200);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect rect(20, 20, 160, 160);
    
    if (rank == "PERFECT") {
        // 황금색 원 배경
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#FFD700"));
        painter.drawEllipse(rect);
        
        // 별 그리기
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        QPolygonF star;
        for (int i = 0; i < 10; ++i) {
            double angle = M_PI * i / 5.0 - M_PI / 2.0;
            double r = (i % 2 == 0) ? 50 : 25;
            star << QPointF(100 + r * std::cos(angle), 100 + r * std::sin(angle));
        }
        painter.drawPolygon(star);
        
    } else if (rank == "GREAT") {
        // 초록색 원 배경
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#4CAF50"));
        painter.drawEllipse(rect);
        
        // 체크마크
        painter.setPen(QPen(Qt::white, 12, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(60, 100, 90, 130);
        painter.drawLine(90, 130, 140, 70);
        
    } else if (rank == "GOOD") {
        // 주황색 원 배경
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#FF9800"));
        painter.drawEllipse(rect);
        
        // 중립 표정
        painter.setPen(QPen(Qt::white, 10, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(70, 110, 130, 110);  // 입
        painter.setBrush(Qt::white);
        painter.drawEllipse(75, 70, 15, 15);  // 왼쪽 눈
        painter.drawEllipse(110, 70, 15, 15); // 오른쪽 눈
        
    } else if (rank == "BAD") {
        // 빨간색 원 배경
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#F44336"));
        painter.drawEllipse(rect);
        
        // X 표시
        painter.setPen(QPen(Qt::white, 12, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(60, 60, 140, 140);
        painter.drawLine(140, 60, 60, 140);
    }

    return pixmap;
}

QString ResultPageWidget::getRankFromScore(int totalScore, int exerciseCount) const
{
    if (exerciseCount == 0) {
        return "BAD";
    }

    double avgScore = static_cast<double>(totalScore) / exerciseCount;

    if (avgScore >= 90) {
        return "PERFECT";
    } else if (avgScore >= 75) {
        return "GREAT";
    } else if (avgScore >= 60) {
        return "GOOD";
    } else {
        return "BAD";
    }
}