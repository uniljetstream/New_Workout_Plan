#ifndef AIRMOUSE_MANAGER_H
#define AIRMOUSE_MANAGER_H

#include <QObject>
#include <QPoint>
#include <QTimer>
#include <QJsonObject>

class QWidget;
class CursorOverlay;

/**
 * @brief 전역 에어마우스 관리 클래스
 *
 * MQTT로부터 받은 에어마우스 데이터를 실제 Qt 마우스 이벤트로 변환하여
 * Qt 앱의 모든 위젯을 제어할 수 있도록 합니다.
 */
class AirMouseManager : public QObject
{
    Q_OBJECT

public:
    explicit AirMouseManager(QWidget *parent = nullptr);
    ~AirMouseManager();

    // 에어마우스 활성화/비활성화
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // 에어마우스 데이터 처리
    void handleMouseData(const QJsonObject &data);

    // 설정
    void setSensitivity(double sensitivity);
    void setSmoothing(bool enable);
    void setShowCursor(bool show);

    // 클릭 시뮬레이션 (조이스틱 버튼용)
    void simulateClick();
    void simulateRightClick();

    // 현재 커서 위치
    QPoint cursorPosition() const { return m_cursorPos; }

signals:
    void cursorMoved(int x, int y);
    void clicked(QWidget *targetWidget);

private:
    // 커서 이동 처리
    void moveCursor(double deltaX, double deltaY);
    void updateCursorPosition(const QPoint &newPos);

    // 스크롤 처리
    void handleScroll(int delta);

    // 위젯 찾기
    QWidget* getWidgetAtCursor() const;

    // 호버 효과
    void updateHoverState();

    // 스무딩
    QPoint applySmoothingFilter(const QPoint &delta);

private:
    QWidget *m_parentWidget;           // 부모 위젯 (MainWindow)
    CursorOverlay *m_cursorOverlay;    // 전역 커서 오버레이

    bool m_enabled;                    // 에어마우스 활성화 상태
    QPoint m_cursorPos;                // 현재 커서 위치

    // 설정
    double m_sensitivity;              // 감도 (1.0 = 기본)
    bool m_smoothing;                  // 스무딩 활성화
    bool m_showCursor;                 // 커서 표시 여부

    // 스무딩 버퍼
    QList<QPoint> m_moveBuffer;
    int m_bufferSize;

    // 호버 상태
    QWidget *m_hoveredWidget;          // 현재 호버 중인 위젯
    QTimer *m_hoverTimer;              // 호버 업데이트 타이머
};

#endif // AIRMOUSE_MANAGER_H
