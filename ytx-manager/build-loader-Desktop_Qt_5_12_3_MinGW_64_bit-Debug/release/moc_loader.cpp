/****************************************************************************
** Meta object code from reading C++ file 'loader.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../../Desktop/ytx-manager/loader/loader.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'loader.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_midiPortsDialog_t {
    QByteArrayData data[1];
    char stringdata0[16];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_midiPortsDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_midiPortsDialog_t qt_meta_stringdata_midiPortsDialog = {
    {
QT_MOC_LITERAL(0, 0, 15) // "midiPortsDialog"

    },
    "midiPortsDialog"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_midiPortsDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void midiPortsDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject midiPortsDialog::staticMetaObject = { {
    &QDialog::staticMetaObject,
    qt_meta_stringdata_midiPortsDialog.data,
    qt_meta_data_midiPortsDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *midiPortsDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *midiPortsDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_midiPortsDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int midiPortsDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_loader_t {
    QByteArrayData data[9];
    char stringdata0[101];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_loader_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_loader_t qt_meta_stringdata_loader = {
    {
QT_MOC_LITERAL(0, 0, 6), // "loader"
QT_MOC_LITERAL(1, 7, 8), // "midiPull"
QT_MOC_LITERAL(2, 16, 0), // ""
QT_MOC_LITERAL(3, 17, 14), // "firmwareUpdate"
QT_MOC_LITERAL(4, 32, 15), // "connectToDevice"
QT_MOC_LITERAL(5, 48, 5), // "index"
QT_MOC_LITERAL(6, 54, 11), // "searchPorts"
QT_MOC_LITERAL(7, 66, 18), // "checkMidiConection"
QT_MOC_LITERAL(8, 85, 15) // "sendNotesRandom"

    },
    "loader\0midiPull\0\0firmwareUpdate\0"
    "connectToDevice\0index\0searchPorts\0"
    "checkMidiConection\0sendNotesRandom"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_loader[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x08 /* Private */,
       3,    0,   45,    2, 0x08 /* Private */,
       4,    1,   46,    2, 0x08 /* Private */,
       6,    0,   49,    2, 0x08 /* Private */,
       7,    0,   50,    2, 0x08 /* Private */,
       8,    0,   51,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void loader::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<loader *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->midiPull(); break;
        case 1: _t->firmwareUpdate(); break;
        case 2: _t->connectToDevice((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->searchPorts(); break;
        case 4: _t->checkMidiConection(); break;
        case 5: _t->sendNotesRandom(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject loader::staticMetaObject = { {
    &QMainWindow::staticMetaObject,
    qt_meta_stringdata_loader.data,
    qt_meta_data_loader,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *loader::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *loader::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_loader.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int loader::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
