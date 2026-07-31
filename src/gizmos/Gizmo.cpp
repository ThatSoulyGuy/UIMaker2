#include "gizmos/Gizmo.hpp"

#include <QString>
#include <QList>
#include <QHash>

GizmoHandle::GizmoHandle(const QString& handleId, Qt::CursorShape cursorShape, int handlePriority)
    : id(handleId)
    , cursor(cursorShape)
    , priority(handlePriority)
{
}

bool GizmoHitResult::IsHit() const noexcept
{
    return !handleId.isEmpty();
}

Gizmo::Gizmo(QObject* parent) : QObject(parent) { }

Qt::CursorShape Gizmo::GetCursor(const QString& handleId) const
{
    for (const auto& handle : GetHandles())
    {
        if (handle.id == handleId)
            return handle.cursor;
    }

    return Qt::ArrowCursor;
}

QHash<QString, Gizmo::GizmoFactory>& Gizmo::Registry()
{
    static QHash<QString, GizmoFactory> registry;

    return registry;
}

void Gizmo::Register(const QString& id, GizmoFactory factory)
{
    Registry().insert(id, factory);
}

Gizmo* Gizmo::Create(const QString& id, QObject* parent)
{
    auto it = Registry().find(id);

    return it != Registry().end() ? it.value()(parent) : nullptr;
}

QStringList Gizmo::GetRegisteredIds()
{
    return Registry().keys();
}
