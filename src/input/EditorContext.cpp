#include "input/EditorContext.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneElementItem.hpp"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QRectF>
#include <QPointF>
#include <QPoint>

QList<SceneElementItem*> EditorContext::GetSelectedItems() const
{
    QList<SceneElementItem*> result;

    if (!document)
        return result;

    auto* scene = document->GetScene();

    if (!scene)
        return result;

    for (auto* item : scene->selectedItems())
    {
        if (auto* se = dynamic_cast<SceneElementItem*>(item))
            result.append(se);
    }

    return result;
}

UiElement* EditorContext::GetSelectedElement() const
{
    auto items = GetSelectedItems();

    if (items.isEmpty())
        return nullptr;

    return items.first()->GetElement();
}

QPointF EditorContext::MapToScene(const QPoint& viewPos) const
{
    if (view)
        return view->mapToScene(viewPos);

    return QPointF(viewPos);
}

QPoint EditorContext::MapFromScene(const QPointF& scenePos) const
{
    if (view)
        return view->mapFromScene(scenePos);

    return scenePos.toPoint();
}

double EditorContext::GetZoom() const
{
    if (view)
        return view->transform().m11();

    return 1.0;
}

QRectF EditorContext::GetVisibleSceneRect() const
{
    if (view)
        return view->mapToScene(view->viewport()->rect()).boundingRect();

    return QRectF();
}
