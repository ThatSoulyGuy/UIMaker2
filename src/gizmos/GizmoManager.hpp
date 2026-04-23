#ifndef GIZMOMANAGER_HPP
#define GIZMOMANAGER_HPP

#include <QObject>
#include <QHash>
#include <QGraphicsView>
#include <memory>

#include "gizmos/Gizmo.hpp"

class GizmoManager : public QObject
{
    Q_OBJECT

public:

    explicit GizmoManager(QObject* parent = nullptr) : QObject(parent)
    {
        // Create default gizmos from registry
        for (const QString& id : Gizmo::GetRegisteredIds())
        {
            if (Gizmo* gizmo = Gizmo::Create(id, this))
                m_gizmos.insert(id, gizmo);
        }
    }

    ~GizmoManager() = default;

    void SetActiveGizmoId(const QString& gizmoId)
    {
        if (m_activeGizmoId != gizmoId)
        {
            m_activeGizmoId = gizmoId;

            emit ActiveGizmoChanged(GetActiveGizmo());
        }
    }

    QString GetActiveGizmoId() const noexcept
    {
        return m_activeGizmoId;
    }

    Gizmo* GetActiveGizmo() const
    {
        return m_gizmos.value(m_activeGizmoId, nullptr);
    }

    Gizmo* GetGizmo(const QString& id) const
    {
        return m_gizmos.value(id, nullptr);
    }

    void RegisterGizmo(Gizmo* gizmo)
    {
        if (!gizmo)
            return;

        gizmo->setParent(this);
        m_gizmos.insert(gizmo->GetId(), gizmo);
    }

    void Draw(QPainter& painter, const QRectF& itemSceneBounds, QGraphicsView* view, double rotation = 0.0, const QPointF& scale = QPointF(1.0, 1.0))
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

    GizmoHitResult HitTest(const QPointF& viewPos, const QRectF& itemSceneBounds, QGraphicsView* view, double rotation = 0.0, const QPointF& scale = QPointF(1.0, 1.0))
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

    void SetActiveHandle(const QString& handleId)
    {
        m_activeHandleId = handleId;
    }

    void ClearActiveHandle()
    {
        m_activeHandleId.clear();
    }

    QString GetActiveHandleId() const
    {
        return m_activeHandleId;
    }

signals:

    void ActiveGizmoChanged(Gizmo* gizmo);

private:

    QHash<QString, Gizmo*> m_gizmos;
    QString m_activeGizmoId;
    QString m_activeHandleId;

};

#endif
