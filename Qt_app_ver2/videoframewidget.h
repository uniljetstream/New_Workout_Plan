#ifndef VIDEOFRAMEWIDGET_H
#define VIDEOFRAMEWIDGET_H

#include <QLabel>
#include <QImage>

/**
 * @brief Simple widget for displaying streamed video frames.
 *
 * Keeps the last received frame and scales it with aspect ratio preserved.
 */
class VideoFrameWidget : public QLabel
{
    Q_OBJECT

public:
    explicit VideoFrameWidget(QWidget *parent = nullptr);

    void setFrame(const QImage &image);
    void setFrame(const QByteArray &encodedImage);
    void clearFrame(const QString &message = QString());

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updatePixmap();

    QImage m_currentFrame;
};

#endif // VIDEOFRAMEWIDGET_H
