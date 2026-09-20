#ifndef TRANSFORMINPUTHANDLER_HPP
#define TRANSFORMINPUTHANDLER_HPP

#include <QGraphicsItem>
#include <QList>

#include "input/InputHandler.hpp"

class GizmoManager;
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
        QRectF startSceneRect;
        double startRotation = 0.0;
        QPointF startScale;
    };

    SceneElementItem* GetSelectedItem(EditorContext& ctx) const;
    TransformComponent* GetTransformComponent(SceneElementItem* item) const;

    // Arm a drag: snapshot every item's start pose and take ownership of the
    // gesture. Shared by the gizmo-handle path and the body-drag path, which
    // differ only in how the handle id is chosen.
    void BeginDrag(const QList<SceneElementItem*>& items, const QString& handleId,
                   const QPointF& scenePos, const QRectF& sceneBounds);

    // The topmost selectable element under this scene position, or null when
    // the press landed on empty canvas (where QGraphicsView still does
    // rubber-band selection).
    static SceneElementItem* TopmostBodyAt(const QPointF& scenePos, EditorContext& ctx);

    // Whether this item's position is owned by a layout parent. Dragging such
    // a child wrote a position the layout overwrote a frame later.
    static bool HasLayoutParent(SceneElementItem* item);

    void ApplyTransform(const QPointF& scenePos, const QPointF& sceneDelta, EditorContext& ctx);
    void ApplyScale(const ItemStartState& state, const QPointF& scenePos, const QPointF& sceneDelta, EditorContext& ctx);

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
