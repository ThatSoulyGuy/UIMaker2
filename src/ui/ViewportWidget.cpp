#include "ui/ViewportWidget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QColor>
#include <cmath>

#include "tools/ToolManager.hpp"
#include "tools/Tool.hpp"
#include "render/RenderPipeline.hpp"
#include "render/GizmoRenderPass.hpp"
#include "input/EditorContext.hpp"
#include "input/InputHandler.hpp"
#include "input/InputEvents.hpp"
#include "scene/SceneDocument.hpp"
#include "core/GridSnap.hpp"

ViewportWidget::ViewportWidget(QWidget* parent)
    : QGraphicsView(parent)
    , m_toolManager(new ToolManager(this))
    , m_renderPipeline(new RenderPipeline(this))
{
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setDragMode(QGraphicsView::RubberBandDrag);
    setMouseTracking(true);

    // Setup render pipeline
    auto* gizmoPass = new GizmoRenderPass(this);
    gizmoPass->SetToolManager(m_toolManager);
    m_renderPipeline->AddPass(gizmoPass);

    // Set default tool
    m_toolManager->SetActiveTool("translate");

    // Connect transform signals
    connect(m_toolManager, &ToolManager::TransformEnded, this, &ViewportWidget::onTransformEnded);
}

void ViewportWidget::SetDocument(SceneDocument* document)
{
    m_document = document;

    if (m_document)
        setScene(m_document->GetScene());
}

SceneDocument* ViewportWidget::GetDocument() const noexcept
{
    return m_document;
}

ToolManager* ViewportWidget::GetToolManager() const noexcept
{
    return m_toolManager;
}

RenderPipeline* ViewportWidget::GetRenderPipeline() const noexcept
{
    return m_renderPipeline;
}

void ViewportWidget::FitToItem(QGraphicsItem* item)
{
    if (!item)
        return;

    const QRectF r = item->sceneBoundingRect().adjusted(-20.0, -20.0, 20.0, 20.0);
    fitInView(r, Qt::KeepAspectRatio);
}

void ViewportWidget::FitToScene()
{
    if (!scene())
        return;

    // Frame the design canvas, not the (much larger) pasteboard scene rect.
    const QRectF target = m_document ? m_document->GetCanvasRect() : scene()->sceneRect();
    fitInView(target, Qt::KeepAspectRatio);
}

void ViewportWidget::drawBackground(QPainter* painter, const QRectF& rect)
{
    // Solid fill from the scene's background brush.
    QGraphicsView::drawBackground(painter, rect);

    // World-space dot grid: the dots sit at fixed scene coordinates and are
    // drawn in scene coordinates, so the whole grid scales with zoom - bigger
    // as you zoom in, smaller as you zoom out - exactly like the canvas
    // content. Drawing them as vector circles instead of a scaled pixmap is
    // what keeps them crisp at every zoom level.
    {
        const double step = 16.0;     // grid spacing, scene units
        const double radius = 1.2;    // dot radius, scene units

        const double left = std::floor(rect.left() / step) * step;
        const double top = std::floor(rect.top() / step) * step;

        // At extreme zoom-out the grid collapses to a sub-pixel haze; skip
        // drawing there so the dot count (and paint cost) stays bounded.
        const double cols = (rect.right() - left) / step + 1.0;
        const double rows = (rect.bottom() - top) / step + 1.0;

        if (cols * rows <= 50000.0)
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(110, 110, 115));

            for (double x = left; x <= rect.right(); x += step)
            {
                for (double y = top; y <= rect.bottom(); y += step)
                    painter->drawEllipse(QPointF(x, y), radius, radius);
            }

            painter->restore();
        }
    }

    // Snapping grid overlay: the design canvas divided into DivisionsX by
    // DivisionsY cells. Drawn in scene coordinates (so it scales/pans with the
    // canvas) with a cosmetic pen (so the lines stay a crisp 1px at any zoom).
    if (GridSnap::Enabled() && m_document)
    {
        const QRectF canvas = m_document->GetCanvasRect();
        const int dx = GridSnap::DivisionsX();
        const int dy = GridSnap::DivisionsY();
        const double zoom = transform().m11();

        if (dx > 0 && dy > 0 && canvas.width() > 0.0 && canvas.height() > 0.0 && zoom > 0.0)
        {
            const double cellW = canvas.width() / dx;
            const double cellH = canvas.height() / dy;

            // Skip only when cells collapse to a sub-pixel wash. The threshold
            // is low enough that a fine grid (e.g. Minecraft's 320x240, whose
            // cells are ~4.5px tall on the canvas) still shows at the default
            // fit-to-canvas zoom, so the overlay agrees with the active snap.
            if (cellW * zoom >= 2.0 && cellH * zoom >= 2.0)
            {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing, false);

                QPen pen(QColor(120, 140, 170, 120));
                pen.setCosmetic(true);
                painter->setPen(pen);

                for (int i = 0; i <= dx; ++i)
                {
                    const double x = canvas.left() + i * cellW;
                    painter->drawLine(QPointF(x, canvas.top()), QPointF(x, canvas.bottom()));
                }

                for (int j = 0; j <= dy; ++j)
                {
                    const double y = canvas.top() + j * cellH;
                    painter->drawLine(QPointF(canvas.left(), y), QPointF(canvas.right(), y));
                }

                painter->restore();
            }
        }
    }
}

void ViewportWidget::paintEvent(QPaintEvent* event)
{
    // Standard QGraphicsView painting
    QGraphicsView::paintEvent(event);

    // Overlay rendering (gizmos, selection, etc.)
    if (m_document)
    {
        EditorContext ctx;
        ctx.document = m_document;
        ctx.view = this;

        QPainter painter(viewport());
        m_renderPipeline->Render(painter, ctx);
    }
}

void ViewportWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_document)
    {
        MousePressEvent e;
        e.viewPos = event->pos();
        e.scenePos = mapToScene(event->pos());
        e.button = event->button();
        e.buttons = event->buttons();
        e.modifiers = event->modifiers();

        EditorContext ctx;
        ctx.document = m_document;
        ctx.view = this;

        InputResult result = m_toolManager->HandleInput(e, ctx);

        if (result.consumed)
        {
            setCursor(result.cursor);

            if (result.needsRepaint)
                viewport()->update();

            return;
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_document)
    {
        MouseMoveEvent e;
        e.viewPos = event->pos();
        e.scenePos = mapToScene(event->pos());
        e.buttons = event->buttons();
        e.modifiers = event->modifiers();

        EditorContext ctx;
        ctx.document = m_document;
        ctx.view = this;

        InputResult result = m_toolManager->HandleInput(e, ctx);

        if (result.consumed || m_toolManager->IsTransforming() || m_toolManager->IsPanning())
        {
            setCursor(result.cursor);

            if (result.needsRepaint)
                viewport()->update();

            if (result.consumed)
                return;
        }
        else
        {
            setCursor(result.cursor);
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_document)
    {
        MouseReleaseEvent e;
        e.viewPos = event->pos();
        e.scenePos = mapToScene(event->pos());
        e.button = event->button();
        e.buttons = event->buttons();
        e.modifiers = event->modifiers();

        EditorContext ctx;
        ctx.document = m_document;
        ctx.view = this;

        InputResult result = m_toolManager->HandleInput(e, ctx);

        if (result.consumed)
        {
            setCursor(result.cursor);

            // Restore drag mode after panning
            if (!m_toolManager->IsPanning())
            {
                Tool* tool = m_toolManager->GetActiveTool();

                if (tool && tool->GetId() == "translate")
                    setDragMode(QGraphicsView::RubberBandDrag);
                else
                    setDragMode(QGraphicsView::NoDrag);
            }

            if (result.needsRepaint)
                viewport()->update();

            return;
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void ViewportWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && scene())
    {
        const QPointF scenePos = mapToScene(event->pos());
        QGraphicsItem* item = nullptr;
        const QList<QGraphicsItem*> hit = scene()->items(scenePos);

        for (QGraphicsItem* it : hit)
        {
            if (it->flags().testFlag(QGraphicsItem::ItemIsSelectable))
            {
                item = it;
                break;
            }
        }

        if (item)
        {
            FitToItem(item);
            return;
        }
    }

    QGraphicsView::mouseDoubleClickEvent(event);
}

void ViewportWidget::wheelEvent(QWheelEvent* event)
{
    WheelEvent e;
    e.viewPos = event->position().toPoint();
    e.scenePos = mapToScene(e.viewPos);
    e.delta = event->angleDelta().y();
    e.orientation = event->angleDelta().y() != 0 ? Qt::Vertical : Qt::Horizontal;
    e.modifiers = event->modifiers();

    EditorContext ctx;
    ctx.document = m_document;
    ctx.view = this;

    InputResult result = m_toolManager->HandleInput(e, ctx);

    if (result.consumed)
    {
        event->accept();
        return;
    }

    QGraphicsView::wheelEvent(event);
}

void ViewportWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_F && scene())
    {
        auto selected = scene()->selectedItems();

        if (!selected.isEmpty())
        {
            FitToItem(selected.first());
            return;
        }
    }

    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_0)
    {
        FitToScene();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void ViewportWidget::onTransformEnded(const QList<TransformDelta>& deltas, const QString& actionName)
{
    emit TransformCompleted(deltas, actionName);
}
