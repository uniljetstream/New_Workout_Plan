#ifndef EXERCISE_CATALOG_H
#define EXERCISE_CATALOG_H

#include <QString>
#include <QVector>

struct ExerciseOption {
    QString displayName;
    QString mode;
};

/**
 * @brief AI 서버가 지원하는 운동 목록을 반환한다.
 *
 * displayName은 Qt UI에 표시되는 한글 이름,
 * mode는 WatchTower/AI 서버와 통신할 때 사용하는 내부 모드 문자열이다.
 */
const QVector<ExerciseOption> &exerciseCatalog();

#endif // EXERCISE_CATALOG_H
