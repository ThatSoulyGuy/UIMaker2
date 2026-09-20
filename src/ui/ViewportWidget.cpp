#include "ui/ViewportWidget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QColor>
#include <QPixmap>
#include <QBrush>
#include <QPaintDevice>

#include <algorithm>
#include <cmath>

#include "tools/ToolManager.hpp"
#include "tools/Tool.hpp"
#include "render/RenderPipeline.hpp"
#include "render/GizmoRenderPass.hpp"
#include "input/EditorContext.hpp"
#include "input/InputHandler.hpp"
#include "input/InputEvents.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/GridSnap.hpp"
#include "core/PixelModel.hpp"

ViewportWidget::ViewportWidget(QWidget* parent)
    : QGraphicsView(parent)
    , m_toolManager(new ToolManager(this))
    , m_renderPipeline(new RenderPipeline(this))
{
    UpdateRenderMode();

    // The gizmo overlay is painted in paintEvent, outside the scene's dirty-region
    // bookkeeping, so under the default MinimalViewportUpdate any scene-driven
    // repaint (a property edit, an undo) reblits only the changed item rect and
    // leaves gizmo pixels smeared across the canvas. Affordable now that the dot
    // grid is a single blit rather than thousands of ellipses.
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

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
    // Cancel any armed calibration pick - its target document is going away.
    if (m_pickMode)
    {
        m_pickMode = false;
        unsetCursor();
    }

    m_document = document;

    // Detach from the outgoing scene when clearing. Callers null this out before
    // `delete document` precisely so no paint can run against a freed scene; not
    // calling setScene(nullptr) here left the view holding the dead one.
    setScene(m_document ? m_document->GetScene() : nullptr);
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

void ViewportWidget::UpdateRenderMode()
{
    QPainter::RenderHints hints = QPainter::Antialiasing | QPainter::TextAntialiasing;

    // Continuous mode smooths scaled pixmaps; PixelGrid keeps them crisp so the
    // editor previews pixel-art the way the target engine will render it.
    if (PixelModel::GetMode() == PixelModel::Mode::Continuous)
        hints |= QPainter::SmoothPixmapTransform;

    setRenderHints(hints);
    viewport()->update();
}

void ViewportWidget::FitToScene()
{
    if (!scene())
        return;

    // Frame the design canvas, not the (much larger) pasteboard scene rect.
    const QRectF target = m_document ? m_document->GetCanvasRect() : scene()->sceneRect();
    fitInView(target, Qt::KeepAspectRatio);
}

bool ViewportWidget::EnsureGridTile(double zoom, double dpr)
{
    constexpr double kStep   = 16.0;   // grid spacing, scene units
    constexpr double kRadius = 1.2;    // dot radius, scene units
    constexpr double kMinCellPx = 3.0; // below this the dots are just a wash

    const double cellPx = kStep * zoom;   // logical px between dots

    if (cellPx < kMinCellPx)
        return false;

    if (!m_gridTile.isNull()
        && qFuzzyCompare(m_gridTileZoom, zoom)
        && qFuzzyCompare(m_gridTileDpr, dpr))
    {
        return true;
    }

    // Pack several cells into one tile so the whole-pixel rounding of the tile
    // edge is amortised: the residual spacing error is under half a pixel per
    // tile rather than per cell. Cap the tile so a deep zoom-in cannot allocate
    // an enormous pixmap.
    const int cells = std::max(1, std::min(8, int(std::ceil(256.0 / cellPx))));
    const int tilePx = std::max(1, int(std::lround(cellPx * cells)));

    const int physical = std::max(1, int(std::lround(tilePx * dpr)));

    QPixmap tile(physical, physical);
    tile.setDevicePixelRatio(dpr);
    tile.fill(Qt::transparent);

    {
        // The pixmap carries a device pixel ratio, so this painter works in the
        // same logical units as the viewport.
        QPainter p(&tile);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(110, 110, 115));

        const double cell = double(tilePx) / cells;
        const double r = std::max(0.35, kRadius * zoom);

        for (int i = 0; i < cells; ++i)
        {
            for (int j = 0; j < cells; ++j)
                p.drawEllipse(QPointF((i + 0.5) * cell, (j + 0.5) * cell), r, r);
        }
    }

    m_gridTile     = tile;
    m_gridTileZoom = zoom;
    m_gridTileDpr  = dpr;
    m_gridTileSize = tilePx;
    m_gridCell     = double(tilePx) / cells;

    return true;
}

void ViewportWidget::drawBackground(QPainter* painter, const QRectF& rect)
{
    // Solid fill from the scene's background brush.
    QGraphicsView::drawBackground(painter, rect);

    // World-space dot grid. The dots still sit at fixed scene coordinates and
    // still scale with zoom, but they are blitted from a cached tile rather than
    // stroked one ellipse at a time, so the cost is flat in the exposed area and
    // in the zoom level. The tile is rendered at device resolution, so the dots
    // stay as crisp as the vector version.
    {
        const QTransform world = painter->worldTransform();
        const double zoom = world.m11();
        const double dpr  = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;

        if (zoom > 0.0 && EnsureGridTile(zoom, dpr))
        {
            // Anchor the tiling so that a dot centre lands exactly on scene
            // (0,0) and therefore on every scene multiple of the grid step. Tile
            // dots sit at half-cell offsets, hence the -0.5 cell.
            const QPointF origin = world.map(QPointF(0.0, 0.0));

            auto phase = [this](double v)
            {
                const double t = double(m_gridTileSize);
                double r = std::fmod(v - 0.5 * m_gridCell, t);
                if (r < 0.0)
                    r += t;
                return r;
            };

            QBrush brush(m_gridTile);
            brush.setTransform(QTransform::fromTranslate(phase(origin.x()), phase(origin.y())));

            painter->save();
            painter->setWorldTransform(QTransform());   // blit in device space
            painter->setPen(Qt::NoPen);
            painter->fillRect(world.mapRect(rect), brush);
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

void ViewportWidget::BeginElementPick()
{
    m_pickMode = true;
    setCursor(Qt::CrossCursor);
}

void ViewportWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_pickMode)
    {
        m_pickMode = false;
        unsetCursor();

        UiElement* picked = nullptr;
        if (event->button() == Qt::LeftButton)
        {
            if (auto* sei = dynamic_cast<SceneElementItem*>(itemAt(event->pos())))
                picked = sei->GetElement();
        }

        emit ElementPicked(picked);
        return;
    }

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
    if (m_pickMode)
        return;   // keep the crosshair; wait for the pick click

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
    if (m_pickMode)
        return;

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
    if (m_pickMode)
    {
        // Escape cancels a calibration pick; swallow other keys so shortcuts
        // don't fire while armed.
        if (event->key() == Qt::Key_Escape)
        {
            m_pickMode = false;
            unsetCursor();
            emit ElementPicked(nullptr);
        }
        return;
    }

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
