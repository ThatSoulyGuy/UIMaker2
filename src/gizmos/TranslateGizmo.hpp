#ifndef TRANSLATEGIZMO_HPP
#define TRANSLATEGIZMO_HPP

#include "gizmos/Gizmo.hpp"

class TranslateGizmo : public Gizmo
{
    Q_OBJECT

public:

    explicit TranslateGizmo(QObject* parent = nullptr);

    QString GetId() const override;

    QString GetDisplayName() const override;

    void Draw(const GizmoContext& ctx) override;

    GizmoHitResult HitTest(const QPointF& viewPos, const GizmoContext& ctx) override;

    QList<GizmoHandle> GetHandles() const override;

private:

    static const bool registered;

};

#endif
