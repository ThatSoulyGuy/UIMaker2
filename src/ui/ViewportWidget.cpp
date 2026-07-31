#include "ui/ViewportWidget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QGraphicsScene>

#include "tools/ToolManager.hpp"
#include "tools/Tool.hpp"
#include "render/RenderPipeline.hpp"
#include "render/GizmoRenderPass.hpp"
#include "input/EditorContext.hpp"
#include "input/InputHandler.hpp"
#include "input/InputEvents.hpp"
#include "scene/SceneDocument.hpp"

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
    if (scene())
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
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
