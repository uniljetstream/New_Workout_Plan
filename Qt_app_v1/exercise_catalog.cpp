#include "exercise_catalog.h"

const QVector<ExerciseOption> &exerciseCatalog()
{
    static const QVector<ExerciseOption> catalog = {
        {QStringLiteral("맨몸 루틴"), QStringLiteral("bodyweight_routine")},
        {QStringLiteral("케틀벨 루틴"), QStringLiteral("kettlebell_routine")},
        {QStringLiteral("바벨 루틴"), QStringLiteral("barbell_routine")},
        {QStringLiteral("스쿼트"), QStringLiteral("squat")},
        {QStringLiteral("푸시업"), QStringLiteral("pushup")},
        {QStringLiteral("플랭크"), QStringLiteral("plank")},
        {QStringLiteral("런지"), QStringLiteral("lunge")},
        {QStringLiteral("케틀벨 스윙"), QStringLiteral("kettlebell_swing")},
        {QStringLiteral("케틀벨 데드리프트"), QStringLiteral("kettlebell_deadlift")},
        {QStringLiteral("사이드 런지"), QStringLiteral("side_lunge")},
        {QStringLiteral("브릿지"), QStringLiteral("bridge")},
        {QStringLiteral("니 드라이브"), QStringLiteral("knee_drive")},
        {QStringLiteral("바벨 로우"), QStringLiteral("barbell_row")},
        {QStringLiteral("바벨 업라이트 로우"), QStringLiteral("barbell_upright_row")},
        {QStringLiteral("바벨 오버헤드 프레스"), QStringLiteral("barbell_overhead_press")},
        {QStringLiteral("바벨 바이셉스 컬"), QStringLiteral("barbell_biceps_curl")},
        {QStringLiteral("바벨 리버스 컬"), QStringLiteral("barbell_reverse_curl")},
    };
    return catalog;
}
