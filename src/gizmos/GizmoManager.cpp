#include "gizmos/GizmoManager.hpp"

#include <QPainter>
#include <QGraphicsView>

#include "gizmos/Gizmo.hpp"

GizmoManager::GizmoManager(QObject* parent) : QObject(parent)
{
    // Create default gizmos from registry
    for (const QString& id : Gizmo::GetRegisteredIds())
    {
        if (Gizmo* gizmo = Gizmo::Create(id, this))
            m_gizmos.insert(id, gizmo);
    }
}

void GizmoManager::SetActiveGizmoId(const QString& gizmoId)
{
    if (m_activeGizmoId != gizmoId)
    {
        m_activeGizmoId = gizmoId;

        emit ActiveGizmoChanged(GetActiveGizmo());
    }
}

QString GizmoManager::GetActiveGizmoId() const noexcept
{
    return m_activeGizmoId;
}

Gizmo* GizmoManager::GetActiveGizmo() const
{
    return m_gizmos.value(m_activeGizmoId, nullptr);
}

Gizmo* GizmoManager::GetGizmo(const QString& id) const
{
    return m_gizmos.value(id, nullptr);
}

void GizmoManager::RegisterGizmo(Gizmo* gizmo)
{
    if (!gizmo)
        return;

    gizmo->setParent(this);
    m_gizmos.insert(gizmo->GetId(), gizmo);
}

void GizmoManager::Draw(QPainter& painter, const QRectF& itemSceneBounds, QGraphicsView* view, double rotation, const QPointF& scale)
{
    if (!view)
        return;

    Gizmo* gizmo = GetActiveGizmo();

    if (!gizmo)
        return;

    GizmoContext ctx;
    ctx.painter = &painter;
    ctx.itemSceneBounds = itemSceneBounds;
    ctx.itemViewBounds = view->mapFromScene(itemSceneBounds).boundingRect();
    ctx.itemCenter = ctx.itemViewBounds.center();
    ctx.rotation = rotation;
    ctx.scale = scale;
    ctx.zoom = view->transform().m11();
    ctx.isActive = !m_activeHandleId.isEmpty();
    ctx.activeHandleId = m_activeHandleId;

    gizmo->Draw(ctx);
}

GizmoHitResult GizmoManager::HitTest(const QPointF& viewPos, const QRectF& itemSceneBounds, QGraphicsView* view, double rotation, const QPointF& scale)
{
    if (!view)
        return GizmoHitResult();

    Gizmo* gizmo = GetActiveGizmo();

    if (!gizmo)
        return GizmoHitResult();

    GizmoContext ctx;
    ctx.itemSceneBounds = itemSceneBounds;
    ctx.itemViewBounds = view->mapFromScene(itemSceneBounds).boundingRect();
    ctx.itemCenter = ctx.itemViewBounds.center();
    ctx.rotation = rotation;
    ctx.scale = scale;
    ctx.zoom = view->transform().m11();
    ctx.isActive = !m_activeHandleId.isEmpty();
    ctx.activeHandleId = m_activeHandleId;

    return gizmo->HitTest(viewPos, ctx);
}

void GizmoManager::SetActiveHandle(const QString& handleId)
{
    m_activeHandleId = handleId;
}

void GizmoManager::ClearActiveHandle()
{
    m_activeHandleId.clear();
}

QString GizmoManager::GetActiveHandleId() const
{
    return m_activeHandleId;
}
