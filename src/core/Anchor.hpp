#ifndef CORE_ANCHOR_HPP
#define CORE_ANCHOR_HPP

#include <QObject>
#include <QMetaType>

class EnumHolder : public QObject
{

    Q_OBJECT

public:

    enum class Anchor
    {
        NONE = 0,
        LEFT = 0x1,
        RIGHT = 0x2,
        TOP = 0x4,
        BOTTOM = 0x8,
        CENTER_X = 0x10,
        CENTER_Y = 0x20
    };

    Q_ENUM(Anchor)
    Q_DECLARE_FLAGS(AnchorFlags, Anchor)
    Q_FLAG(AnchorFlags)
};

using Anchor = EnumHolder::Anchor;
using AnchorFlags = EnumHolder::AnchorFlags;

Q_DECLARE_METATYPE(AnchorFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(AnchorFlags)

#endif
