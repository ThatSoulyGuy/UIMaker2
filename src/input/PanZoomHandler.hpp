#ifndef PANZOOMHANDLER_HPP
#define PANZOOMHANDLER_HPP

#include <QScrollBar>
#include <cmath>

#include "input/InputHandler.hpp"

class PanZoomHandler : public InputHandler
{
    Q_OBJECT

public:

    explicit PanZoomHandler(QObject* parent = nullptr) : InputHandler(parent) { }

    InputResult HandlePress(const MousePressEvent& event, EditorContext& ctx) override
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

    InputResult HandleMove(const MouseMoveEvent& event, EditorContext& ctx) override
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

    InputResult HandleRelease(const MouseReleaseEvent& event, EditorContext& ctx) override
    {
        Q_UNUSED(ctx);

        if (m_panning && (event.button == Qt::MiddleButton || event.button == Qt::LeftButton))
        {
            m_panning = false;

            return InputResult::Consumed(Qt::ArrowCursor, false);
        }

        return InputResult::NotConsumed();
    }

    InputResult HandleWheel(const WheelEvent& event, EditorContext& ctx) override
    {
        if (event.delta != 0 && ctx.view)
        {
            const double factor = std::pow(1.0015, static_cast<double>(event.delta));

            ZoomAt(ctx.view, event.viewPos, factor);

            return InputResult::Consumed(Qt::ArrowCursor, true);
        }

        return InputResult::NotConsumed();
    }

    bool IsPanning() const noexcept
    {
        return m_panning;
    }

private:

    void ZoomAt(QGraphicsView* view, const QPoint& viewPos, double factor)
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

    bool m_panning = false;
    QPoint m_lastPanPoint;
    double m_minZoom = 0.05;
    double m_maxZoom = 20.0;

};

#endif
