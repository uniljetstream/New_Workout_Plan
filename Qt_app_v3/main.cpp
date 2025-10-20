#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application info
    QApplication::setApplicationName("Home Workout System");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("WatchTower");

    MainWindow window;
    window.show();

    return app.exec();
}
