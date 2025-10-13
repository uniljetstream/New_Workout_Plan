#include "widget.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << "==========================================================";
    qDebug() << "  New Workout Plan";
    qDebug() << "  Qt 버전:" << QT_VERSION_STR;
    qDebug() << "==========================================================";

    Widget w;
    w.show();

    return a.exec();
}
