#include "input/TransformInputHandler.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"
#include "components/TransformComponent.hpp"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

TransformInputHandler::TransformInputHandler(GizmoManager* gizmoManager, QObject* parent)
    : InputHandler(parent)
    , m_gizmoManager(gizmoManager)
{
}

InputResult TransformInputHandler::HandlePress(const MousePressEvent& event, EditorContext& ctx)
{
    if (event.button != Qt::LeftButton || !m_gizmoManager || !ctx.view)
        return InputResult::NotConsumed();

    SceneElementItem* selectedItem = GetSelectedItem(ctx);

    if (!selectedItem)
        return InputResult::NotConsumed();

    QRectF sceneBounds = selectedItem->sceneBoundingRect();
    double rotation = 0.0;
    QPointF scale(1.0, 1.0);

    if (auto* xform = GetTransformComponent(selectedItem))
    {
        rotation = xform->GetRotationDegrees();
        scale = xform->GetScale();
    }

    GizmoHitResult hit = m_gizmoManager->HitTest(event.viewPos, sceneBounds, ctx.view, rotation, scale);

    if (!hit.IsHit())
        return InputResult::NotConsumed();

    m_transforming = true;
    m_activeHandleId = hit.handleId;
    m_startViewPos = event.viewPos;
    m_startScenePos = event.scenePos;
    m_itemCenter = sceneBounds.center();
    m_startRect = sceneBounds;

    m_startItemPos = selectedItem->pos();

    if (auto* xform = GetTransformComponent(selectedItem))
    {
        m_startRotation = xform->GetRotationDegrees();
        m_startScale = xform->GetScale();
    }

    // Capture state for undo
    if (ctx.document)
        m_startJson = CaptureState(ctx);

    m_gizmoManager->SetActiveHandle(m_activeHandleId);

    emit TransformStarted();

    return InputResult::Consumed(hit.cursor, true);
}

InputResult TransformInputHandler::HandleMove(const MouseMoveEvent& event, EditorContext& ctx)
{
    if (!m_transforming || !m_gizmoManager)
    {
        // Just update cursor on hover
        SceneElementItem* selectedItem = GetSelectedItem(ctx);

        if (selectedItem && ctx.view)
        {
            QRectF sceneBounds = selectedItem->sceneBoundingRect();
            GizmoHitResult hit = m_gizmoManager->HitTest(event.viewPos, sceneBounds, ctx.view);

            if (hit.IsHit())
            {
                InputResult result;
                result.cursor = hit.cursor;

                return result;
            }
        }

        return InputResult::NotConsumed();
    }

    SceneElementItem* selectedItem = GetSelectedItem(ctx);

    if (!selectedItem)
        return InputResult::NotConsumed();

    auto* xform = GetTransformComponent(selectedItem);

    if (!xform)
        return InputResult::NotConsumed();

    const QPointF sceneDelta = event.scenePos - m_startScenePos;

    ApplyTransform(selectedItem, xform, event.scenePos, sceneDelta);

    emit TransformUpdated();

    Gizmo* gizmo = m_gizmoManager->GetActiveGizmo();
    Qt::CursorShape cursor = gizmo ? gizmo->GetCursor(m_activeHandleId) : Qt::ArrowCursor;

    return InputResult::Consumed(cursor, true);
}

InputResult TransformInputHandler::HandleRelease(const MouseReleaseEvent& event, EditorContext& ctx)
{
    Q_UNUSED(event);

    if (!m_transforming)
        return InputResult::NotConsumed();

    m_transforming = false;

    if (m_gizmoManager)
        m_gizmoManager->ClearActiveHandle();

    // Capture end state
    QByteArray endJson = CaptureState(ctx);

    if (m_startJson != endJson)
    {
        emit TransformEnded(m_startJson, endJson, GetUndoActionName());
    }

    m_activeHandleId.clear();

    return InputResult::Consumed(Qt::ArrowCursor, true);
}

bool TransformInputHandler::IsTransforming() const noexcept
{
    return m_transforming;
}

QString TransformInputHandler::GetActiveHandleId() const
{
    return m_activeHandleId;
}

QString TransformInputHandler::GetUndoActionName() const
{
    if (m_activeHandleId.startsWith("translate"))
        return "Move";
    else if (m_activeHandleId.startsWith("rotate"))
        return "Rotate";
    else if (m_activeHandleId.startsWith("scale"))
        return "Scale";

    return "Transform";
}

SceneElementItem* TransformInputHandler::GetSelectedItem(EditorContext& ctx) const
{
    if (!ctx.document)
        return nullptr;

    auto* scene = ctx.document->GetScene();

    if (!scene)
        return nullptr;

    auto selected = scene->selectedItems();

    if (selected.isEmpty())
        return nullptr;

    return dynamic_cast<SceneElementItem*>(selected.first());
}

TransformComponent* TransformInputHandler::GetTransformComponent(SceneElementItem* item) const
{
    if (!item)
        return nullptr;

    UiElement* element = item->GetElement();

    if (!element)
        return nullptr;

    return element->GetComponent<TransformComponent>();
}

QByteArray TransformInputHandler::CaptureState(EditorContext& ctx) const
{
    if (!ctx.document)
        return QByteArray();

    return ctx.document->ExportJson();
}

void TransformInputHandler::ApplyTransform(SceneElementItem* item, TransformComponent* xform, const QPointF& scenePos, const QPointF& sceneDelta)
{
    // Translate
    if (m_activeHandleId == "translate_x")
    {
        item->setPos(m_startItemPos + QPointF(sceneDelta.x(), 0));
    }
    else if (m_activeHandleId == "translate_y")
    {
        item->setPos(m_startItemPos + QPointF(0, sceneDelta.y()));
    }
    else if (m_activeHandleId == "translate_xy")
    {
        item->setPos(m_startItemPos + sceneDelta);
    }
    // Rotate
    else if (m_activeHandleId == "rotate_ring")
    {
        const QPointF startVec = m_startScenePos - m_itemCenter;
        const QPointF currentVec = scenePos - m_itemCenter;
        double startAngle = std::atan2(startVec.y(), startVec.x());
        double currentAngle = std::atan2(currentVec.y(), currentVec.x());
        double deltaAngle = (currentAngle - startAngle) * 180.0 / M_PI;

        xform->SetRotationDegrees(m_startRotation + deltaAngle);
    }
    // Scale
    else if (m_activeHandleId.startsWith("scale"))
    {
        ApplyScale(item, xform, scenePos, sceneDelta);
    }
}

void TransformInputHandler::ApplyScale(SceneElementItem* item, TransformComponent* xform, const QPointF& scenePos, const QPointF& sceneDelta)
{
    double newW = m_startScale.x();
    double newH = m_startScale.y();

    if (m_activeHandleId == "scale_right")
    {
        newW = std::max(10.0, m_startScale.x() + sceneDelta.x());
    }
    else if (m_activeHandleId == "scale_left")
    {
        newW = std::max(10.0, m_startScale.x() - sceneDelta.x());
    }
    else if (m_activeHandleId == "scale_bottom")
    {
        newH = std::max(10.0, m_startScale.y() + sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_top")
    {
        newH = std::max(10.0, m_startScale.y() - sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_top_left")
    {
        newW = std::max(10.0, m_startScale.x() - sceneDelta.x());
        newH = std::max(10.0, m_startScale.y() - sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_top_right")
    {
        newW = std::max(10.0, m_startScale.x() + sceneDelta.x());
        newH = std::max(10.0, m_startScale.y() - sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_bottom_left")
    {
        newW = std::max(10.0, m_startScale.x() - sceneDelta.x());
        newH = std::max(10.0, m_startScale.y() + sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_bottom_right")
    {
        newW = std::max(10.0, m_startScale.x() + sceneDelta.x());
        newH = std::max(10.0, m_startScale.y() + sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_uniform")
    {
        const double startDist = std::hypot(m_startScenePos.x() - m_itemCenter.x(),
                                             m_startScenePos.y() - m_itemCenter.y());
        const double currentDist = std::hypot(scenePos.x() - m_itemCenter.x(),
                                               scenePos.y() - m_itemCenter.y());

        if (startDist > 1.0)
        {
            double factor = currentDist / startDist;
            newW = std::max(10.0, m_startScale.x() * factor);
            newH = std::max(10.0, m_startScale.y() * factor);
        }
    }

    xform->SetScale(QPointF(newW, newH));

    // Adjust position for left-side handles
    if (m_activeHandleId == "scale_left" ||
        m_activeHandleId == "scale_top_left" ||
        m_activeHandleId == "scale_bottom_left")
    {
        double widthDelta = newW - m_startScale.x();
        item->setPos(QPointF(m_startItemPos.x() - widthDelta, item->pos().y()));
    }

    // Adjust position for top-side handles
    if (m_activeHandleId == "scale_top" ||
        m_activeHandleId == "scale_top_left" ||
        m_activeHandleId == "scale_top_right")
    {
        double heightDelta = newH - m_startScale.y();
        item->setPos(QPointF(item->pos().x(), m_startItemPos.y() - heightDelta));
    }
}
