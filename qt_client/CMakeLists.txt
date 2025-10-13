cmake_minimum_required(VERSION 3.16)
project(WatchTowerQtClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Mqtt)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Mqtt)

add_executable(watchtower_qt_client
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(watchtower_qt_client
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Mqtt
)

target_include_directories(watchtower_qt_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
