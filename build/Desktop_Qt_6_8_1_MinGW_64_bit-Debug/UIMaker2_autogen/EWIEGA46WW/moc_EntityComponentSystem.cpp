/****************************************************************************
** Meta object code from reading C++ file 'EntityComponentSystem.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../EntityComponentSystem.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EntityComponentSystem.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9ComponentE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN9ComponentE = QtMocHelpers::stringData(
    "Component",
    "ComponentChanged",
    ""
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN9ComponentE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   20,    2, 0x06,    1 /* Public */,

 // signals: parameters
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject Component::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN9ComponentE.offsetsAndSizes,
    qt_meta_data_ZN9ComponentE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN9ComponentE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Component, std::true_type>,
        // method 'ComponentChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void Component::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Component *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->ComponentChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (Component::*)();
            if (_q_method_type _q_method = &Component::ComponentChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *Component::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Component::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN9ComponentE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Component::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void Component::ComponentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
namespace {
struct qt_meta_tag_ZN18TransformComponentE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN18TransformComponentE = QtMocHelpers::stringData(
    "TransformComponent",
    "ComponentChanged",
    "position",
    "rotationDegrees",
    "scale",
    "anchors",
    "AnchorFlags",
    "stretch",
    "Anchor",
    "NONE",
    "LEFT",
    "RIGHT",
    "TOP",
    "BOTTOM",
    "CENTER_X",
    "CENTER_Y"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN18TransformComponentE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       5,   14, // properties
       2,   39, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags, notifyId, revision
       2, QMetaType::QPointF, 0x00015003, uint(1879048193), 0,
       3, QMetaType::Double, 0x00015003, uint(1879048193), 0,
       4, QMetaType::QPointF, 0x00015003, uint(1879048193), 0,
       5, 0x80000000 | 6, 0x0001500b, uint(1879048193), 0,
       7, 0x80000000 | 6, 0x0001500b, uint(1879048193), 0,

 // enums: name, alias, flags, count, data
       8,    8, 0x2,    7,   49,
       6,    8, 0x3,    7,   63,

 // enum data: key, value
       9, uint(TransformComponent::Anchor::NONE),
      10, uint(TransformComponent::Anchor::LEFT),
      11, uint(TransformComponent::Anchor::RIGHT),
      12, uint(TransformComponent::Anchor::TOP),
      13, uint(TransformComponent::Anchor::BOTTOM),
      14, uint(TransformComponent::Anchor::CENTER_X),
      15, uint(TransformComponent::Anchor::CENTER_Y),
       9, uint(TransformComponent::Anchor::NONE),
      10, uint(TransformComponent::Anchor::LEFT),
      11, uint(TransformComponent::Anchor::RIGHT),
      12, uint(TransformComponent::Anchor::TOP),
      13, uint(TransformComponent::Anchor::BOTTOM),
      14, uint(TransformComponent::Anchor::CENTER_X),
      15, uint(TransformComponent::Anchor::CENTER_Y),

       0        // eod
};

Q_CONSTINIT const QMetaObject TransformComponent::staticMetaObject = { {
    QMetaObject::SuperData::link<Component::staticMetaObject>(),
    qt_meta_stringdata_ZN18TransformComponentE.offsetsAndSizes,
    qt_meta_data_ZN18TransformComponentE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN18TransformComponentE_t,
        // property 'position'
        QtPrivate::TypeAndForceComplete<QPointF, std::true_type>,
        // property 'rotationDegrees'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'scale'
        QtPrivate::TypeAndForceComplete<QPointF, std::true_type>,
        // property 'anchors'
        QtPrivate::TypeAndForceComplete<AnchorFlags, std::true_type>,
        // property 'stretch'
        QtPrivate::TypeAndForceComplete<AnchorFlags, std::true_type>,
        // enum 'Anchor'
        QtPrivate::TypeAndForceComplete<TransformComponent::Anchor, std::true_type>,
        // enum 'AnchorFlags'
        QtPrivate::TypeAndForceComplete<TransformComponent::AnchorFlags, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TransformComponent, std::true_type>
    >,
    nullptr
} };

void TransformComponent::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TransformComponent *>(_o);
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
        case 3:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< AnchorFlags >(); break;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QPointF*>(_v) = _t->GetPosition(); break;
        case 1: *reinterpret_cast< double*>(_v) = _t->GetRotationDegrees(); break;
        case 2: *reinterpret_cast< QPointF*>(_v) = _t->GetScale(); break;
        case 3: *reinterpret_cast<int*>(_v) = QFlag(_t->GetAnchors()); break;
        case 4: *reinterpret_cast<int*>(_v) = QFlag(_t->GetStretch()); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->SetPosition(*reinterpret_cast< QPointF*>(_v)); break;
        case 1: _t->SetRotationDegrees(*reinterpret_cast< double*>(_v)); break;
        case 2: _t->SetScale(*reinterpret_cast< QPointF*>(_v)); break;
        case 3: _t->SetAnchors(QFlag(*reinterpret_cast<int*>(_v))); break;
        case 4: _t->SetStretch(QFlag(*reinterpret_cast<int*>(_v))); break;
        default: break;
        }
    }
}

const QMetaObject *TransformComponent::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TransformComponent::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN18TransformComponentE.stringdata0))
        return static_cast<void*>(this);
    return Component::qt_metacast(_clname);
}

int TransformComponent::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Component::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
namespace CheckNotifySignalValidity_ZN18TransformComponentE {
template<typename T> using has_nullary_ComponentChanged = decltype(std::declval<T>().ComponentChanged());
template<typename T> using has_unary_ComponentChanged = decltype(std::declval<T>().ComponentChanged(std::declval<QPointF>()));
static_assert(qxp::is_detected_v<has_nullary_ComponentChanged, TransformComponent> || qxp::is_detected_v<has_unary_ComponentChanged, TransformComponent>,
              "NOTIFY signal ComponentChanged does not exist in class (or is private in its parent)");
}
namespace {
struct qt_meta_tag_ZN14ImageComponentE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN14ImageComponentE = QtMocHelpers::stringData(
    "ImageComponent",
    "ComponentChanged",
    "imagePath",
    "tint",
    "assetPath"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN14ImageComponentE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       3,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags, notifyId, revision
       2, QMetaType::QString, 0x00015003, uint(1879048193), 0,
       3, QMetaType::QColor, 0x00015003, uint(1879048193), 0,
       4, QMetaType::QString, 0x00015003, uint(1879048193), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject ImageComponent::staticMetaObject = { {
    QMetaObject::SuperData::link<Component::staticMetaObject>(),
    qt_meta_stringdata_ZN14ImageComponentE.offsetsAndSizes,
    qt_meta_data_ZN14ImageComponentE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN14ImageComponentE_t,
        // property 'imagePath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'tint'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'assetPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ImageComponent, std::true_type>
    >,
    nullptr
} };

void ImageComponent::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ImageComponent *>(_o);
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->GetImagePath(); break;
        case 1: *reinterpret_cast< QColor*>(_v) = _t->GetTint(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->GetAssetPath(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->SetImagePath(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->SetTint(*reinterpret_cast< QColor*>(_v)); break;
        case 2: _t->SetAssetPath(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *ImageComponent::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ImageComponent::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN14ImageComponentE.stringdata0))
        return static_cast<void*>(this);
    return Component::qt_metacast(_clname);
}

int ImageComponent::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Component::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
namespace CheckNotifySignalValidity_ZN14ImageComponentE {
template<typename T> using has_nullary_ComponentChanged = decltype(std::declval<T>().ComponentChanged());
template<typename T> using has_unary_ComponentChanged = decltype(std::declval<T>().ComponentChanged(std::declval<QString>()));
static_assert(qxp::is_detected_v<has_nullary_ComponentChanged, ImageComponent> || qxp::is_detected_v<has_unary_ComponentChanged, ImageComponent>,
              "NOTIFY signal ComponentChanged does not exist in class (or is private in its parent)");
}
namespace {
struct qt_meta_tag_ZN13TextComponentE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13TextComponentE = QtMocHelpers::stringData(
    "TextComponent",
    "ComponentChanged",
    "text",
    "fontFamily",
    "pixelSize",
    "color",
    "fontPath",
    "assetPath"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13TextComponentE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       6,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags, notifyId, revision
       2, QMetaType::QString, 0x00015003, uint(1879048193), 0,
       3, QMetaType::QString, 0x00015003, uint(1879048193), 0,
       4, QMetaType::Int, 0x00015003, uint(1879048193), 0,
       5, QMetaType::QColor, 0x00015003, uint(1879048193), 0,
       6, QMetaType::QString, 0x00015003, uint(1879048193), 0,
       7, QMetaType::QString, 0x00015003, uint(1879048193), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject TextComponent::staticMetaObject = { {
    QMetaObject::SuperData::link<Component::staticMetaObject>(),
    qt_meta_stringdata_ZN13TextComponentE.offsetsAndSizes,
    qt_meta_data_ZN13TextComponentE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13TextComponentE_t,
        // property 'text'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'fontFamily'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'pixelSize'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'color'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'fontPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'assetPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TextComponent, std::true_type>
    >,
    nullptr
} };

void TextComponent::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TextComponent *>(_o);
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->GetText(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->GetFontFamily(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->GetPixelSize(); break;
        case 3: *reinterpret_cast< QColor*>(_v) = _t->GetColor(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->GetFontPath(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->GetAssetPath(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->SetText(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->SetFontFamily(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->SetPixelSize(*reinterpret_cast< int*>(_v)); break;
        case 3: _t->SetColor(*reinterpret_cast< QColor*>(_v)); break;
        case 4: _t->SetFontPath(*reinterpret_cast< QString*>(_v)); break;
        case 5: _t->SetAssetPath(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *TextComponent::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TextComponent::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13TextComponentE.stringdata0))
        return static_cast<void*>(this);
    return Component::qt_metacast(_clname);
}

int TextComponent::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Component::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
namespace CheckNotifySignalValidity_ZN13TextComponentE {
template<typename T> using has_nullary_ComponentChanged = decltype(std::declval<T>().ComponentChanged());
template<typename T> using has_unary_ComponentChanged = decltype(std::declval<T>().ComponentChanged(std::declval<QString>()));
static_assert(qxp::is_detected_v<has_nullary_ComponentChanged, TextComponent> || qxp::is_detected_v<has_unary_ComponentChanged, TextComponent>,
              "NOTIFY signal ComponentChanged does not exist in class (or is private in its parent)");
}
namespace {
struct qt_meta_tag_ZN15ButtonComponentE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN15ButtonComponentE = QtMocHelpers::stringData(
    "ButtonComponent",
    "ComponentChanged",
    "text",
    "backgroundColor",
    "textColor",
    "fontFamily",
    "pixelSize",
    "fontPath",
    "assetPath"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN15ButtonComponentE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       7,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags, notifyId, revision
       2, QMetaType::QString, 0x00015003, uint(1879048193), 0,
       3, QMetaType::QColor, 0x00015003, uint(1879048193), 0,
       4, QMetaType::QColor, 0x00015003, uint(1879048193), 0,
       5, QMetaType::QString, 0x00015003, uint(1879048193), 0,
       6, QMetaType::Int, 0x00015003, uint(1879048193), 0,
       7, QMetaType::QString, 0x00015003, uint(1879048193), 0,
       8, QMetaType::QString, 0x00015003, uint(1879048193), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject ButtonComponent::staticMetaObject = { {
    QMetaObject::SuperData::link<Component::staticMetaObject>(),
    qt_meta_stringdata_ZN15ButtonComponentE.offsetsAndSizes,
    qt_meta_data_ZN15ButtonComponentE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN15ButtonComponentE_t,
        // property 'text'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'backgroundColor'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'textColor'
        QtPrivate::TypeAndForceComplete<QColor, std::true_type>,
        // property 'fontFamily'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'pixelSize'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'fontPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'assetPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ButtonComponent, std::true_type>
    >,
    nullptr
} };

void ButtonComponent::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ButtonComponent *>(_o);
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->GetText(); break;
        case 1: *reinterpret_cast< QColor*>(_v) = _t->GetBackgroundColor(); break;
        case 2: *reinterpret_cast< QColor*>(_v) = _t->GetTextColor(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->GetFontFamily(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->GetPixelSize(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->GetFontPath(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->GetAssetPath(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->SetText(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->SetBackgroundColor(*reinterpret_cast< QColor*>(_v)); break;
        case 2: _t->SetTextColor(*reinterpret_cast< QColor*>(_v)); break;
        case 3: _t->SetFontFamily(*reinterpret_cast< QString*>(_v)); break;
        case 4: _t->SetPixelSize(*reinterpret_cast< int*>(_v)); break;
        case 5: _t->SetFontPath(*reinterpret_cast< QString*>(_v)); break;
        case 6: _t->SetAssetPath(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *ButtonComponent::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ButtonComponent::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN15ButtonComponentE.stringdata0))
        return static_cast<void*>(this);
    return Component::qt_metacast(_clname);
}

int ButtonComponent::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Component::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
namespace CheckNotifySignalValidity_ZN15ButtonComponentE {
template<typename T> using has_nullary_ComponentChanged = decltype(std::declval<T>().ComponentChanged());
template<typename T> using has_unary_ComponentChanged = decltype(std::declval<T>().ComponentChanged(std::declval<QString>()));
static_assert(qxp::is_detected_v<has_nullary_ComponentChanged, ButtonComponent> || qxp::is_detected_v<has_unary_ComponentChanged, ButtonComponent>,
              "NOTIFY signal ComponentChanged does not exist in class (or is private in its parent)");
}
namespace {
struct qt_meta_tag_ZN9UiElementE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN9UiElementE = QtMocHelpers::stringData(
    "UiElement",
    "NameChanged",
    "",
    "newName",
    "StructureChanged",
    "ComponentListChanged",
    "UiElement*",
    "name"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN9UiElementE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       1,   39, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   32,    2, 0x06,    2 /* Public */,
       4,    0,   35,    2, 0x06,    4 /* Public */,
       5,    1,   36,    2, 0x06,    5 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    2,

 // properties: name, type, flags, notifyId, revision
       7, QMetaType::QString, 0x00015003, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject UiElement::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN9UiElementE.offsetsAndSizes,
    qt_meta_data_ZN9UiElementE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN9UiElementE_t,
        // property 'name'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UiElement, std::true_type>,
        // method 'NameChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'StructureChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'ComponentListChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<UiElement *, std::false_type>
    >,
    nullptr
} };

void UiElement::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<UiElement *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->NameChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->StructureChanged(); break;
        case 2: _t->ComponentListChanged((*reinterpret_cast< std::add_pointer_t<UiElement*>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< UiElement* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (UiElement::*)(const QString & );
            if (_q_method_type _q_method = &UiElement::NameChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (UiElement::*)();
            if (_q_method_type _q_method = &UiElement::StructureChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (UiElement::*)(UiElement * );
            if (_q_method_type _q_method = &UiElement::ComponentListChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->GetName(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->SetName(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *UiElement::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UiElement::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN9UiElementE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UiElement::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void UiElement::NameChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void UiElement::StructureChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void UiElement::ComponentListChanged(UiElement * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
