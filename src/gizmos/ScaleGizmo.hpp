#ifndef SCALEGIZMO_HPP
#define SCALEGIZMO_HPP

#include "gizmos/Gizmo.hpp"

class ScaleGizmo : public Gizmo
{
    Q_OBJECT

public:

    explicit ScaleGizmo(QObject* parent = nullptr);

    QString GetId() const override;

    QString GetDisplayName() const override;

    void Draw(const GizmoContext& ctx) override;

    GizmoHitResult HitTest(const QPointF& viewPos, const GizmoContext& ctx) override;

    QList<GizmoHandle> GetHandles() const override;

private:

    static const bool registered;

};

#endif
