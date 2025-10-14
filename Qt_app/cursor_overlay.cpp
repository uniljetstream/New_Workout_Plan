#include "cursor_overlay.h"
#include <QPainter>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>

CursorOverlay::CursorOverlay(QWidget *parent)
    : QWidget(nullptr)  // 독립적인 윈도우로 생성 (부모 없음)
    , m_cursorPos(0, 0)
    , m_cursorVisible(true)
    , m_cursorColor(255, 100, 100)
    , m_cursorSize(20)
    , m_showTrail(true)
    , m_maxTrailLength(30)
    , m_clickAnimationActive(false)
    , m_clickAnimationRadius(0)
{
    Q_UNUSED(parent);  // parent는 geometry 설정에만 사용

    // 독립적인 투명 오버레이 윈도우 설정
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);  // 모든 마우스 이벤트 통과
    setAttribute(Qt::WA_ShowWithoutActivating);  // 포커스를 가져가지 않음

    // 클릭 애니메이션 타이머
    m_clickAnimationTimer = new QTimer(this);
    connect(m_clickAnimationTimer, &QTimer::timeout, this, &CursorOverlay::updateClickAnimation);

    qDebug() << "CursorOverlay created as independent window";
}

void CursorOverlay::setCursorPosition(const QPoint &pos)
{
    m_cursorPos = pos;

    if (m_showTrail) {
        addToTrail(pos);
    }

    update();
}

void CursorOverlay::setCursorVisible(bool visible)
{
    m_cursorVisible = visible;
    update();
}

void CursorOverlay::setCursorColor(const QColor &color)
{
    m_cursorColor = color;
    update();
}

void CursorOverlay::setCursorSize(int size)
{
    m_cursorSize = size;
    update();
}

void CursorOverlay::setShowTrail(bool show)
{
    m_showTrail = show;
    if (!show) {
        m_trail.clear();
    }
    update();
}

void CursorOverlay::setTrailLength(int length)
{
    m_maxTrailLength = length;
}

void CursorOverlay::showClickAnimation()
{
    m_clickAnimationActive = true;
    m_clickAnimationRadius = 0;
    m_clickAnimationTimer->start(16); // ~60 FPS
}

void CursorOverlay::addToTrail(const QPoint &pos)
{
    m_trail.append(pos);
    if (m_trail.size() > m_maxTrailLength) {
        m_trail.removeFirst();
    }
}

void CursorOverlay::updateClickAnimation()
{
    if (m_clickAnimationActive) {
        m_clickAnimationRadius += 5;
        if (m_clickAnimationRadius > 50) {
            m_clickAnimationActive = false;
            m_clickAnimationTimer->stop();
        }
        update();
    }
}

void CursorOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 트레일 그리기
    if (m_showTrail && m_trail.size() > 1) {
        for (int i = 0; i < m_trail.size() - 1; ++i) {
            double alpha = static_cast<double>(i) / m_trail.size();
            QColor color = m_cursorColor;
            color.setAlpha(static_cast<int>(alpha * 100));
            painter.setPen(QPen(color, 2));
            painter.drawLine(m_trail[i], m_trail[i + 1]);
        }
    }

    // 클릭 애니메이션
    if (m_clickAnimationActive) {
        int alpha = 255 - (m_clickAnimationRadius * 5);
        if (alpha < 0) alpha = 0;

        QColor animColor = m_cursorColor;
        animColor.setAlpha(alpha);

        painter.setPen(QPen(animColor, 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(m_cursorPos, m_clickAnimationRadius, m_clickAnimationRadius);
    }

    // 커서 그리기
    if (m_cursorVisible) {
        // 커서 그림자
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 80));
        painter.drawEllipse(m_cursorPos + QPoint(2, 2), m_cursorSize / 2, m_cursorSize / 2);

        // 커서 외부 원
        painter.setPen(QPen(Qt::white, 3));
        painter.setBrush(m_cursorColor);
        painter.drawEllipse(m_cursorPos, m_cursorSize / 2, m_cursorSize / 2);

        // 커서 내부 점
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.drawEllipse(m_cursorPos, 4, 4);

        // 커서 십자선
        painter.setPen(QPen(Qt::white, 2));
        int crossSize = m_cursorSize;
        painter.drawLine(m_cursorPos.x() - crossSize, m_cursorPos.y(),
                        m_cursorPos.x() - crossSize / 2, m_cursorPos.y());
        painter.drawLine(m_cursorPos.x() + crossSize / 2, m_cursorPos.y(),
                        m_cursorPos.x() + crossSize, m_cursorPos.y());
        painter.drawLine(m_cursorPos.x(), m_cursorPos.y() - crossSize,
                        m_cursorPos.x(), m_cursorPos.y() - crossSize / 2);
        painter.drawLine(m_cursorPos.x(), m_cursorPos.y() + crossSize / 2,
                        m_cursorPos.x(), m_cursorPos.y() + crossSize);
    }
}

bool CursorOverlay::event(QEvent *event)
{
    // Qt::WA_TransparentForMouseEvents가 활성화되어 있으므로
    // 모든 마우스 이벤트는 자동으로 하위 위젯으로 전달됨
    // 여기서는 특별한 처리 없이 기본 동작만 수행
    return QWidget::event(event);
}
