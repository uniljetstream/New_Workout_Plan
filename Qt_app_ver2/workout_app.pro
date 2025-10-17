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
    config.cpp \
    airmouse_manager.cpp \
    cursor_overlay.cpp \
    videoframewidget.cpp \
    main_menu_page_widget.cpp \
    exercise_selection_page_widget.cpp \
    settings_page_widget.cpp \
    workout_page_widget.cpp \
    result_page_widget.cpp \
    exercise_catalog.cpp

HEADERS += \
    mainwindow.h \
    cursorcanvas.h \
    config.h \
    airmouse_manager.h \
    cursor_overlay.h \
    videoframewidget.h \
    main_menu_page_widget.h \
    exercise_selection_page_widget.h \
    settings_page_widget.h \
    workout_page_widget.h \
    result_page_widget.h \
    exercise_catalog.h

FORMS += \
    main.ui \
    exercise_selection.ui \
    settings.ui \
    workout.ui \
    result.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# MQTT library
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += Qt5Mqtt
}
