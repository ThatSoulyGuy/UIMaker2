#include "ui/ViewportWidget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QNativeGestureEvent>
#include <QEvent>
#include <QScrollBar>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QColor>
#include <QPixmap>
#include <QBrush>
#include <QVarLengthArray>
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
#include "input/PanZoomHandler.hpp"
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

bool ViewportWidget::EnsureSnapTile(double cellW, double cellH, double zoom, double dpr)
{
    const double wPx = cellW * zoom;   // logical px per cell
    const double hPx = cellH * zoom;

    if (wPx <= 0.0 || hPx <= 0.0)
        return false;

    if (!m_snapTile.isNull()
        && qFuzzyCompare(m_snapZoom, zoom) && qFuzzyCompare(m_snapDpr, dpr)
        && qFuzzyCompare(m_snapCellSrcW, cellW) && qFuzzyCompare(m_snapCellSrcH, cellH))
    {
        return true;
    }

    // Pack several cells per tile so the whole-pixel rounding of the tile edge
    // is amortised across them rather than accumulating once per cell.
    const int nx = std::max(1, std::min(64, int(std::ceil(192.0 / wPx))));
    const int ny = std::max(1, std::min(64, int(std::ceil(192.0 / hPx))));

    const int tileW = std::max(1, int(std::lround(wPx * nx)));
    const int tileH = std::max(1, int(std::lround(hPx * ny)));

    QPixmap tile(std::max(1, int(std::lround(tileW * dpr))),
                 std::max(1, int(std::lround(tileH * dpr))));
    tile.setDevicePixelRatio(dpr);
    tile.fill(Qt::transparent);

    {
        QPainter p(&tile);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(QPen(QColor(120, 140, 170, 120)));

        const double cw = double(tileW) / nx;
        const double ch = double(tileH) / ny;

        for (int i = 0; i < nx; ++i)
        {
            const double x = std::floor(i * cw) + 0.0;
            p.drawLine(QPointF(x, 0.0), QPointF(x, double(tileH)));
        }

        for (int j = 0; j < ny; ++j)
        {
            const double y = std::floor(j * ch) + 0.0;
            p.drawLine(QPointF(0.0, y), QPointF(double(tileW), y));
        }
    }

    m_snapTile  = tile;
    m_snapZoom  = zoom;
    m_snapDpr   = dpr;
    m_snapCellW = double(tileW) / nx;
    m_snapCellH = double(tileH) / ny;
    m_snapTileW = tileW;
    m_snapTileH = tileH;
    m_snapCellSrcW = cellW;
    m_snapCellSrcH = cellH;

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

    // Snapping grid overlay. The cell comes from GridSnap::CellSize, which in
    // PixelGrid mode is the square virtual pixel and in Continuous mode is the
    // canvas divided by the X/Y counts - so the overlay always shows exactly
    // what a drag will snap to.
    if (GridSnap::Enabled() && m_document)
    {
        const QRectF canvas = m_document->GetCanvasRect();
        const QSizeF cell = GridSnap::CellSize(canvas);
        const double zoom = transform().m11();

        if (cell.width() > 0.0 && cell.height() > 0.0
            && canvas.width() > 0.0 && canvas.height() > 0.0 && zoom > 0.0)
        {
            const double cellW = cell.width();
            const double cellH = cell.height();

            // A grid finer than a few pixels on screen is a grey wash, not a
            // reference - and drawing it costs one primitive per line. So step
            // up by powers of two until the drawn spacing is legible, exactly
            // as a DCC app shows major gridlines when you zoom out. Every line
            // drawn is still a REAL snap position, just not every snap position
            // is drawn.
            // Cost is dominated by total line LENGTH, so the lever is line count:
            // measured ~48 us per full-height line in the software rasteriser.
            // 16px keeps the grid a usable spatial reference while halving the
            // worst case; once you zoom in far enough to work pixel-by-pixel,
            // the step falls to 1 and you see the true grid - and at that zoom
            // few lines are on screen anyway.
            constexpr double kMinSpacingPx = 16.0;

            int stepX = 1;
            while (cellW * zoom * stepX < kMinSpacingPx && stepX < (1 << 16))
                stepX *= 2;

            int stepY = 1;
            while (cellH * zoom * stepY < kMinSpacingPx && stepY < (1 << 16))
                stepY *= 2;

            const double spanX = cellW * stepX;
            const double spanY = cellH * stepY;

            // Still give up when even a stepped grid would cover the canvas in
            // lines, which happens only at absurd zoom-out.
            if (spanX * zoom >= 2.0 && spanY * zoom >= 2.0)
            {
                // Only the lines that actually intersect the exposed area, and
                // only across the exposed span. Drawing all (dx+1)+(dy+1) lines
                // at full canvas length cost 21 ms per repaint at 320x240 and
                // zoom 1 - every pan, every drag frame - because the count is
                // fixed by the division count and each line spanned the whole
                // canvas no matter how little of it was on screen.
                const QRectF area = rect.intersected(canvas);

                // EXACT positions, not a tile.
                //
                // The previous version blitted a cached tile, which meant the
                // drawn period was lround(cell * n) DEVICE PIXELS while the
                // real snap period is fractional. The grid therefore drifted
                // from the positions a drag actually snaps to, and every zoom
                // step re-rounded the tile and made the whole grid jump - the
                // reported jitter. A pixel grid that does not sit where the
                // pixels are is worse than a slow one.
                //
                // Drawn in device space with a plain 1px pen and batched into
                // one drawLines call. QGraphicsView has already set a clip by
                // the time drawBackground runs, which is the fast raster path:
                // measured ~1 ms for ~700 lines versus ~34 ms unclipped.
                const QTransform world = painter->worldTransform();

                // Division counts follow from the cell: in PixelGrid mode the
                // cell is the virtual pixel and the counts are derived, so they
                // cannot be read from GridSnap.
                const int divX = int(std::ceil(canvas.width() / spanX));
                const int divY = int(std::ceil(canvas.height() / spanY));

                const int i0 = std::max(0, int(std::floor((area.left() - canvas.left()) / spanX)));
                const int i1 = std::min(divX, int(std::ceil((area.right() - canvas.left()) / spanX)));
                const int j0 = std::max(0, int(std::floor((area.top() - canvas.top()) / spanY)));
                const int j1 = std::min(divY, int(std::ceil((area.bottom() - canvas.top()) / spanY)));

                const QPointF a0 = world.map(area.topLeft());
                const QPointF a1 = world.map(area.bottomRight());

                QVarLengthArray<QLineF, 2048> lines;

                for (int i = i0; i <= i1; ++i)
                {
                    const double x = world.map(QPointF(canvas.left() + i * spanX, 0.0)).x();
                    lines.append(QLineF(x, a0.y(), x, a1.y()));
                }

                for (int j = j0; j <= j1; ++j)
                {
                    const double y = world.map(QPointF(0.0, canvas.top() + j * spanY)).y();
                    lines.append(QLineF(a0.x(), y, a1.x(), y));
                }

                if (!lines.isEmpty())
                {
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing, false);
                    painter->setWorldTransform(QTransform());
                    painter->setPen(QPen(QColor(120, 140, 170, 120)));
                    painter->drawLines(lines.constData(), lines.size());
                    painter->restore();
                }
            }
        }
    }
}

bool ViewportWidget::event(QEvent* e)
{
    if (e->type() == QEvent::NativeGesture)
    {
        auto* g = static_cast<QNativeGestureEvent*>(e);

        if (g->gestureType() == Qt::ZoomNativeGesture && m_toolManager)
        {
            EditorContext ctx;
            ctx.document = m_document;
            ctx.view = this;

            if (auto* pz = m_toolManager->GetPanZoomHandler())
            {
                const InputResult r = pz->HandlePinch(g->position().toPoint(), g->value(), ctx);

                if (r.consumed)
                {
                    if (r.needsRepaint)
                        viewport()->update();

                    e->accept();
                    return true;
                }
            }
        }
    }

    return QGraphicsView::event(e);
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
    // Qt delivers the SECOND of two quick clicks here instead of as a press,
    // so anything special-cased on this event silently steals every rapid
    // repeat click. That is exactly the gesture click-through selection is
    // built on, so route it through the ordinary press path and let the
    // handler treat it as what the user did: another click.
    //
    // This used to frame the item under the cursor. F still does that (and
    // frames the whole selection, not just one item), so nothing is lost.
    mousePressEvent(event);
}

void ViewportWidget::wheelEvent(QWheelEvent* event)
{
    WheelEvent e;
    e.viewPos = event->position().toPoint();
    e.scenePos = mapToScene(e.viewPos);
    e.angleDelta = event->angleDelta();
    e.pixelDelta = event->pixelDelta();

    // A trackpad reports pixelDelta and synthesises the event; a wheel mouse
    // reports only angleDelta in 120-unit detents. Treat them differently:
    // swiping should scroll the canvas, a wheel click should zoom.
    e.fromTrackpad = !event->pixelDelta().isNull()
                  || event->source() == Qt::MouseEventSynthesizedBySystem;

    e.delta = e.angleDelta.y() != 0 ? e.angleDelta.y() : e.angleDelta.x();
    e.orientation = e.angleDelta.y() != 0 ? Qt::Vertical : Qt::Horizontal;
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
