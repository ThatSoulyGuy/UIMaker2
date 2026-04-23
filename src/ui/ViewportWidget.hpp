#ifndef VIEWPORTWIDGET_HPP
#define VIEWPORTWIDGET_HPP

#include <QGraphicsView>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include "tools/ToolManager.hpp"
#include "render/RenderPipeline.hpp"
#include "render/GizmoRenderPass.hpp"
#include "input/EditorContext.hpp"
#include "scene/SceneDocument.hpp"

class ViewportWidget : public QGraphicsView
{
    Q_OBJECT

public:

    explicit ViewportWidget(QWidget* parent = nullptr)
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

    void SetDocument(SceneDocument* document)
    {
        m_document = document;

        if (m_document)
            setScene(m_document->GetScene());
    }

    SceneDocument* GetDocument() const noexcept
    {
        return m_document;
    }

    ToolManager* GetToolManager() const noexcept
    {
        return m_toolManager;
    }

    RenderPipeline* GetRenderPipeline() const noexcept
    {
        return m_renderPipeline;
    }

    void FitToItem(QGraphicsItem* item)
    {
        if (!item)
            return;

        const QRectF r = item->sceneBoundingRect().adjusted(-20.0, -20.0, 20.0, 20.0);
        fitInView(r, Qt::KeepAspectRatio);
    }

    void FitToScene()
    {
        if (scene())
            fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }

signals:

    void TransformCompleted(const QByteArray& beforeState, const QByteArray& afterState, const QString& actionName);

protected:

    void paintEvent(QPaintEvent* event) override
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

    void mousePressEvent(QMouseEvent* event) override
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

        QGraphicsView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
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

        QGraphicsView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
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

        QGraphicsView::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
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

    void wheelEvent(QWheelEvent* event) override
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

    void keyPressEvent(QKeyEvent* event) override
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

private slots:

    void onTransformEnded(const QByteArray& before, const QByteArray& after, const QString& actionName)
    {
        emit TransformCompleted(before, after, actionName);
    }

private:

    SceneDocument* m_document = nullptr;
    ToolManager* m_toolManager = nullptr;
    RenderPipeline* m_renderPipeline = nullptr;

};

#endif
