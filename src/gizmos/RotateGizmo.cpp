#include "gizmos/RotateGizmo.hpp"

#include <QPainter>
#include <QPen>
#include <QColor>

REGISTER_GIZMO(RotateGizmo, "rotate")

const bool RotateGizmo::registered = true;

RotateGizmo::RotateGizmo(QObject* parent) : Gizmo(parent) { }

QString RotateGizmo::GetId() const
{
    return QStringLiteral("rotate");
}

QString RotateGizmo::GetDisplayName() const
{
    return tr("Rotate");
}

void RotateGizmo::Draw(const GizmoContext& ctx)
{
    if (!ctx.painter)
        return;

    QPainter& painter = *ctx.painter;
    const QPointF center = ctx.itemViewBounds.center();

    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw rotation ring
    painter.setPen(QPen(QColor(60, 150, 220), 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, kRotateRadius, kRotateRadius);

    // Draw handle points at cardinal directions
    painter.setBrush(QColor(60, 150, 220));
    for (int i = 0; i < 4; ++i)
    {
        double angle = i * M_PI / 2.0;
        QPointF handlePos = center + QPointF(std::cos(angle) * kRotateRadius, -std::sin(angle) * kRotateRadius);
        painter.drawEllipse(handlePos, kHandleSize / 2, kHandleSize / 2);
    }

    // Draw reference line showing current rotation angle
    double currentRot = ctx.rotation * M_PI / 180.0;
    painter.setPen(QPen(QColor(220, 180, 60), 2));
    QPointF rotatedEnd = center + QPointF(std::sin(currentRot) * kRotateRadius,
                                           -std::cos(currentRot) * kRotateRadius);
    painter.drawLine(center, rotatedEnd);
}

GizmoHitResult RotateGizmo::HitTest(const QPointF& viewPos, const GizmoContext& ctx)
{
    const QPointF center = ctx.itemViewBounds.center();
    GizmoHitResult result;

    QPointF toMouse = viewPos - center;
    double dist = std::hypot(toMouse.x(), toMouse.y());

    if (std::abs(dist - kRotateRadius) < kHitTolerance)
    {
        result.handleId = "rotate_ring";
        result.cursor = Qt::CrossCursor;
        result.tooltip = tr("Rotate");
        return result;
    }

    return result;
}

QList<GizmoHandle> RotateGizmo::GetHandles() const
{
    return {
        GizmoHandle("rotate_ring", Qt::CrossCursor, 1)
    };
}
