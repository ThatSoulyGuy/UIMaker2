#ifndef PANZOOMHANDLER_HPP
#define PANZOOMHANDLER_HPP

#include <QPoint>

#include "input/InputHandler.hpp"

class QGraphicsView;

class PanZoomHandler : public InputHandler
{
    Q_OBJECT

public:

    explicit PanZoomHandler(QObject* parent = nullptr);

    InputResult HandlePress(const MousePressEvent& event, EditorContext& ctx) override;
    InputResult HandleMove(const MouseMoveEvent& event, EditorContext& ctx) override;
    InputResult HandleRelease(const MouseReleaseEvent& event, EditorContext& ctx) override;
    InputResult HandleWheel(const WheelEvent& event, EditorContext& ctx) override;

    bool IsPanning() const noexcept;

private:

    void ZoomAt(QGraphicsView* view, const QPoint& viewPos, double factor);

    bool m_panning = false;
    QPoint m_lastPanPoint;
    double m_minZoom = 0.05;
    double m_maxZoom = 20.0;

};

#endif
