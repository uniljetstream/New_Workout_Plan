/****************************************************************************
** Meta object code from reading C++ file 'settings_page_widget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "settings_page_widget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'settings_page_widget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SettingsPageWidget_t {
    QByteArrayData data[13];
    char stringdata0[195];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SettingsPageWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SettingsPageWidget_t qt_meta_stringdata_SettingsPageWidget = {
    {
QT_MOC_LITERAL(0, 0, 18), // "SettingsPageWidget"
QT_MOC_LITERAL(1, 19, 16), // "connectRequested"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 19), // "disconnectRequested"
QT_MOC_LITERAL(4, 57, 18), // "calibrateRequested"
QT_MOC_LITERAL(5, 76, 23), // "toggleAirMouseRequested"
QT_MOC_LITERAL(6, 100, 13), // "saveRequested"
QT_MOC_LITERAL(7, 114, 13), // "backRequested"
QT_MOC_LITERAL(8, 128, 18), // "sensitivityChanged"
QT_MOC_LITERAL(9, 147, 5), // "value"
QT_MOC_LITERAL(10, 153, 16), // "smoothingChanged"
QT_MOC_LITERAL(11, 170, 7), // "enabled"
QT_MOC_LITERAL(12, 178, 16) // "showTrailChanged"

    },
    "SettingsPageWidget\0connectRequested\0"
    "\0disconnectRequested\0calibrateRequested\0"
    "toggleAirMouseRequested\0saveRequested\0"
    "backRequested\0sensitivityChanged\0value\0"
    "smoothingChanged\0enabled\0showTrailChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SettingsPageWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x06 /* Public */,
       3,    0,   60,    2, 0x06 /* Public */,
       4,    0,   61,    2, 0x06 /* Public */,
       5,    0,   62,    2, 0x06 /* Public */,
       6,    0,   63,    2, 0x06 /* Public */,
       7,    0,   64,    2, 0x06 /* Public */,
       8,    1,   65,    2, 0x06 /* Public */,
      10,    1,   68,    2, 0x06 /* Public */,
      12,    1,   71,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double,    9,
    QMetaType::Void, QMetaType::Bool,   11,
    QMetaType::Void, QMetaType::Bool,   11,

       0        // eod
};

void SettingsPageWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SettingsPageWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connectRequested(); break;
        case 1: _t->disconnectRequested(); break;
        case 2: _t->calibrateRequested(); break;
        case 3: _t->toggleAirMouseRequested(); break;
        case 4: _t->saveRequested(); break;
        case 5: _t->backRequested(); break;
        case 6: _t->sensitivityChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 7: _t->smoothingChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->showTrailChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SettingsPageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsPageWidget::connectRequested)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SettingsPageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsPageWidget::disconnectRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SettingsPageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsPageWidget::calibrateRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SettingsPageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsPageWidget::toggleAirMouseRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SettingsPageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsPageWidget::saveRequested)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (SettingsPageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsPageWidget::backRequested)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (SettingsPageWidget::*)(double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsPageWidget::sensitivityChanged)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (SettingsPageWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsPageWidget::smoothingChanged)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (SettingsPageWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsPageWidget::showTrailChanged)) {
                *result = 8;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SettingsPageWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_SettingsPageWidget.data,
    qt_meta_data_SettingsPageWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SettingsPageWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsPageWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SettingsPageWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SettingsPageWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void SettingsPageWidget::connectRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SettingsPageWidget::disconnectRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SettingsPageWidget::calibrateRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SettingsPageWidget::toggleAirMouseRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SettingsPageWidget::saveRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SettingsPageWidget::backRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SettingsPageWidget::sensitivityChanged(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void SettingsPageWidget::smoothingChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void SettingsPageWidget::showTrailChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
