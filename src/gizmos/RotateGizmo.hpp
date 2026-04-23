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

    explicit RotateGizmo(QObject* parent = nullptr) : Gizmo(parent) { }

    QString GetId() const override
    {
        return QStringLiteral("rotate");
    }

    QString GetDisplayName() const override
    {
        return tr("Rotate");
    }

    void Draw(const GizmoContext& ctx) override
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

    GizmoHitResult HitTest(const QPointF& viewPos, const GizmoContext& ctx) override
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

    QList<GizmoHandle> GetHandles() const override
    {
        return {
            GizmoHandle("rotate_ring", Qt::CrossCursor, 1)
        };
    }

private:

    static const bool registered;

};

#endif
