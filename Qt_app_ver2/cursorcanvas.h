#ifndef CURSORCANVAS_H
#define CURSORCANVAS_H

#include <QWidget>
#include <QPoint>
#include <QList>

class CursorCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit CursorCanvas(QWidget *parent = nullptr);

    // Cursor control
    void moveCursor(int deltaX, int deltaY);
    void resetCursor();
    void setCursorVisible(bool visible);

    // Settings
    void setSensitivity(double sensitivity);
    void setSmoothing(bool enable);
    void setShowTrail(bool show);

    QPoint cursorPosition() const { return m_cursorPos; }

signals:
    void cursorMoved(int x, int y);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QPoint m_cursorPos;
    bool m_cursorVisible;
    double m_sensitivity;
    bool m_smoothing;
    bool m_showTrail;

    // Trail
    QList<QPoint> m_trail;
    int m_maxTrailLength;

    // Smoothing buffer
    QList<QPoint> m_moveBuffer;
    int m_bufferSize;

    void addToTrail(const QPoint &pos);
};

#endif // CURSORCANVAS_H
