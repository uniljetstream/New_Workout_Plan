#include "videoframewidget.h"

#include <QPixmap>
#include <QResizeEvent>

VideoFrameWidget::VideoFrameWidget(QWidget *parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setStyleSheet("background-color: #000000;");
    setMinimumSize(320, 240);
    setText("영상 대기 중...");
    setWordWrap(true);
}

void VideoFrameWidget::setFrame(const QImage &image)
{
    if (image.isNull()) {
        clearFrame("영상 신호를 수신하지 못했습니다.");
        return;
    }

    m_currentFrame = image;
    setText(QString());
    updatePixmap();
}

void VideoFrameWidget::setFrame(const QByteArray &encodedImage)
{
    QImage image;
    image.loadFromData(encodedImage);
    setFrame(image);
}

void VideoFrameWidget::clearFrame(const QString &message)
{
    m_currentFrame = QImage();
    setPixmap(QPixmap());
    setText(message.isEmpty() ? tr("영상 신호 없음") : message);
}

void VideoFrameWidget::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    updatePixmap();
}

void VideoFrameWidget::updatePixmap()
{
    if (m_currentFrame.isNull()) {
        return;
    }

    const QPixmap pixmap = QPixmap::fromImage(m_currentFrame);
    setPixmap(pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
