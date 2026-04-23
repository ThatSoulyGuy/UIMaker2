#ifndef SCALEGIZMO_HPP
#define SCALEGIZMO_HPP

#include "gizmos/Gizmo.hpp"

class ScaleGizmo : public Gizmo
{
    Q_OBJECT

public:

    explicit ScaleGizmo(QObject* parent = nullptr) : Gizmo(parent) { }

    QString GetId() const override
    {
        return QStringLiteral("scale");
    }

    QString GetDisplayName() const override
    {
        return tr("Scale");
    }

    void Draw(const GizmoContext& ctx) override
    {
        if (!ctx.painter)
            return;

        QPainter& painter = *ctx.painter;
        const QRectF viewRect = ctx.itemViewBounds;
        const QPointF center = viewRect.center();

        painter.setRenderHint(QPainter::Antialiasing, true);

        // Draw bounding box outline
        painter.setPen(QPen(QColor(60, 150, 220), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(viewRect.adjusted(-2, -2, 2, 2));

        // Draw corner handles (blue)
        painter.setBrush(QColor(60, 150, 220));
        QRectF handle(-kHandleSize / 2, -kHandleSize / 2, kHandleSize, kHandleSize);

        painter.drawRect(handle.translated(viewRect.topLeft()));
        painter.drawRect(handle.translated(viewRect.topRight()));
        painter.drawRect(handle.translated(viewRect.bottomLeft()));
        painter.drawRect(handle.translated(viewRect.bottomRight()));

        // Draw edge handles (light blue)
        painter.setBrush(QColor(100, 180, 240));
        painter.drawRect(handle.translated((viewRect.topLeft() + viewRect.topRight()) / 2));
        painter.drawRect(handle.translated((viewRect.bottomLeft() + viewRect.bottomRight()) / 2));
        painter.drawRect(handle.translated((viewRect.topLeft() + viewRect.bottomLeft()) / 2));
        painter.drawRect(handle.translated((viewRect.topRight() + viewRect.bottomRight()) / 2));

        // Draw center handle for uniform scaling (orange)
        painter.setBrush(QColor(220, 180, 60));
        painter.drawRect(handle.translated(center));
    }

    GizmoHitResult HitTest(const QPointF& viewPos, const GizmoContext& ctx) override
    {
        const QRectF viewRect = ctx.itemViewBounds;
        const QPointF center = viewRect.center();
        GizmoHitResult result;

        const int hitSize = 14;

        auto hitTest = [&](const QPointF& pt) {
            return QRectF(pt.x() - hitSize / 2, pt.y() - hitSize / 2, hitSize, hitSize).contains(viewPos);
        };

        // Test corner handles
        if (hitTest(viewRect.topLeft()))
        {
            result.handleId = "scale_top_left";
            result.cursor = Qt::SizeFDiagCursor;
            return result;
        }
        if (hitTest(viewRect.topRight()))
        {
            result.handleId = "scale_top_right";
            result.cursor = Qt::SizeBDiagCursor;
            return result;
        }
        if (hitTest(viewRect.bottomLeft()))
        {
            result.handleId = "scale_bottom_left";
            result.cursor = Qt::SizeBDiagCursor;
            return result;
        }
        if (hitTest(viewRect.bottomRight()))
        {
            result.handleId = "scale_bottom_right";
            result.cursor = Qt::SizeFDiagCursor;
            return result;
        }

        // Test edge handles
        if (hitTest((viewRect.topLeft() + viewRect.topRight()) / 2))
        {
            result.handleId = "scale_top";
            result.cursor = Qt::SizeVerCursor;
            return result;
        }
        if (hitTest((viewRect.bottomLeft() + viewRect.bottomRight()) / 2))
        {
            result.handleId = "scale_bottom";
            result.cursor = Qt::SizeVerCursor;
            return result;
        }
        if (hitTest((viewRect.topLeft() + viewRect.bottomLeft()) / 2))
        {
            result.handleId = "scale_left";
            result.cursor = Qt::SizeHorCursor;
            return result;
        }
        if (hitTest((viewRect.topRight() + viewRect.bottomRight()) / 2))
        {
            result.handleId = "scale_right";
            result.cursor = Qt::SizeHorCursor;
            return result;
        }

        // Test center handle for uniform scaling
        if (hitTest(center))
        {
            result.handleId = "scale_uniform";
            result.cursor = Qt::SizeAllCursor;
            return result;
        }

        return result;
    }

    QList<GizmoHandle> GetHandles() const override
    {
        return {
            GizmoHandle("scale_top_left", Qt::SizeFDiagCursor, 1),
            GizmoHandle("scale_top_right", Qt::SizeBDiagCursor, 1),
            GizmoHandle("scale_bottom_left", Qt::SizeBDiagCursor, 1),
            GizmoHandle("scale_bottom_right", Qt::SizeFDiagCursor, 1),
            GizmoHandle("scale_top", Qt::SizeVerCursor, 0),
            GizmoHandle("scale_bottom", Qt::SizeVerCursor, 0),
            GizmoHandle("scale_left", Qt::SizeHorCursor, 0),
            GizmoHandle("scale_right", Qt::SizeHorCursor, 0),
            GizmoHandle("scale_uniform", Qt::SizeAllCursor, 2)
        };
    }

private:

    static const bool registered;

};

#endif
