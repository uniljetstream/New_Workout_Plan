cmake_minimum_required(VERSION 3.16)

project(qt_test_app VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt5 packages
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Mqtt)

# Source files
set(SOURCES
    main.cpp
    mainwindow.cpp
    config.cpp
    cursorcanvas.cpp
)

# Header files
set(HEADERS
    mainwindow.h
    config.h
    cursorcanvas.h
)

# UI files
set(UI_FILES
    mainwindow.ui
)

# Create executable
add_executable(qt_test_app
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

# Link Qt5 libraries
target_link_libraries(qt_test_app
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::Mqtt
)

# Installation
install(TARGETS qt_test_app
    RUNTIME DESTINATION bin
)

# Copy config.json to build directory
configure_file(config.json ${CMAKE_CURRENT_BINARY_DIR}/config.json COPYONLY)
