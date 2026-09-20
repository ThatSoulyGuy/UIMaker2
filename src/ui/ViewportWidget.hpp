#ifndef VIEWPORTWIDGET_HPP
#define VIEWPORTWIDGET_HPP

#include <QGraphicsView>

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

    // Paints the dot grid as constant-size vector dots (crisp at every zoom)
    // instead of a scaled pixmap brush.
    void drawBackground(QPainter* painter, const QRectF& rect) override;

    void paintEvent(QPaintEvent* event) override;

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

};

#endif
