#include "airmouse_manager.h"
#include "cursor_overlay.h"
#include <QWidget>
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>
#include <QDebug>
#include <QtMath>

AirMouseManager::AirMouseManager(QWidget *parent)
    : QObject(parent)
    , m_parentWidget(parent)
    , m_cursorOverlay(nullptr)
    , m_enabled(false)
    , m_cursorPos(400, 300)
    , m_sensitivity(1.0)
    , m_smoothing(true)
    , m_showCursor(true)
    , m_bufferSize(3)
    , m_hoveredWidget(nullptr)
{
    // 커서 오버레이 생성
    m_cursorOverlay = new CursorOverlay(parent);
    m_cursorOverlay->setGeometry(parent->rect());
    m_cursorOverlay->setCursorPosition(m_cursorPos);
    m_cursorOverlay->hide();

    // 호버 업데이트 타이머
    m_hoverTimer = new QTimer(this);
    connect(m_hoverTimer, &QTimer::timeout, this, &AirMouseManager::updateHoverState);

    qDebug() << "AirMouseManager initialized";
}

AirMouseManager::~AirMouseManager()
{
    if (m_cursorOverlay) {
        m_cursorOverlay->deleteLater();
    }
}

void AirMouseManager::setEnabled(bool enabled)
{
    m_enabled = enabled;

    if (m_enabled) {
        // 에어마우스 활성화
        m_cursorOverlay->show();
        m_cursorOverlay->raise(); // 최상위로

        // 커서를 중앙으로 초기화
        m_cursorPos = QPoint(m_parentWidget->width() / 2, m_parentWidget->height() / 2);
        m_cursorOverlay->setCursorPosition(m_cursorPos);

        // 호버 타이머 시작
        m_hoverTimer->start(50); // 20 Hz

        qDebug() << "AirMouse enabled";
    } else {
        // 에어마우스 비활성화
        m_cursorOverlay->hide();
        m_hoverTimer->stop();
        m_hoveredWidget = nullptr;

        qDebug() << "AirMouse disabled";
    }
}

void AirMouseManager::handleMouseData(const QJsonObject &data)
{
    if (!m_enabled) {
        return;
    }

    // 마우스 이동 데이터 추출
    double mouseX = data["mouse_x"].toDouble();
    double mouseY = data["mouse_y"].toDouble();

    // 커서 이동
    moveCursor(mouseX, mouseY);

    // 스크롤 데이터 처리
    if (data.contains("scroll_delta")) {
        int scrollDelta = data["scroll_delta"].toInt();
        if (scrollDelta != 0) {
            handleScroll(scrollDelta);
        }
    }
}

void AirMouseManager::moveCursor(double deltaX, double deltaY)
{
    // 감도 적용
    int adjustedX = static_cast<int>(deltaX * m_sensitivity);
    int adjustedY = static_cast<int>(deltaY * m_sensitivity);

    QPoint delta(adjustedX, adjustedY);

    // 스무딩 필터 적용
    if (m_smoothing) {
        delta = applySmoothingFilter(delta);
    }

    // 새 커서 위치 계산
    QPoint newPos = m_cursorPos + delta;

    // 경계 체크
    if (newPos.x() < 0) newPos.setX(0);
    if (newPos.y() < 0) newPos.setY(0);
    if (newPos.x() >= m_parentWidget->width()) newPos.setX(m_parentWidget->width() - 1);
    if (newPos.y() >= m_parentWidget->height()) newPos.setY(m_parentWidget->height() - 1);

    // 커서 위치 업데이트
    updateCursorPosition(newPos);
}

void AirMouseManager::updateCursorPosition(const QPoint &newPos)
{
    m_cursorPos = newPos;

    // 오버레이 커서 업데이트
    if (m_cursorOverlay && m_showCursor) {
        m_cursorOverlay->setCursorPosition(m_cursorPos);
    }

    // 시그널 발생
    emit cursorMoved(m_cursorPos.x(), m_cursorPos.y());
}

QPoint AirMouseManager::applySmoothingFilter(const QPoint &delta)
{
    m_moveBuffer.append(delta);
    if (m_moveBuffer.size() > m_bufferSize) {
        m_moveBuffer.removeFirst();
    }

    // 평균 계산
    int avgX = 0, avgY = 0;
    for (const QPoint &p : m_moveBuffer) {
        avgX += p.x();
        avgY += p.y();
    }

    if (!m_moveBuffer.isEmpty()) {
        avgX /= m_moveBuffer.size();
        avgY /= m_moveBuffer.size();
    }

    return QPoint(avgX, avgY);
}

void AirMouseManager::handleScroll(int delta)
{
    QWidget *targetWidget = getWidgetAtCursor();

    if (targetWidget) {
        // 스크롤 이벤트 생성
        QPoint globalPos = m_parentWidget->mapToGlobal(m_cursorPos);
        QPoint localPos = targetWidget->mapFromGlobal(globalPos);

        QWheelEvent wheelEvent(
            localPos,
            globalPos,
            QPoint(0, 0),
            QPoint(0, delta * 120), // 120 units per degree
            Qt::NoButton,
            Qt::NoModifier,
            Qt::ScrollUpdate,
            false
        );

        QApplication::sendEvent(targetWidget, &wheelEvent);
        qDebug() << "Scroll event sent to" << targetWidget->objectName() << "delta:" << delta;
    }
}

void AirMouseManager::simulateClick()
{
    if (!m_enabled) {
        return;
    }

    QWidget *targetWidget = getWidgetAtCursor();

    if (targetWidget) {
        QPoint globalPos = m_parentWidget->mapToGlobal(m_cursorPos);
        QPoint localPos = targetWidget->mapFromGlobal(globalPos);

        // 마우스 프레스 이벤트
        QMouseEvent pressEvent(
            QEvent::MouseButtonPress,
            localPos,
            globalPos,
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier
        );
        QApplication::sendEvent(targetWidget, &pressEvent);

        // 마우스 릴리스 이벤트
        QMouseEvent releaseEvent(
            QEvent::MouseButtonRelease,
            localPos,
            globalPos,
            Qt::LeftButton,
            Qt::NoButton,
            Qt::NoModifier
        );
        QApplication::sendEvent(targetWidget, &releaseEvent);

        // 클릭 애니메이션 표시
        m_cursorOverlay->showClickAnimation();

        emit clicked(targetWidget);
        qDebug() << "Click simulated on" << targetWidget->objectName();
    }
}

void AirMouseManager::simulateRightClick()
{
    if (!m_enabled) {
        return;
    }

    QWidget *targetWidget = getWidgetAtCursor();

    if (targetWidget) {
        QPoint globalPos = m_parentWidget->mapToGlobal(m_cursorPos);
        QPoint localPos = targetWidget->mapFromGlobal(globalPos);

        // 우클릭 이벤트
        QMouseEvent pressEvent(
            QEvent::MouseButtonPress,
            localPos,
            globalPos,
            Qt::RightButton,
            Qt::RightButton,
            Qt::NoModifier
        );
        QApplication::sendEvent(targetWidget, &pressEvent);

        QMouseEvent releaseEvent(
            QEvent::MouseButtonRelease,
            localPos,
            globalPos,
            Qt::RightButton,
            Qt::NoButton,
            Qt::NoModifier
        );
        QApplication::sendEvent(targetWidget, &releaseEvent);

        qDebug() << "Right-click simulated on" << targetWidget->objectName();
    }
}

QWidget* AirMouseManager::getWidgetAtCursor() const
{
    QPoint globalPos = m_parentWidget->mapToGlobal(m_cursorPos);
    QWidget *widget = QApplication::widgetAt(globalPos);

    // 오버레이 자신은 무시
    if (widget == m_cursorOverlay) {
        widget = m_parentWidget->childAt(m_cursorPos);
    }

    return widget;
}

void AirMouseManager::updateHoverState()
{
    if (!m_enabled) {
        return;
    }

    QWidget *currentWidget = getWidgetAtCursor();

    if (currentWidget != m_hoveredWidget) {
        // 이전 위젯 호버 종료
        if (m_hoveredWidget) {
            QHoverEvent leaveEvent(
                QEvent::HoverLeave,
                QPointF(),
                QPointF(m_cursorPos),
                Qt::NoModifier
            );
            QApplication::sendEvent(m_hoveredWidget, &leaveEvent);
        }

        // 새 위젯 호버 시작
        if (currentWidget) {
            QPoint globalPos = m_parentWidget->mapToGlobal(m_cursorPos);
            QPoint localPos = currentWidget->mapFromGlobal(globalPos);

            QHoverEvent enterEvent(
                QEvent::HoverEnter,
                QPointF(localPos),
                QPointF(),
                Qt::NoModifier
            );
            QApplication::sendEvent(currentWidget, &enterEvent);
        }

        m_hoveredWidget = currentWidget;
    } else if (currentWidget) {
        // 호버 이동
        QPoint globalPos = m_parentWidget->mapToGlobal(m_cursorPos);
        QPoint localPos = currentWidget->mapFromGlobal(globalPos);

        QHoverEvent moveEvent(
            QEvent::HoverMove,
            QPointF(localPos),
            QPointF(localPos),
            Qt::NoModifier
        );
        QApplication::sendEvent(currentWidget, &moveEvent);
    }
}

void AirMouseManager::setSensitivity(double sensitivity)
{
    m_sensitivity = sensitivity;
    qDebug() << "AirMouse sensitivity set to" << sensitivity;
}

void AirMouseManager::setSmoothing(bool enable)
{
    m_smoothing = enable;
    if (!enable) {
        m_moveBuffer.clear();
    }
    qDebug() << "AirMouse smoothing" << (enable ? "enabled" : "disabled");
}

void AirMouseManager::setShowCursor(bool show)
{
    m_showCursor = show;
    if (m_cursorOverlay) {
        m_cursorOverlay->setCursorVisible(show);
    }
}
