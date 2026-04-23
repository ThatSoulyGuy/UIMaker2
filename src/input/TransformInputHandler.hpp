#ifndef TRANSFORMINPUTHANDLER_HPP
#define TRANSFORMINPUTHANDLER_HPP

#include <QGraphicsItem>

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

private:

    SceneElementItem* GetSelectedItem(EditorContext& ctx) const;
    TransformComponent* GetTransformComponent(SceneElementItem* item) const;
    QByteArray CaptureState(EditorContext& ctx) const;

    void ApplyTransform(SceneElementItem* item, TransformComponent* xform, const QPointF& scenePos, const QPointF& sceneDelta);
    void ApplyScale(SceneElementItem* item, TransformComponent* xform, const QPointF& scenePos, const QPointF& sceneDelta);

    GizmoManager* m_gizmoManager = nullptr;

    bool m_transforming = false;
    QString m_activeHandleId;

    QPoint m_startViewPos;
    QPointF m_startScenePos;
    QPointF m_itemCenter;
    QPointF m_startItemPos;
    double m_startRotation = 0.0;
    QPointF m_startScale;
    QRectF m_startRect;
    QByteArray m_startJson;

};

#endif
