#ifndef INPUTEVENTS_HPP
#define INPUTEVENTS_HPP

#include <QPointF>
#include <QPoint>
#include <Qt>

struct InputEvent
{
    QPoint viewPos;
    QPointF scenePos;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = Qt::NoButton;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
};

struct MousePressEvent : public InputEvent { };
struct MouseMoveEvent : public InputEvent { };
struct MouseReleaseEvent : public InputEvent { };
struct MouseDoubleClickEvent : public InputEvent { };

struct WheelEvent
{
    QPoint viewPos;
    QPointF scenePos;

    // Kept for existing callers: angleDelta.y(), or .x() for a horizontal-only
    // wheel. On its own it cannot distinguish a trackpad swipe from a wheel
    // click, which is why the fields below exist.
    int delta = 0;
    Qt::Orientation orientation = Qt::Vertical;

    // Raw deltas. pixelDelta is non-null only for high-resolution devices -
    // trackpads and precision mice - where it is the correct thing to scroll by.
    QPoint angleDelta;
    QPoint pixelDelta;

    // True when the event came from a trackpad/precision device, i.e. the user
    // is swiping to scroll rather than clicking a wheel detent.
    bool fromTrackpad = false;

    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
};

struct KeyPressEvent
{
    int key = 0;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    QString text;
    bool isAutoRepeat = false;
};

struct KeyReleaseEvent
{
    int key = 0;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    QString text;
};

#endif
