#include "gizmos/TranslateGizmo.hpp"

#include <QPainter>
#include <QPen>
#include <QColor>
#include <QPolygonF>

REGISTER_GIZMO(TranslateGizmo, "translate")

const bool TranslateGizmo::registered = true;

TranslateGizmo::TranslateGizmo(QObject* parent) : Gizmo(parent) { }

QString TranslateGizmo::GetId() const
{
    return QStringLiteral("translate");
}

QString TranslateGizmo::GetDisplayName() const
{
    return tr("Translate");
}

void TranslateGizmo::Draw(const GizmoContext& ctx)
{
    if (!ctx.painter)
        return;

    QPainter& painter = *ctx.painter;
    const QPointF center = ctx.itemViewBounds.center();

    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw X axis (red)
    painter.setPen(QPen(QColor(220, 60, 60), 3));
    painter.drawLine(center, center + QPointF(kArrowLength, 0));
    painter.setBrush(QColor(220, 60, 60));

    QPolygonF xArrow;
    xArrow << center + QPointF(kArrowLength + kArrowHeadSize, 0)
           << center + QPointF(kArrowLength - 2, -kArrowHeadSize / 2)
           << center + QPointF(kArrowLength - 2, kArrowHeadSize / 2);
    painter.drawPolygon(xArrow);

    // Draw Y axis (green)
    painter.setPen(QPen(QColor(60, 180, 60), 3));
    painter.drawLine(center, center + QPointF(0, -kArrowLength));
    painter.setBrush(QColor(60, 180, 60));

    QPolygonF yArrow;
    yArrow << center + QPointF(0, -kArrowLength - kArrowHeadSize)
           << center + QPointF(-kArrowHeadSize / 2, -kArrowLength + 2)
           << center + QPointF(kArrowHeadSize / 2, -kArrowLength + 2);
    painter.drawPolygon(yArrow);

    // Draw XY plane handle (blue)
    painter.setPen(QPen(QColor(60, 150, 220), 2));
    painter.setBrush(QColor(60, 150, 220, 100));
    painter.drawRect(QRectF(center.x(), center.y() - 25, 25, 25));
}

GizmoHitResult TranslateGizmo::HitTest(const QPointF& viewPos, const GizmoContext& ctx)
{
    const QPointF center = ctx.itemViewBounds.center();
    GizmoHitResult result;

    // Test XY plane handle first (highest priority)
    if (QRectF(center.x(), center.y() - 25, 25, 25).contains(viewPos))
    {
        result.handleId = "translate_xy";
        result.cursor = Qt::SizeAllCursor;
        result.tooltip = tr("Move on XY plane");
        return result;
    }

    // Test X axis
    QRectF xAxisRect(center.x(), center.y() - 8, kArrowLength + 15, 16);
    if (xAxisRect.contains(viewPos))
    {
        result.handleId = "translate_x";
        result.cursor = Qt::SizeHorCursor;
        result.tooltip = tr("Move along X axis");
        return result;
    }

    // Test Y axis
    QRectF yAxisRect(center.x() - 8, center.y() - kArrowLength - 15, 16, kArrowLength + 15);
    if (yAxisRect.contains(viewPos))
    {
        result.handleId = "translate_y";
        result.cursor = Qt::SizeVerCursor;
        result.tooltip = tr("Move along Y axis");
        return result;
    }

    return result;
}

QList<GizmoHandle> TranslateGizmo::GetHandles() const
{
    return {
        GizmoHandle("translate_xy", Qt::SizeAllCursor, 2),
        GizmoHandle("translate_x", Qt::SizeHorCursor, 1),
        GizmoHandle("translate_y", Qt::SizeVerCursor, 1)
    };
}
