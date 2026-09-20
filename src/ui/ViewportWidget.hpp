#ifndef VIEWPORTWIDGET_HPP
#define VIEWPORTWIDGET_HPP

#include <QGraphicsView>
#include <QPixmap>
#include <QSizeF>

#include "scene/TransformDelta.hpp"

class QGraphicsItem;
class QPainter;
class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QRectF;

class ToolManager;
class RenderPipeline;
class SceneDocument;
class UiElement;

class ViewportWidget : public QGraphicsView
{
    Q_OBJECT

public:

    explicit ViewportWidget(QWidget* parent = nullptr);

    void SetDocument(SceneDocument* document);

    SceneDocument* GetDocument() const noexcept;

    ToolManager* GetToolManager() const noexcept;

    RenderPipeline* GetRenderPipeline() const noexcept;

    void FitToItem(QGraphicsItem* item);

    void FitToScene();

    // Re-apply render hints from the current PixelModel: PixelGrid mode drops
    // smooth pixmap scaling so the preview is crisp/pixelated. Call after the
    // rendering model changes.
    void UpdateRenderMode();

    // Arm a one-shot element pick: the next left-click in the viewport emits
    // ElementPicked with the clicked element (or nullptr for empty space /
    // other buttons) and disarms. Used by the pixel-unit calibration flow.
    void BeginElementPick();

signals:

    void TransformCompleted(const QList<TransformDelta>& deltas, const QString& actionName);

    void ElementPicked(UiElement* element);

protected:

    // Paints the world-space dot grid by tiling a cached, device-resolution
    // pixmap. The dots still scale with zoom and stay crisp, but the cost no
    // longer grows with the exposed area: the old nested drawEllipse loop issued
    // ~9,000 antialiased calls per repaint at the default zoom (~3.8 ms) and
    // ~36,000 two wheel notches out, on every pan and every drag frame.
    void drawBackground(QPainter* painter, const QRectF& rect) override;

    void paintEvent(QPaintEvent* event) override;

    // macOS delivers pinch-to-zoom as QEvent::NativeGesture, which has no
    // dedicated virtual on QWidget.
    bool event(QEvent* e) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void mouseDoubleClickEvent(QMouseEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

private slots:

    void onTransformEnded(const QList<TransformDelta>& deltas, const QString& actionName);

private:

    SceneDocument* m_document = nullptr;
    ToolManager* m_toolManager = nullptr;
    RenderPipeline* m_renderPipeline = nullptr;

    bool m_pickMode = false;

    // Cached dot-grid tile. Rebuilt only when the zoom or the device pixel ratio
    // changes; see EnsureGridTile.
    QPixmap m_gridTile;
    double  m_gridTileZoom = 0.0;
    double  m_gridTileDpr  = 0.0;
    double  m_gridCell     = 0.0;   // logical px between dots, inside the tile
    int     m_gridTileSize = 0;     // logical px, tile edge (a whole number of cells)

    // Builds m_gridTile for this zoom/dpr if it is not already current.
    // Returns false when the grid should not be drawn at all (dots too dense).
    bool EnsureGridTile(double zoom, double dpr);

    // Cached snapping-grid tile. The snap grid is a uniform subdivision of the
    // canvas, so it is periodic and tiles exactly like the dot grid - which
    // matters because drawing its lines individually cost 16-21 ms per repaint
    // at a fine division count.
    QPixmap m_snapTile;
    double  m_snapZoom = 0.0;
    double  m_snapDpr  = 0.0;
    double  m_snapCellW = 0.0;   // logical px, inside the tile
    double  m_snapCellH = 0.0;
    int     m_snapTileW = 0;
    int     m_snapTileH = 0;
    double  m_snapCellSrcW = 0.0;   // cache key: the scene-unit cell it was built for
    double  m_snapCellSrcH = 0.0;
    bool EnsureSnapTile(double cellW, double cellH, double zoom, double dpr);

};

#endif
