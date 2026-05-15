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

    QList<SceneElementItem*> selectedItems = GetTopLevelSelectedItems(ctx);

    if (selectedItems.isEmpty())
        return InputResult::NotConsumed();

    const bool isGroup = selectedItems.size() > 1;
    const QRectF sceneBounds = isGroup ? ComputeUnionSceneBounds(selectedItems) : selectedItems.first()->sceneBoundingRect();

    double rotation = 0.0;
    QPointF scale(1.0, 1.0);

    if (!isGroup)
    {
        if (auto* xform = GetTransformComponent(selectedItems.first()))
        {
            rotation = xform->GetRotationDegrees();
            scale = xform->GetScale();
        }
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

    m_startStates.clear();
    for (auto* item : selectedItems)
    {
        if (!item)
            continue;

        ItemStartState s;
        s.item = item;
        s.xform = GetTransformComponent(item);
        s.startItemPos = item->pos();
        s.startSceneCenter = item->sceneBoundingRect().center();
        if (s.xform)
        {
            s.startPosition = s.xform->GetPosition();
            s.startRotation = s.xform->GetRotationDegrees();
            s.startScale = s.xform->GetScale();
        }
        m_startStates.append(s);
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
        QList<SceneElementItem*> selectedItems = GetTopLevelSelectedItems(ctx);

        if (!selectedItems.isEmpty() && ctx.view)
        {
            const QRectF sceneBounds = selectedItems.size() > 1
                ? ComputeUnionSceneBounds(selectedItems)
                : selectedItems.first()->sceneBoundingRect();

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

    if (m_startStates.isEmpty())
        return InputResult::NotConsumed();

    const QPointF sceneDelta = event.scenePos - m_startScenePos;

    ApplyTransform(event.scenePos, sceneDelta);

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
    m_startStates.clear();

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

void TransformInputHandler::ApplyTransform(const QPointF& scenePos, const QPointF& sceneDelta)
{
    // For rotate, the per-item delta angle is identical across the group, so compute once.
    double deltaAngle = 0.0;
    double cosA = 1.0;
    double sinA = 0.0;
    if (m_activeHandleId == "rotate_ring")
    {
        const QPointF startVec = m_startScenePos - m_itemCenter;
        const QPointF currentVec = scenePos - m_itemCenter;
        double startAngle = std::atan2(startVec.y(), startVec.x());
        double currentAngle = std::atan2(currentVec.y(), currentVec.x());
        deltaAngle = (currentAngle - startAngle) * 180.0 / M_PI;

        const double rad = deltaAngle * M_PI / 180.0;
        cosA = std::cos(rad);
        sinA = std::sin(rad);
    }

    for (const ItemStartState& s : m_startStates)
    {
        if (!s.item || !s.xform)
            continue;

        if (m_activeHandleId == "translate_x")
        {
            s.item->setPos(s.startItemPos + QPointF(sceneDelta.x(), 0));
        }
        else if (m_activeHandleId == "translate_y")
        {
            s.item->setPos(s.startItemPos + QPointF(0, sceneDelta.y()));
        }
        else if (m_activeHandleId == "translate_xy")
        {
            s.item->setPos(s.startItemPos + sceneDelta);
        }
        else if (m_activeHandleId == "rotate_ring")
        {
            // Photoshop-style group rotation: orbit each item's center around the shared
            // group pivot (m_itemCenter, in scene coords) AND spin the item itself by the
            // same angle. For a single selection the pivot equals the item's own center,
            // so the orbit delta is zero and this reduces to a pure self-rotation.
            const QPointF v = s.startSceneCenter - m_itemCenter;
            const QPointF rotated(v.x() * cosA - v.y() * sinA,
                                  v.x() * sinA + v.y() * cosA);
            const QPointF newSceneCenter = m_itemCenter + rotated;
            const QPointF sceneShift = newSceneCenter - s.startSceneCenter;

            s.item->setPos(s.startItemPos + sceneShift);
            s.xform->SetRotationDegrees(s.startRotation + deltaAngle);
        }
        else if (m_activeHandleId.startsWith("scale"))
        {
            ApplyScale(s, scenePos, sceneDelta);
        }
    }
}

void TransformInputHandler::ApplyScale(const ItemStartState& s, const QPointF& scenePos, const QPointF& sceneDelta)
{
    if (!s.item || !s.xform)
        return;

    double newW = s.startScale.x();
    double newH = s.startScale.y();

    if (m_activeHandleId == "scale_right")
    {
        newW = std::max(10.0, s.startScale.x() + sceneDelta.x());
    }
    else if (m_activeHandleId == "scale_left")
    {
        newW = std::max(10.0, s.startScale.x() - sceneDelta.x());
    }
    else if (m_activeHandleId == "scale_bottom")
    {
        newH = std::max(10.0, s.startScale.y() + sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_top")
    {
        newH = std::max(10.0, s.startScale.y() - sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_top_left")
    {
        newW = std::max(10.0, s.startScale.x() - sceneDelta.x());
        newH = std::max(10.0, s.startScale.y() - sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_top_right")
    {
        newW = std::max(10.0, s.startScale.x() + sceneDelta.x());
        newH = std::max(10.0, s.startScale.y() - sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_bottom_left")
    {
        newW = std::max(10.0, s.startScale.x() - sceneDelta.x());
        newH = std::max(10.0, s.startScale.y() + sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_bottom_right")
    {
        newW = std::max(10.0, s.startScale.x() + sceneDelta.x());
        newH = std::max(10.0, s.startScale.y() + sceneDelta.y());
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
            newW = std::max(10.0, s.startScale.x() * factor);
            newH = std::max(10.0, s.startScale.y() * factor);
        }
    }

    s.xform->SetScale(QPointF(newW, newH));

    // Adjust position for left-side handles (compensate so the opposite edge stays fixed).
    if (m_activeHandleId == "scale_left" ||
        m_activeHandleId == "scale_top_left" ||
        m_activeHandleId == "scale_bottom_left")
    {
        double widthDelta = newW - s.startScale.x();
        s.item->setPos(QPointF(s.startItemPos.x() - widthDelta, s.item->pos().y()));
    }

    if (m_activeHandleId == "scale_top" ||
        m_activeHandleId == "scale_top_left" ||
        m_activeHandleId == "scale_top_right")
    {
        double heightDelta = newH - s.startScale.y();
        s.item->setPos(QPointF(s.item->pos().x(), s.startItemPos.y() - heightDelta));
    }
}

QList<SceneElementItem*> TransformInputHandler::GetTopLevelSelectedItems(EditorContext& ctx)
{
    QList<SceneElementItem*> result;

    if (!ctx.document)
        return result;

    auto* scene = ctx.document->GetScene();
    if (!scene)
        return result;

    QList<SceneElementItem*> allSelected;
    for (auto* qitem : scene->selectedItems())
    {
        if (auto* sei = dynamic_cast<SceneElementItem*>(qitem))
            allSelected.append(sei);
    }

    // Filter out any item that has a selected ancestor — moving a parent already moves the
    // child via Qt's parent-child coordinate inheritance, so applying the transform to both
    // would double-apply.
    for (auto* item : allSelected)
    {
        bool hasSelectedAncestor = false;
        for (auto* p = item->parentItem(); p != nullptr; p = p->parentItem())
        {
            if (allSelected.contains(dynamic_cast<SceneElementItem*>(p)))
            {
                hasSelectedAncestor = true;
                break;
            }
        }

        if (!hasSelectedAncestor)
            result.append(item);
    }

    return result;
}

QRectF TransformInputHandler::ComputeUnionSceneBounds(const QList<SceneElementItem*>& items)
{
    QRectF result;
    for (auto* item : items)
    {
        if (!item)
            continue;
        if (result.isNull())
            result = item->sceneBoundingRect();
        else
            result = result.united(item->sceneBoundingRect());
    }
    return result;
}
