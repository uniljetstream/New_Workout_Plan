QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 타겟 이름
TARGET = NWP_jetson_ui
TEMPLATE = app

# OpenCV 설정 (Jetson Nano용)
INCLUDEPATH += /usr/include/opencv4
LIBS += -L/usr/lib/aarch64-linux-gnu \
        -lopencv_core \
        -lopencv_imgproc \
        -lopencv_highgui \
        -lopencv_videoio \
        -lopencv_imgcodecs

# 소스 파일
SOURCES += \
    main.cpp \
    widget.cpp

# 헤더 파일
HEADERS += \
    widget.h

# UI 파일
FORMS += \
    widget.ui

# 추가 파일들 (Qt Creator에서 보이도록)
OTHER_FILES += \
    README.md \
    QUICK_START.md \
    PROJECT_STRUCTURE.md \
    build.sh \
    install_deps.sh

# 기본 규칙
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 경고 무시
QMAKE_CXXFLAGS += -Wno-deprecated-declarations
