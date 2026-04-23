#ifndef EDITORCONTEXT_HPP
#define EDITORCONTEXT_HPP

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QList>
#include <QPointF>
#include <QPoint>

class SceneDocument;
class SceneElementItem;
class UiElement;

class EditorContext
{

public:

    SceneDocument* document = nullptr;
    QGraphicsView* view = nullptr;

    QList<SceneElementItem*> GetSelectedItems() const;

    UiElement* GetSelectedElement() const;

    QPointF MapToScene(const QPoint& viewPos) const
    {
        if (view)
            return view->mapToScene(viewPos);

        return QPointF(viewPos);
    }

    QPoint MapFromScene(const QPointF& scenePos) const
    {
        if (view)
            return view->mapFromScene(scenePos);

        return scenePos.toPoint();
    }

    double GetZoom() const
    {
        if (view)
            return view->transform().m11();

        return 1.0;
    }

    QRectF GetVisibleSceneRect() const
    {
        if (view)
            return view->mapToScene(view->viewport()->rect()).boundingRect();

        return QRectF();
    }

};

#endif
