#ifndef CURSOR_OVERLAY_H
#define CURSOR_OVERLAY_H

#include <QWidget>
#include <QPoint>
#include <QList>

/**
 * @brief 전역 커서 오버레이 위젯
 *
 * 모든 페이지 위에 표시되는 투명 오버레이로, 에어마우스 커서를 렌더링합니다.
 * 마우스 이벤트를 하위 위젯으로 전달하여 실제 클릭이 가능하도록 합니다.
 */
class CursorOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit CursorOverlay(QWidget *parent = nullptr);

    // 커서 위치 설정
    void setCursorPosition(const QPoint &pos);
    QPoint cursorPosition() const { return m_cursorPos; }

    // 커서 표시/숨김
    void setCursorVisible(bool visible);
    bool isCursorVisible() const { return m_cursorVisible; }

    // 커서 스타일
    void setCursorColor(const QColor &color);
    void setCursorSize(int size);

    // 트레일 설정
    void setShowTrail(bool show);
    void setTrailLength(int length);

    // 클릭 애니메이션
    void showClickAnimation();

protected:
    void paintEvent(QPaintEvent *event) override;

    // 마우스 이벤트를 하위 위젯으로 전달
    bool event(QEvent *event) override;

private:
    void addToTrail(const QPoint &pos);
    void updateClickAnimation();

private:
    QPoint m_cursorPos;              // 커서 위치
    bool m_cursorVisible;            // 커서 표시 여부

    // 커서 스타일
    QColor m_cursorColor;            // 커서 색상
    int m_cursorSize;                // 커서 크기

    // 트레일
    bool m_showTrail;                // 트레일 표시
    QList<QPoint> m_trail;           // 트레일 점들
    int m_maxTrailLength;            // 최대 트레일 길이

    // 클릭 애니메이션
    bool m_clickAnimationActive;     // 클릭 애니메이션 활성화
    int m_clickAnimationRadius;      // 클릭 애니메이션 반경
    QTimer *m_clickAnimationTimer;   // 클릭 애니메이션 타이머
};

#endif // CURSOR_OVERLAY_H
