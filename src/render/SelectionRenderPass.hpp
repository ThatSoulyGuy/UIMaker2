#ifndef SELECTIONRENDERPASS_HPP
#define SELECTIONRENDERPASS_HPP

#include "render/RenderPass.hpp"
#include "scene/SceneDocument.hpp"

class SelectionRenderPass : public RenderPass
{
    Q_OBJECT

public:

    explicit SelectionRenderPass(QObject* parent = nullptr) : RenderPass(parent) { }

    QString GetId() const override
    {
        return QStringLiteral("selection");
    }

    RenderLayer GetLayer() const override
    {
        return RenderLayer::Selection;
    }

    void Render(QPainter& painter, const EditorContext& ctx) override
    {
        if (!ctx.document || !ctx.view)
            return;

        auto* scene = ctx.document->GetScene();

        if (!scene)
            return;

        auto selected = scene->selectedItems();

        if (selected.isEmpty())
            return;

        painter.setRenderHint(QPainter::Antialiasing, true);

        for (auto* item : selected)
        {
            QRectF sceneBounds = item->sceneBoundingRect();
            QPolygonF viewPoly = ctx.view->mapFromScene(sceneBounds);
            QRectF viewRect = viewPoly.boundingRect();

            painter.setPen(QPen(QColor(0, 180, 255), 1, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(viewRect.adjusted(-1, -1, 1, 1));
        }
    }

};

#endif
