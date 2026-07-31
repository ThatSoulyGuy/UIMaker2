#ifndef ROTATEGIZMO_HPP
#define ROTATEGIZMO_HPP

#include "gizmos/Gizmo.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class RotateGizmo : public Gizmo
{
    Q_OBJECT

public:

    explicit RotateGizmo(QObject* parent = nullptr);

    QString GetId() const override;

    QString GetDisplayName() const override;

    void Draw(const GizmoContext& ctx) override;

    GizmoHitResult HitTest(const QPointF& viewPos, const GizmoContext& ctx) override;

    QList<GizmoHandle> GetHandles() const override;

private:

    static const bool registered;

};

#endif
