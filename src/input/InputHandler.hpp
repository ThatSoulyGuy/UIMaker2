#ifndef INPUTHANDLER_HPP
#define INPUTHANDLER_HPP

#include <QObject>
#include <QString>
#include <QByteArray>
#include <Qt>

#include "input/EditorContext.hpp"
#include "input/InputEvents.hpp"
#include "scene/TransformDelta.hpp"

struct InputResult
{
    bool consumed = false;
    Qt::CursorShape cursor = Qt::ArrowCursor;
    bool needsRepaint = false;
    QString statusMessage;

    static InputResult Consumed(Qt::CursorShape cursorShape = Qt::ArrowCursor, bool repaint = true);

    static InputResult NotConsumed();
};

class InputHandler : public QObject
{
    Q_OBJECT

public:

    explicit InputHandler(QObject* parent = nullptr);

    virtual ~InputHandler() = default;

    virtual InputResult HandlePress(const MousePressEvent& event, EditorContext& ctx) = 0;
    virtual InputResult HandleMove(const MouseMoveEvent& event, EditorContext& ctx) = 0;
    virtual InputResult HandleRelease(const MouseReleaseEvent& event, EditorContext& ctx) = 0;

    virtual InputResult HandleDoubleClick(const MouseDoubleClickEvent& event, EditorContext& ctx);

    virtual InputResult HandleWheel(const WheelEvent& event, EditorContext& ctx);

    virtual InputResult HandleKeyPress(const KeyPressEvent& event, EditorContext& ctx);

    virtual bool IsTransforming() const noexcept;

    virtual QString GetActiveHandleId() const;

    virtual QString GetUndoActionName() const;

signals:

    void TransformStarted();
    void TransformUpdated();
    void TransformEnded(const QList<TransformDelta>& deltas, const QString& actionName);
    void TransformCancelled();

};

#endif
