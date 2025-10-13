QT       += core gui widgets mqtt

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    cursorcanvas.cpp \
    config.cpp

HEADERS += \
    mainwindow.h \
    cursorcanvas.h \
    config.h

FORMS += \
    main.ui \
    exercise_selection.ui \
    settings.ui \
    workout.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# MQTT library
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += Qt5Mqtt
}
