#include "input/TransformInputHandler.hpp"
#include "gizmos/GizmoManager.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"
#include "core/GridSnap.hpp"
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
        s.startSceneRect = item->sceneBoundingRect();
        if (s.xform)
        {
            s.startPosition = s.xform->GetPosition();
            s.startRotation = s.xform->GetRotationDegrees();
            s.startScale = s.xform->GetScale();
        }
        m_startStates.append(s);
    }

    // Per-element start poses are now the undo source of truth; no full-scene
    // JSON snapshot is taken on press. The delta is built at release time.
    Q_UNUSED(ctx);

    m_gizmoManager->SetActiveHandle(m_activeHandleId);

    emit TransformStarted();

    return InputResult::Consumed(hit.cursor, true);
}

InputResult TransformInputHandler::HandleMove(const MouseMoveEvent& event, EditorContext& ctx)
{
    if (!m_transforming || !m_gizmoManager)
    {
        // Just update cursor on hover
        if (m_gizmoManager)
        {
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
        }

        return InputResult::NotConsumed();
    }

    if (m_startStates.isEmpty())
        return InputResult::NotConsumed();

    const QPointF sceneDelta = event.scenePos - m_startScenePos;

    ApplyTransform(event.scenePos, sceneDelta, ctx);

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

    Q_UNUSED(ctx);

    // Build a per-element delta list. Only include elements whose pose
    // actually changed (skip ones the user grabbed but didn't ultimately
    // move); if no element changed we emit nothing and the undo stack
    // gets no entry for this gesture.
    QList<TransformDelta> deltas;
    for (const ItemStartState& s : m_startStates)
    {
        if (!s.item || !s.xform)
            continue;

        TransformDelta d;
        d.id              = s.item->GetElement()->GetId();
        d.beforePos       = s.startPosition;
        d.beforeRotation  = s.startRotation;
        d.beforeScale     = s.startScale;
        d.afterPos        = s.xform->GetPosition();
        d.afterRotation   = s.xform->GetRotationDegrees();
        d.afterScale      = s.xform->GetScale();

        if (d.beforePos == d.afterPos
            && d.beforeRotation == d.afterRotation
            && d.beforeScale == d.afterScale)
        {
            continue;
        }

        deltas.append(d);
    }

    if (!deltas.isEmpty())
        emit TransformEnded(deltas, GetUndoActionName());

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

void TransformInputHandler::ApplyTransform(const QPointF& scenePos, const QPointF& sceneDelta, EditorContext& ctx)
{
    // Resolve the shared translation delta once (used by all three translate
    // handles). The x-only / y-only handles zero the locked axis.
    const bool isTranslate = m_activeHandleId.startsWith("translate");

    QPointF translateDelta = sceneDelta;
    if (m_activeHandleId == "translate_x")
        translateDelta.setY(0.0);
    else if (m_activeHandleId == "translate_y")
        translateDelta.setX(0.0);

    // Grid snapping: snap the reference (first) item's resulting position to
    // the nearest grid intersection and reuse that same delta for the whole
    // selection, so a group moves rigidly and snaps as one instead of each
    // item snapping independently. Only the axes this handle actually moves are
    // snapped. The snap is done in SCENE space (mapping through the item's
    // parent) because item positions are parent-relative but the grid/canvas
    // lives in scene coordinates - so a nested element still snaps to the
    // on-screen grid, not to an offset copy of it.
    if (isTranslate && GridSnap::Enabled() && ctx.document && !m_startStates.isEmpty()
        && m_startStates.first().item)
    {
        const QRectF canvas = ctx.document->GetCanvasRect();
        SceneElementItem* refItem = m_startStates.first().item;
        QGraphicsItem* parentItem = refItem->parentItem();
        const QPointF refStart = m_startStates.first().startItemPos;

        const QPointF targetItemPos = refStart + translateDelta;
        const QPointF targetScene = parentItem ? parentItem->mapToScene(targetItemPos) : targetItemPos;
        const QPointF snappedScene = GridSnap::Snap(targetScene, canvas);
        const QPointF snappedItemPos = parentItem ? parentItem->mapFromScene(snappedScene) : snappedScene;

        if (m_activeHandleId != "translate_y")
            translateDelta.setX(snappedItemPos.x() - refStart.x());
        if (m_activeHandleId != "translate_x")
            translateDelta.setY(snappedItemPos.y() - refStart.y());
    }

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

        if (isTranslate)
        {
            s.item->setPos(s.startItemPos + translateDelta);
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
            ApplyScale(s, scenePos, sceneDelta, ctx);
        }
    }
}

void TransformInputHandler::ApplyScale(const ItemStartState& s, const QPointF& scenePos, const QPointF& sceneDelta, EditorContext& ctx)
{
    if (!s.item || !s.xform)
        return;

    // An axis where stretch fills the parent (LEFT|RIGHT or TOP|BOTTOM in the
    // stretch bitmask) gets its size from parentRect - 2*position, not from
    // xform.scale. Touching scale on that axis is a no-op for rendering, and
    // any position compensation would just confuse the user, so we skip the
    // whole axis for stretched elements.
    const auto stretchFlags = s.xform->GetStretch();
    const bool stretchX = stretchFlags.testFlag(Anchor::LEFT) && stretchFlags.testFlag(Anchor::RIGHT);
    const bool stretchY = stretchFlags.testFlag(Anchor::TOP)  && stretchFlags.testFlag(Anchor::BOTTOM);

    double newW = s.startScale.x();
    double newH = s.startScale.y();

    if (m_activeHandleId == "scale_right")
    {
        if (!stretchX) newW = std::max(10.0, s.startScale.x() + sceneDelta.x());
    }
    else if (m_activeHandleId == "scale_left")
    {
        if (!stretchX) newW = std::max(10.0, s.startScale.x() - sceneDelta.x());
    }
    else if (m_activeHandleId == "scale_bottom")
    {
        if (!stretchY) newH = std::max(10.0, s.startScale.y() + sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_top")
    {
        if (!stretchY) newH = std::max(10.0, s.startScale.y() - sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_top_left")
    {
        if (!stretchX) newW = std::max(10.0, s.startScale.x() - sceneDelta.x());
        if (!stretchY) newH = std::max(10.0, s.startScale.y() - sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_top_right")
    {
        if (!stretchX) newW = std::max(10.0, s.startScale.x() + sceneDelta.x());
        if (!stretchY) newH = std::max(10.0, s.startScale.y() - sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_bottom_left")
    {
        if (!stretchX) newW = std::max(10.0, s.startScale.x() - sceneDelta.x());
        if (!stretchY) newH = std::max(10.0, s.startScale.y() + sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_bottom_right")
    {
        if (!stretchX) newW = std::max(10.0, s.startScale.x() + sceneDelta.x());
        if (!stretchY) newH = std::max(10.0, s.startScale.y() + sceneDelta.y());
    }
    else if (m_activeHandleId == "scale_uniform")
    {
        const double startDist = std::hypot(m_startScenePos.x() - m_itemCenter.x(),
                                             m_startScenePos.y() - m_itemCenter.y());
        const double currentDist = std::hypot(scenePos.x() - m_itemCenter.x(),
                                               scenePos.y() - m_itemCenter.y());

        if (startDist > 1.0)
        {
            const double factor = currentDist / startDist;
            if (!stretchX) newW = std::max(10.0, s.startScale.x() * factor);
            if (!stretchY) newH = std::max(10.0, s.startScale.y() * factor);
        }
    }

    // Grid snapping: snap the MOVING edge to the nearest grid line. Which edge
    // actually moves depends on the anchor, not just the handle: a RIGHT-anchored
    // element pins its right edge (so the LEFT edge moves for either X handle), a
    // BOTTOM-anchored element pins its bottom edge, and a LEFT/TOP (default)
    // element moves the dragged edge. A CENTER-anchored axis scales about its
    // centre - which doesn't map cleanly to an edge-on-a-line - so it is left
    // unsnapped, as is uniform scaling (pinning both edges while preserving
    // aspect is over-constrained). The fixed edge is read from the element's
    // start scene rect.
    if (GridSnap::Enabled() && ctx.document && m_activeHandleId != "scale_uniform")
    {
        const QRectF canvas = ctx.document->GetCanvasRect();
        const QRectF sr = s.startSceneRect;
        const QString& h = m_activeHandleId;
        const auto a = s.xform->GetAnchors();

        const bool rightSide  = h == "scale_right" || h == "scale_top_right" || h == "scale_bottom_right";
        const bool leftSide   = h == "scale_left"  || h == "scale_top_left"  || h == "scale_bottom_left";
        const bool bottomSide = h == "scale_bottom" || h == "scale_bottom_left" || h == "scale_bottom_right";
        const bool topSide    = h == "scale_top"    || h == "scale_top_left"    || h == "scale_top_right";

        if ((leftSide || rightSide) && !stretchX && !a.testFlag(Anchor::CENTER_X))
        {
            // RIGHT anchor pins the right edge, so the left edge is the one that
            // moves regardless of handle; otherwise the dragged edge moves.
            if (a.testFlag(Anchor::RIGHT) || leftSide)
            {
                const double edge = GridSnap::Snap(QPointF(sr.right() - newW, sr.center().y()), canvas).x();
                newW = std::max(10.0, sr.right() - edge);
            }
            else
            {
                const double edge = GridSnap::Snap(QPointF(sr.left() + newW, sr.center().y()), canvas).x();
                newW = std::max(10.0, edge - sr.left());
            }
        }

        if ((topSide || bottomSide) && !stretchY && !a.testFlag(Anchor::CENTER_Y))
        {
            if (a.testFlag(Anchor::BOTTOM) || topSide)
            {
                const double edge = GridSnap::Snap(QPointF(sr.center().x(), sr.bottom() - newH), canvas).y();
                newH = std::max(10.0, sr.bottom() - edge);
            }
            else
            {
                const double edge = GridSnap::Snap(QPointF(sr.center().x(), sr.top() + newH), canvas).y();
                newH = std::max(10.0, edge - sr.top());
            }
        }
    }

    s.xform->SetScale(QPointF(newW, newH));

    // Position compensation for left- and top-side handles. The user expects
    // the OPPOSITE edge to stay fixed (drag left edge -> right edge stays;
    // drag top edge -> bottom stays). Translate that intent into a new
    // xform.position depending on the active anchor on each axis, then write
    // it DIRECTLY (not via item->setPos). Going through setPos triggers the
    // ItemPositionHasChanged writeback, which uses the still-stale localRect
    // dimensions to invert the anchor formula and lands xform.position at the
    // wrong value for CENTER/BOTTOM/RIGHT anchored elements - that mismatch
    // is what produced the "scales both ways and keeps walking back" bug.
    const auto anchors = s.xform->GetAnchors();
    const bool isLeftSide  = m_activeHandleId == "scale_left"
                          || m_activeHandleId == "scale_top_left"
                          || m_activeHandleId == "scale_bottom_left";
    const bool isTopSide   = m_activeHandleId == "scale_top"
                          || m_activeHandleId == "scale_top_left"
                          || m_activeHandleId == "scale_top_right";

    QPointF newPos = s.startPosition;

    if (isLeftSide && !stretchX)
    {
        const double dW = newW - s.startScale.x();   // negative when shrinking

        if (anchors.testFlag(Anchor::RIGHT))
            newPos.setX(s.startPosition.x());        // right edge already pinned
        else if (anchors.testFlag(Anchor::CENTER_X))
            newPos.setX(s.startPosition.x() - dW * 0.5);
        else
            newPos.setX(s.startPosition.x() - dW);   // LEFT anchor (default)
    }

    if (isTopSide && !stretchY)
    {
        const double dH = newH - s.startScale.y();

        if (anchors.testFlag(Anchor::BOTTOM))
            newPos.setY(s.startPosition.y());
        else if (anchors.testFlag(Anchor::CENTER_Y))
            newPos.setY(s.startPosition.y() - dH * 0.5);
        else
            newPos.setY(s.startPosition.y() - dH);   // TOP anchor (default)
    }

    if (newPos != s.startPosition)
        s.xform->SetPosition(newPos);
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
