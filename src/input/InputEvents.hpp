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
    int delta = 0;
    Qt::Orientation orientation = Qt::Vertical;
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
