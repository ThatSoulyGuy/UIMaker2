#ifndef TRANSFORMINPUTHANDLER_HPP
#define TRANSFORMINPUTHANDLER_HPP

#include <QGraphicsItem>
#include <QList>
#include <QPoint>
#include <QUuid>
#include <QElapsedTimer>

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

    // Every selectable element under this scene position, topmost first.
    static QList<SceneElementItem*> BodiesAt(const QPointF& scenePos, EditorContext& ctx);

    // The topmost selectable element under this scene position, or null when
    // the press landed on empty canvas (where QGraphicsView still does
    // rubber-band selection).
    static SceneElementItem* TopmostBodyAt(const QPointF& scenePos, EditorContext& ctx);

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

    // Click-through: a click that lands on the same overlapping stack as the
    // previous one, soon enough and without a drag in between, selects the
    // element one layer DOWN. Repeats to the bottom, then wraps.
    //
    // Resolved on RELEASE rather than on press: at press time we cannot know
    // whether a drag is about to start, and descending mid-gesture would drag
    // the wrong element.
    void CycleSelectionAt(const QPoint& viewPos, const QPointF& scenePos, EditorContext& ctx);

    // Forget where we were in the stack. Anything that is not "click again in
    // the same place" ends the descent: a drag, a click elsewhere, a click on
    // empty canvas, or simply waiting too long.
    void ResetCycle();

    // How long after a click a second one still counts as "again". Derived
    // from the system double-click interval so it tracks the user's own
    // setting for how fast repeated clicking is.
    static int CycleWindowMs();

    // True when this press continues the descent started by the last click.
    bool CycleIsLiveAt(const QPoint& viewPos, const QList<SceneElementItem*>& stack) const;

    // The element a press should act on: the one the cycle has descended to
    // when it is still live, otherwise the topmost. Without this the press
    // would re-select the top of the stack and undo the previous descent.
    SceneElementItem* PickBodyForPress(const QPoint& viewPos, const QPointF& scenePos, EditorContext& ctx);

    static QList<QUuid> StackIds(const QList<SceneElementItem*>& stack);

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

    // --- click-through cycling state ---------------------------------------
    QPoint m_pressViewPos;          // where the current gesture began
    bool   m_pressWasOnBody = false;

    QPoint m_cycleViewPos;          // where the last completed click landed
    QElapsedTimer m_cycleTimer;
    int    m_cycleDepth = 0;
    QList<QUuid> m_cycleStack;      // ids of the stack that depth indexes into

    // A click is a press and release at nearly the same point; past this many
    // view pixels the gesture is a drag and must not descend.
    static constexpr int kClickSlopPx = 3;

};

#endif
