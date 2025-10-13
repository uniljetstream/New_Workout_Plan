/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[42];
    char stringdata0[969];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 31), // "on_exerciseSelectButton_clicked"
QT_MOC_LITERAL(2, 43, 0), // ""
QT_MOC_LITERAL(3, 44, 25), // "on_settingsButton_clicked"
QT_MOC_LITERAL(4, 70, 22), // "on_squatButton_clicked"
QT_MOC_LITERAL(5, 93, 23), // "on_pushupButton_clicked"
QT_MOC_LITERAL(6, 117, 22), // "on_plankButton_clicked"
QT_MOC_LITERAL(7, 140, 22), // "on_lungeButton_clicked"
QT_MOC_LITERAL(8, 163, 28), // "on_jumpingJackButton_clicked"
QT_MOC_LITERAL(9, 192, 32), // "on_mountainClimberButton_clicked"
QT_MOC_LITERAL(10, 225, 23), // "on_burpeeButton_clicked"
QT_MOC_LITERAL(11, 249, 23), // "on_customButton_clicked"
QT_MOC_LITERAL(12, 273, 25), // "on_scrollUpButton_clicked"
QT_MOC_LITERAL(13, 299, 27), // "on_scrollDownButton_clicked"
QT_MOC_LITERAL(14, 327, 39), // "on_exerciseSelection_backButt..."
QT_MOC_LITERAL(15, 367, 33), // "on_settings_connectButton_cli..."
QT_MOC_LITERAL(16, 401, 36), // "on_settings_disconnectButton_..."
QT_MOC_LITERAL(17, 438, 35), // "on_settings_calibrateButton_c..."
QT_MOC_LITERAL(18, 474, 38), // "on_settings_testAirMouseButto..."
QT_MOC_LITERAL(19, 513, 30), // "on_settings_saveButton_clicked"
QT_MOC_LITERAL(20, 544, 30), // "on_settings_backButton_clicked"
QT_MOC_LITERAL(21, 575, 33), // "on_sensitivitySlider_valueCha..."
QT_MOC_LITERAL(22, 609, 5), // "value"
QT_MOC_LITERAL(23, 615, 28), // "on_smoothingCheckBox_toggled"
QT_MOC_LITERAL(24, 644, 7), // "checked"
QT_MOC_LITERAL(25, 652, 24), // "on_trailCheckBox_toggled"
QT_MOC_LITERAL(26, 677, 30), // "on_workout_startButton_clicked"
QT_MOC_LITERAL(27, 708, 29), // "on_workout_stopButton_clicked"
QT_MOC_LITERAL(28, 738, 29), // "on_workout_backButton_clicked"
QT_MOC_LITERAL(29, 768, 15), // "onMqttConnected"
QT_MOC_LITERAL(30, 784, 18), // "onMqttDisconnected"
QT_MOC_LITERAL(31, 803, 21), // "onMqttMessageReceived"
QT_MOC_LITERAL(32, 825, 7), // "message"
QT_MOC_LITERAL(33, 833, 14), // "QMqttTopicName"
QT_MOC_LITERAL(34, 848, 5), // "topic"
QT_MOC_LITERAL(35, 854, 18), // "onMqttStateChanged"
QT_MOC_LITERAL(36, 873, 24), // "QMqttClient::ClientState"
QT_MOC_LITERAL(37, 898, 5), // "state"
QT_MOC_LITERAL(38, 904, 11), // "onMqttError"
QT_MOC_LITERAL(39, 916, 24), // "QMqttClient::ClientError"
QT_MOC_LITERAL(40, 941, 5), // "error"
QT_MOC_LITERAL(41, 947, 21) // "onWorkoutTimerTimeout"

    },
    "MainWindow\0on_exerciseSelectButton_clicked\0"
    "\0on_settingsButton_clicked\0"
    "on_squatButton_clicked\0on_pushupButton_clicked\0"
    "on_plankButton_clicked\0on_lungeButton_clicked\0"
    "on_jumpingJackButton_clicked\0"
    "on_mountainClimberButton_clicked\0"
    "on_burpeeButton_clicked\0on_customButton_clicked\0"
    "on_scrollUpButton_clicked\0"
    "on_scrollDownButton_clicked\0"
    "on_exerciseSelection_backButton_clicked\0"
    "on_settings_connectButton_clicked\0"
    "on_settings_disconnectButton_clicked\0"
    "on_settings_calibrateButton_clicked\0"
    "on_settings_testAirMouseButton_clicked\0"
    "on_settings_saveButton_clicked\0"
    "on_settings_backButton_clicked\0"
    "on_sensitivitySlider_valueChanged\0"
    "value\0on_smoothingCheckBox_toggled\0"
    "checked\0on_trailCheckBox_toggled\0"
    "on_workout_startButton_clicked\0"
    "on_workout_stopButton_clicked\0"
    "on_workout_backButton_clicked\0"
    "onMqttConnected\0onMqttDisconnected\0"
    "onMqttMessageReceived\0message\0"
    "QMqttTopicName\0topic\0onMqttStateChanged\0"
    "QMqttClient::ClientState\0state\0"
    "onMqttError\0QMqttClient::ClientError\0"
    "error\0onWorkoutTimerTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      31,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  169,    2, 0x08 /* Private */,
       3,    0,  170,    2, 0x08 /* Private */,
       4,    0,  171,    2, 0x08 /* Private */,
       5,    0,  172,    2, 0x08 /* Private */,
       6,    0,  173,    2, 0x08 /* Private */,
       7,    0,  174,    2, 0x08 /* Private */,
       8,    0,  175,    2, 0x08 /* Private */,
       9,    0,  176,    2, 0x08 /* Private */,
      10,    0,  177,    2, 0x08 /* Private */,
      11,    0,  178,    2, 0x08 /* Private */,
      12,    0,  179,    2, 0x08 /* Private */,
      13,    0,  180,    2, 0x08 /* Private */,
      14,    0,  181,    2, 0x08 /* Private */,
      15,    0,  182,    2, 0x08 /* Private */,
      16,    0,  183,    2, 0x08 /* Private */,
      17,    0,  184,    2, 0x08 /* Private */,
      18,    0,  185,    2, 0x08 /* Private */,
      19,    0,  186,    2, 0x08 /* Private */,
      20,    0,  187,    2, 0x08 /* Private */,
      21,    1,  188,    2, 0x08 /* Private */,
      23,    1,  191,    2, 0x08 /* Private */,
      25,    1,  194,    2, 0x08 /* Private */,
      26,    0,  197,    2, 0x08 /* Private */,
      27,    0,  198,    2, 0x08 /* Private */,
      28,    0,  199,    2, 0x08 /* Private */,
      29,    0,  200,    2, 0x08 /* Private */,
      30,    0,  201,    2, 0x08 /* Private */,
      31,    2,  202,    2, 0x08 /* Private */,
      35,    1,  207,    2, 0x08 /* Private */,
      38,    1,  210,    2, 0x08 /* Private */,
      41,    0,  213,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   22,
    QMetaType::Void, QMetaType::Bool,   24,
    QMetaType::Void, QMetaType::Bool,   24,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QByteArray, 0x80000000 | 33,   32,   34,
    QMetaType::Void, 0x80000000 | 36,   37,
    QMetaType::Void, 0x80000000 | 39,   40,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->on_exerciseSelectButton_clicked(); break;
        case 1: _t->on_settingsButton_clicked(); break;
        case 2: _t->on_squatButton_clicked(); break;
        case 3: _t->on_pushupButton_clicked(); break;
        case 4: _t->on_plankButton_clicked(); break;
        case 5: _t->on_lungeButton_clicked(); break;
        case 6: _t->on_jumpingJackButton_clicked(); break;
        case 7: _t->on_mountainClimberButton_clicked(); break;
        case 8: _t->on_burpeeButton_clicked(); break;
        case 9: _t->on_customButton_clicked(); break;
        case 10: _t->on_scrollUpButton_clicked(); break;
        case 11: _t->on_scrollDownButton_clicked(); break;
        case 12: _t->on_exerciseSelection_backButton_clicked(); break;
        case 13: _t->on_settings_connectButton_clicked(); break;
        case 14: _t->on_settings_disconnectButton_clicked(); break;
        case 15: _t->on_settings_calibrateButton_clicked(); break;
        case 16: _t->on_settings_testAirMouseButton_clicked(); break;
        case 17: _t->on_settings_saveButton_clicked(); break;
        case 18: _t->on_settings_backButton_clicked(); break;
        case 19: _t->on_sensitivitySlider_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 20: _t->on_smoothingCheckBox_toggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 21: _t->on_trailCheckBox_toggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 22: _t->on_workout_startButton_clicked(); break;
        case 23: _t->on_workout_stopButton_clicked(); break;
        case 24: _t->on_workout_backButton_clicked(); break;
        case 25: _t->onMqttConnected(); break;
        case 26: _t->onMqttDisconnected(); break;
        case 27: _t->onMqttMessageReceived((*reinterpret_cast< const QByteArray(*)>(_a[1])),(*reinterpret_cast< const QMqttTopicName(*)>(_a[2]))); break;
        case 28: _t->onMqttStateChanged((*reinterpret_cast< QMqttClient::ClientState(*)>(_a[1]))); break;
        case 29: _t->onMqttError((*reinterpret_cast< QMqttClient::ClientError(*)>(_a[1]))); break;
        case 30: _t->onWorkoutTimerTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 27:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMqttTopicName >(); break;
            }
            break;
        case 28:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMqttClient::ClientState >(); break;
            }
            break;
        case 29:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMqttClient::ClientError >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 31)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 31;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 31)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 31;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
