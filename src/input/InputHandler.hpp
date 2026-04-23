#ifndef INPUTHANDLER_HPP
#define INPUTHANDLER_HPP

#include <QObject>
#include <QString>
#include <QByteArray>
#include <Qt>

#include "input/EditorContext.hpp"
#include "input/InputEvents.hpp"

struct InputResult
{
    bool consumed = false;
    Qt::CursorShape cursor = Qt::ArrowCursor;
    bool needsRepaint = false;
    QString statusMessage;

    static InputResult Consumed(Qt::CursorShape cursorShape = Qt::ArrowCursor, bool repaint = true)
    {
        InputResult result;
        result.consumed = true;
        result.cursor = cursorShape;
        result.needsRepaint = repaint;

        return result;
    }

    static InputResult NotConsumed()
    {
        return InputResult();
    }
};

class InputHandler : public QObject
{
    Q_OBJECT

public:

    explicit InputHandler(QObject* parent = nullptr) : QObject(parent) { }

    virtual ~InputHandler() = default;

    virtual InputResult HandlePress(const MousePressEvent& event, EditorContext& ctx) = 0;
    virtual InputResult HandleMove(const MouseMoveEvent& event, EditorContext& ctx) = 0;
    virtual InputResult HandleRelease(const MouseReleaseEvent& event, EditorContext& ctx) = 0;

    virtual InputResult HandleDoubleClick(const MouseDoubleClickEvent& event, EditorContext& ctx)
    {
        Q_UNUSED(event);
        Q_UNUSED(ctx);

        return InputResult::NotConsumed();
    }

    virtual InputResult HandleWheel(const WheelEvent& event, EditorContext& ctx)
    {
        Q_UNUSED(event);
        Q_UNUSED(ctx);

        return InputResult::NotConsumed();
    }

    virtual InputResult HandleKeyPress(const KeyPressEvent& event, EditorContext& ctx)
    {
        Q_UNUSED(event);
        Q_UNUSED(ctx);

        return InputResult::NotConsumed();
    }

    virtual bool IsTransforming() const noexcept
    {
        return false;
    }

    virtual QString GetActiveHandleId() const
    {
        return QString();
    }

    virtual QString GetUndoActionName() const
    {
        return QString();
    }

signals:

    void TransformStarted();
    void TransformUpdated();
    void TransformEnded(const QByteArray& beforeState, const QByteArray& afterState, const QString& actionName);
    void TransformCancelled();

};

#endif
