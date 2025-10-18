#include "exercise_catalog.h"

const QVector<ExerciseOption> &exerciseCatalog()
{
    static const QVector<ExerciseOption> catalog = {
                                                    // [루틴 모드]
                                                    {QStringLiteral("맨몸 운동 루틴"),        QStringLiteral("bodyweight_routine")},
                                                    {QStringLiteral("케틀벨 운동 루틴"),      QStringLiteral("kettlebell_routine")},
                                                    {QStringLiteral("바벨 운동 루틴"),        QStringLiteral("barbell_routine")},

                                                    // [개별 맨몸 운동 - 2개]
                                                    {QStringLiteral("스쿼트"),                 QStringLiteral("squat")},
                                                    {QStringLiteral("런지"),                   QStringLiteral("lunge")},

                                                    // [개별 케틀벨 운동 - 2개]
                                                    {QStringLiteral("케틀벨 스윙"),            QStringLiteral("kettlebell_swing")},
                                                    {QStringLiteral("케틀벨 데드리프트"),      QStringLiteral("kettlebell_deadlift")},

                                                    // [개별 바벨 운동 - 5개]
                                                    {QStringLiteral("바벨 로우"),              QStringLiteral("barbell_row")},
                                                    {QStringLiteral("바벨 업라이트 로우"),     QStringLiteral("barbell_upright_row")},
                                                    {QStringLiteral("바벨 오버헤드 프레스"),   QStringLiteral("barbell_overhead_press")},
                                                    {QStringLiteral("바벨 바이셉스 컬"),       QStringLiteral("barbell_biceps_curl")},
                                                    {QStringLiteral("바벨 리버스 컬"),         QStringLiteral("barbell_reverse_curl")},
                                                    };
    return catalog;
}
