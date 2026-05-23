#ifndef TRANSFORMINPUTHANDLER_HPP
#define TRANSFORMINPUTHANDLER_HPP

#include <QGraphicsItem>
#include <QList>

#include "input/InputHandler.hpp"
#include "gizmos/GizmoManager.hpp"

class SceneDocument;
class SceneElementItem;
class TransformComponent;

class TransformInputHandler : public InputHandler
{
    Q_OBJECT

public:

    explicit TransformInputHandler(GizmoManager* gizmoManager, QObject* parent = nullptr);

    InputResult HandlePress(const MousePressEvent& event, EditorContext& ctx) override;
    InputResult HandleMove(const MouseMoveEvent& event, EditorContext& ctx) override;
    InputResult HandleRelease(const MouseReleaseEvent& event, EditorContext& ctx) override;

    bool IsTransforming() const noexcept override;
    QString GetActiveHandleId() const override;
    QString GetUndoActionName() const override;

    static QList<SceneElementItem*> GetTopLevelSelectedItems(EditorContext& ctx);
    static QRectF ComputeUnionSceneBounds(const QList<SceneElementItem*>& items);

private:

    struct ItemStartState
    {
        SceneElementItem* item = nullptr;
        TransformComponent* xform = nullptr;
        QPointF startItemPos;
        QPointF startPosition;
        QPointF startSceneCenter;
        double startRotation = 0.0;
        QPointF startScale;
    };

    SceneElementItem* GetSelectedItem(EditorContext& ctx) const;
    TransformComponent* GetTransformComponent(SceneElementItem* item) const;

    void ApplyTransform(const QPointF& scenePos, const QPointF& sceneDelta);
    void ApplyScale(const ItemStartState& state, const QPointF& scenePos, const QPointF& sceneDelta);

    GizmoManager* m_gizmoManager = nullptr;

    bool m_transforming = false;
    QString m_activeHandleId;

    QPoint m_startViewPos;
    QPointF m_startScenePos;
    QPointF m_itemCenter;
    QRectF m_startRect;

    QList<ItemStartState> m_startStates;

};

#endif
