#include "airmouse_manager.h"
#include "cursor_overlay.h"
#include <QWidget>
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QAbstractButton>
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
    if (!parent) {
        qWarning() << "AirMouseManager: parent widget is null!";
        return;
    }

    // 커서 오버레이 생성 (독립 윈도우)
    m_cursorOverlay = new CursorOverlay(parent);

    // 부모 위젯과 동일한 위치/크기로 설정
    m_cursorOverlay->setGeometry(parent->geometry());
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
    if (!m_parentWidget || !m_cursorOverlay) {
        qWarning() << "AirMouseManager: Cannot enable, widgets not initialized";
        return;
    }

    m_enabled = enabled;

    if (m_enabled) {
        // 에어마우스 활성화

        // 커서를 중앙으로 초기화
        m_cursorPos = QPoint(m_parentWidget->width() / 2, m_parentWidget->height() / 2);
        m_cursorOverlay->setCursorPosition(m_cursorPos);

        // 오버레이 geometry 업데이트 (MainWindow와 동일하게)
        m_cursorOverlay->setGeometry(m_parentWidget->geometry());

        // 오버레이 표시
        m_cursorOverlay->show();
        m_cursorOverlay->raise(); // 최상위로

        // 약간의 딜레이 후 타이머 시작 (오버레이가 완전히 표시된 후)
        QTimer::singleShot(100, this, [this]() {
            if (m_enabled && m_hoverTimer) {
                m_hoverTimer->start(50); // 20 Hz
                qDebug() << "AirMouse hover timer started";
            }
        });

        qDebug() << "AirMouse enabled at position" << m_cursorPos;
    } else {
        // 에어마우스 비활성화
        if (m_hoverTimer) {
            m_hoverTimer->stop();
        }
        m_hoveredWidget = nullptr;
        m_cursorOverlay->hide();

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
    if (!m_parentWidget) {
        return;
    }

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
    if (!m_enabled || !m_parentWidget) {
        return;
    }

    QWidget *targetWidget = getWidgetAtCursor();

    if (targetWidget) {
        // 버튼 등 실제 클릭 가능한 위젯으로 승격
        QWidget *clickTarget = targetWidget;
        for (QWidget *searchWidget = targetWidget; searchWidget; searchWidget = searchWidget->parentWidget()) {
            if (!searchWidget->isEnabled() || !searchWidget->isVisible()) {
                continue;
            }
            if (qobject_cast<QAbstractButton *>(searchWidget)) {
                clickTarget = searchWidget;
                break;
            }
        }

        QPoint globalPos = m_cursorOverlay
            ? m_cursorOverlay->mapToGlobal(m_cursorPos)
            : m_parentWidget->mapToGlobal(m_cursorPos);

        if (!clickTarget->isEnabled() || !clickTarget->isVisible()) {
            qDebug() << "Click skipped - target not ready" << clickTarget;
            return;
        }

        if (auto *button = qobject_cast<QAbstractButton *>(clickTarget)) {
            button->setFocus(Qt::MouseFocusReason);
            button->animateClick(1); // 즉시 클릭 애니메이션 및 시그널 발생
        } else {
            QPoint localPos = clickTarget->mapFromGlobal(globalPos);

            // 마우스 프레스 이벤트
            QMouseEvent pressEvent(
                QEvent::MouseButtonPress,
                localPos,
                globalPos,
                Qt::LeftButton,
                Qt::LeftButton,
                Qt::NoModifier
            );
            QApplication::sendEvent(clickTarget, &pressEvent);

            // 마우스 릴리스 이벤트
            QMouseEvent releaseEvent(
                QEvent::MouseButtonRelease,
                localPos,
                globalPos,
                Qt::LeftButton,
                Qt::NoButton,
                Qt::NoModifier
            );
            QApplication::sendEvent(clickTarget, &releaseEvent);
        }

        // 클릭 애니메이션 표시
        if (m_cursorOverlay) {
            m_cursorOverlay->showClickAnimation();
        }

        emit clicked(clickTarget);
        qDebug() << "Click simulated on" << clickTarget->metaObject()->className()
                << clickTarget->objectName();
    }
    else {
        qDebug() << "Click skipped - no target widget at cursor";
    }
}

void AirMouseManager::simulateRightClick()
{
    if (!m_enabled || !m_parentWidget) {
        return;
    }

    QWidget *targetWidget = getWidgetAtCursor();

    if (targetWidget) {
        const QPoint globalPos = m_cursorOverlay
            ? m_cursorOverlay->mapToGlobal(m_cursorPos)
            : m_parentWidget->mapToGlobal(m_cursorPos);
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
    if (!m_parentWidget) {
        return nullptr;
    }

    // 전역 좌표 계산 (오버레이가 있으면 오버레이 기준)
    const QPoint globalPos = m_cursorOverlay
        ? m_cursorOverlay->mapToGlobal(m_cursorPos)
        : m_parentWidget->mapToGlobal(m_cursorPos);

    // 부모 위젯 좌표계로 변환한 뒤 자식 위젯 검색
    const QPoint parentLocalPos = m_parentWidget->mapFromGlobal(globalPos);
    QWidget *widget = m_parentWidget->childAt(parentLocalPos);

    // 찾지 못했으면 전역 좌표로 시도
    if (!widget) {
        widget = QApplication::widgetAt(globalPos);

        // 오버레이 자신은 무시
        if (widget == m_cursorOverlay) {
            widget = nullptr;
        }
    }

    // 컨테이너 위젯인 경우 실제 하위 위젯 탐색
    if (widget) {
        QWidget *current = widget;
        while (current) {
            const QPoint localPos = current->mapFromGlobal(globalPos);
            QWidget *candidate = current->childAt(localPos);
            if (!candidate || candidate == current) {
                break;
            }
            current = candidate;
        }
        widget = current;

        // 여전히 오버레이면 무시
        if (widget == m_cursorOverlay) {
            widget = nullptr;
        }
    }

    return widget;
}

void AirMouseManager::updateHoverState()
{
    if (!m_enabled || !m_parentWidget) {
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
        if (currentWidget && m_parentWidget) {
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
    } else if (currentWidget && m_parentWidget) {
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
