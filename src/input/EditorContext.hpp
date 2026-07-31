#ifndef EDITORCONTEXT_HPP
#define EDITORCONTEXT_HPP

#include <QList>
#include <QPointF>
#include <QPoint>
#include <QRectF>

class SceneDocument;
class SceneElementItem;
class UiElement;
class QGraphicsView;

class EditorContext
{

public:

    SceneDocument* document = nullptr;
    QGraphicsView* view = nullptr;

    QList<SceneElementItem*> GetSelectedItems() const;

    UiElement* GetSelectedElement() const;

    QPointF MapToScene(const QPoint& viewPos) const;

    QPoint MapFromScene(const QPointF& scenePos) const;

    double GetZoom() const;

    QRectF GetVisibleSceneRect() const;

};

#endif
