#include "cursorcanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>

CursorCanvas::CursorCanvas(QWidget *parent)
    : QWidget(parent)
    , m_cursorVisible(true)
    , m_sensitivity(1.0)
    , m_smoothing(true)
    , m_showTrail(true)
    , m_maxTrailLength(50)
    , m_bufferSize(3)
{
    setMinimumSize(800, 600);
    setMouseTracking(true);

    // Start cursor at center
    resetCursor();

    // Style
    setStyleSheet("background-color: #f0f0f0; border: 2px solid #333;");
}

void CursorCanvas::moveCursor(int deltaX, int deltaY)
{
    if (!m_cursorVisible) return;

    // Apply sensitivity
    int adjustedX = static_cast<int>(deltaX * m_sensitivity);
    int adjustedY = static_cast<int>(deltaY * m_sensitivity);

    // Apply smoothing
    if (m_smoothing) {
        m_moveBuffer.append(QPoint(adjustedX, adjustedY));
        if (m_moveBuffer.size() > m_bufferSize) {
            m_moveBuffer.removeFirst();
        }

        // Calculate average
        int avgX = 0, avgY = 0;
        for (const QPoint &p : m_moveBuffer) {
            avgX += p.x();
            avgY += p.y();
        }
        if (!m_moveBuffer.isEmpty()) {
            adjustedX = avgX / m_moveBuffer.size();
            adjustedY = avgY / m_moveBuffer.size();
        }
    }

    // Move cursor
    m_cursorPos += QPoint(adjustedX, adjustedY);

    // Keep within bounds
    if (m_cursorPos.x() < 0) m_cursorPos.setX(0);
    if (m_cursorPos.y() < 0) m_cursorPos.setY(0);
    if (m_cursorPos.x() >= width()) m_cursorPos.setX(width() - 1);
    if (m_cursorPos.y() >= height()) m_cursorPos.setY(height() - 1);

    // Add to trail
    if (m_showTrail) {
        addToTrail(m_cursorPos);
    }

    emit cursorMoved(m_cursorPos.x(), m_cursorPos.y());
    update();
}

void CursorCanvas::resetCursor()
{
    m_cursorPos = QPoint(width() / 2, height() / 2);
    m_trail.clear();
    m_moveBuffer.clear();
    update();
}

void CursorCanvas::setCursorVisible(bool visible)
{
    m_cursorVisible = visible;
    update();
}

void CursorCanvas::setSensitivity(double sensitivity)
{
    m_sensitivity = sensitivity;
}

void CursorCanvas::setSmoothing(bool enable)
{
    m_smoothing = enable;
    if (!enable) {
        m_moveBuffer.clear();
    }
}

void CursorCanvas::setShowTrail(bool show)
{
    m_showTrail = show;
    if (!show) {
        m_trail.clear();
    }
    update();
}

void CursorCanvas::addToTrail(const QPoint &pos)
{
    m_trail.append(pos);
    if (m_trail.size() > m_maxTrailLength) {
        m_trail.removeFirst();
    }
}

void CursorCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw grid
    painter.setPen(QPen(QColor(220, 220, 220), 1));
    for (int x = 0; x < width(); x += 50) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += 50) {
        painter.drawLine(0, y, width(), y);
    }

    // Draw center crosshair
    painter.setPen(QPen(QColor(200, 200, 200), 1, Qt::DashLine));
    painter.drawLine(width() / 2, 0, width() / 2, height());
    painter.drawLine(0, height() / 2, width(), height() / 2);

    // Draw trail
    if (m_showTrail && m_trail.size() > 1) {
        QPainterPath path;
        path.moveTo(m_trail.first());
        for (int i = 1; i < m_trail.size(); ++i) {
            path.lineTo(m_trail[i]);
        }

        // Gradient color for trail
        for (int i = 0; i < m_trail.size() - 1; ++i) {
            double alpha = static_cast<double>(i) / m_trail.size();
            QColor color(100, 150, 255, static_cast<int>(alpha * 150));
            painter.setPen(QPen(color, 2));
            painter.drawLine(m_trail[i], m_trail[i + 1]);
        }
    }

    // Draw cursor
    if (m_cursorVisible) {
        // Cursor shadow
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 50));
        painter.drawEllipse(m_cursorPos + QPoint(2, 2), 12, 12);

        // Cursor outer circle
        painter.setPen(QPen(Qt::black, 2));
        painter.setBrush(QColor(255, 100, 100, 200));
        painter.drawEllipse(m_cursorPos, 10, 10);

        // Cursor inner circle
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.drawEllipse(m_cursorPos, 3, 3);

        // Cursor crosshair
        painter.setPen(QPen(Qt::white, 2));
        painter.drawLine(m_cursorPos.x() - 15, m_cursorPos.y(),
                        m_cursorPos.x() - 5, m_cursorPos.y());
        painter.drawLine(m_cursorPos.x() + 5, m_cursorPos.y(),
                        m_cursorPos.x() + 15, m_cursorPos.y());
        painter.drawLine(m_cursorPos.x(), m_cursorPos.y() - 15,
                        m_cursorPos.x(), m_cursorPos.y() - 5);
        painter.drawLine(m_cursorPos.x(), m_cursorPos.y() + 5,
                        m_cursorPos.x(), m_cursorPos.y() + 15);
    }

    // Draw position text
    painter.setPen(Qt::black);
    painter.setFont(QFont("Monospace", 10));
    QString posText = QString("Cursor: (%1, %2)").arg(m_cursorPos.x()).arg(m_cursorPos.y());
    painter.drawText(10, 20, posText);
}

void CursorCanvas::mousePressEvent(QMouseEvent *event)
{
    // Allow clicking to move cursor
    m_cursorPos = event->pos();
    m_trail.clear();
    emit cursorMoved(m_cursorPos.x(), m_cursorPos.y());
    update();
}
