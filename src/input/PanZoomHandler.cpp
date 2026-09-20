#include "input/PanZoomHandler.hpp"

#include <QScrollBar>
#include <QGraphicsView>
#include <QTransform>
#include <QPointF>
#include <QPoint>
#include <Qt>
#include <cmath>

#include "input/EditorContext.hpp"
#include "input/InputEvents.hpp"

PanZoomHandler::PanZoomHandler(QObject* parent) : InputHandler(parent) { }

InputResult PanZoomHandler::HandlePress(const MousePressEvent& event, EditorContext& ctx)
{
    if (event.button == Qt::MiddleButton ||
        (event.button == Qt::LeftButton && (event.modifiers & Qt::ShiftModifier)))
    {
        m_panning = true;
        m_lastPanPoint = event.viewPos;

        return InputResult::Consumed(Qt::ClosedHandCursor, false);
    }

    return InputResult::NotConsumed();
}

InputResult PanZoomHandler::HandleMove(const MouseMoveEvent& event, EditorContext& ctx)
{
    if (m_panning && ctx.view)
    {
        const QPoint delta = event.viewPos - m_lastPanPoint;
        m_lastPanPoint = event.viewPos;

        ctx.view->horizontalScrollBar()->setValue(ctx.view->horizontalScrollBar()->value() - delta.x());
        ctx.view->verticalScrollBar()->setValue(ctx.view->verticalScrollBar()->value() - delta.y());

        return InputResult::Consumed(Qt::ClosedHandCursor, false);
    }

    return InputResult::NotConsumed();
}

InputResult PanZoomHandler::HandleRelease(const MouseReleaseEvent& event, EditorContext& ctx)
{
    Q_UNUSED(ctx);

    if (m_panning && (event.button == Qt::MiddleButton || event.button == Qt::LeftButton))
    {
        m_panning = false;

        return InputResult::Consumed(Qt::ArrowCursor, false);
    }

    return InputResult::NotConsumed();
}

InputResult PanZoomHandler::HandleWheel(const WheelEvent& event, EditorContext& ctx)
{
    if (!ctx.view)
        return InputResult::NotConsumed();

    // Ctrl/Cmd + anything means zoom, on every device.
    const bool zoomModifier = event.modifiers & (Qt::ControlModifier | Qt::MetaModifier);

    // A trackpad swipe scrolls the canvas; a wheel detent zooms. Previously
    // EVERY wheel event zoomed, so two-finger scrolling on a trackpad zoomed
    // erratically in and out - and a horizontal-only swipe had angleDelta.y()==0
    // so it fell through to QGraphicsView and scrolled instead. Two different
    // responses to one gesture is what made it feel like a jittery mess.
    if (event.fromTrackpad && !zoomModifier)
    {
        // pixelDelta is the device's own precise scroll amount; angleDelta is
        // the fallback for a precision device that reports no pixels.
        QPoint d = event.pixelDelta;

        if (d.isNull())
            d = event.angleDelta / 8;   // degrees -> approximate pixels

        if (d.isNull())
            return InputResult::NotConsumed();

        ctx.view->horizontalScrollBar()->setValue(ctx.view->horizontalScrollBar()->value() - d.x());
        ctx.view->verticalScrollBar()->setValue(ctx.view->verticalScrollBar()->value() - d.y());

        return InputResult::Consumed(Qt::ArrowCursor, true);
    }

    const int delta = event.angleDelta.y() != 0 ? event.angleDelta.y() : event.angleDelta.x();

    if (delta == 0)
        return InputResult::NotConsumed();

    ZoomAt(ctx.view, event.viewPos, std::pow(1.0015, static_cast<double>(delta)));

    return InputResult::Consumed(Qt::ArrowCursor, true);
}

InputResult PanZoomHandler::HandlePinch(const QPoint& viewPos, double scaleFactor, EditorContext& ctx)
{
    if (!ctx.view || scaleFactor <= 0.0)
        return InputResult::NotConsumed();

    // macOS reports pinch as an incremental fraction (+0.01 = 1% bigger).
    ZoomAt(ctx.view, viewPos, 1.0 + scaleFactor);

    return InputResult::Consumed(Qt::ArrowCursor, true);
}

bool PanZoomHandler::IsPanning() const noexcept
{
    return m_panning;
}

void PanZoomHandler::ZoomAt(QGraphicsView* view, const QPoint& viewPos, double factor)
{
    if (!view)
        return;

    QTransform t = view->transform();

    const double current = t.m11();
    double clampedFactor = factor;

    if (current * factor < m_minZoom)
        clampedFactor = m_minZoom / current;
    if (current * factor > m_maxZoom)
        clampedFactor = m_maxZoom / current;

    if (std::abs(clampedFactor - 1.0) < 1e-9)
        return;

    const QPointF scenePosBefore = view->mapToScene(viewPos);

    view->scale(clampedFactor, clampedFactor);

    const QPointF scenePosAfter = view->mapToScene(viewPos);
    const QPointF delta = scenePosAfter - scenePosBefore;

    view->translate(delta.x(), delta.y());
}
