#include "input/InputHandler.hpp"

#include <QObject>
#include <QString>
#include <Qt>

#include "input/EditorContext.hpp"
#include "input/InputEvents.hpp"

InputResult InputResult::Consumed(Qt::CursorShape cursorShape, bool repaint)
{
    InputResult result;
    result.consumed = true;
    result.cursor = cursorShape;
    result.needsRepaint = repaint;

    return result;
}

InputResult InputResult::NotConsumed()
{
    return InputResult();
}

InputHandler::InputHandler(QObject* parent) : QObject(parent) { }

InputResult InputHandler::HandleDoubleClick(const MouseDoubleClickEvent& event, EditorContext& ctx)
{
    Q_UNUSED(event);
    Q_UNUSED(ctx);

    return InputResult::NotConsumed();
}

InputResult InputHandler::HandleWheel(const WheelEvent& event, EditorContext& ctx)
{
    Q_UNUSED(event);
    Q_UNUSED(ctx);

    return InputResult::NotConsumed();
}

InputResult InputHandler::HandleKeyPress(const KeyPressEvent& event, EditorContext& ctx)
{
    Q_UNUSED(event);
    Q_UNUSED(ctx);

    return InputResult::NotConsumed();
}

bool InputHandler::IsTransforming() const noexcept
{
    return false;
}

QString InputHandler::GetActiveHandleId() const
{
    return QString();
}

QString InputHandler::GetUndoActionName() const
{
    return QString();
}
