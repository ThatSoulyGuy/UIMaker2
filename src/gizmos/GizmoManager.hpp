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

    explicit GizmoManager(QObject* parent = nullptr);

    ~GizmoManager() = default;

    void SetActiveGizmoId(const QString& gizmoId);

    QString GetActiveGizmoId() const noexcept;

    Gizmo* GetActiveGizmo() const;

    Gizmo* GetGizmo(const QString& id) const;

    void RegisterGizmo(Gizmo* gizmo);

    void Draw(QPainter& painter, const QRectF& itemSceneBounds, QGraphicsView* view, double rotation = 0.0, const QPointF& scale = QPointF(1.0, 1.0));

    GizmoHitResult HitTest(const QPointF& viewPos, const QRectF& itemSceneBounds, QGraphicsView* view, double rotation = 0.0, const QPointF& scale = QPointF(1.0, 1.0));

    void SetActiveHandle(const QString& handleId);

    void ClearActiveHandle();

    QString GetActiveHandleId() const;

signals:

    void ActiveGizmoChanged(Gizmo* gizmo);

private:

    QHash<QString, Gizmo*> m_gizmos;
    QString m_activeGizmoId;
    QString m_activeHandleId;

};

#endif
