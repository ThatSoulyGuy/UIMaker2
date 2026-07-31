#include "scene/SceneElementItem.hpp"
#include <QFontMetrics>
#include <QGraphicsScene>
#include <QPointer>
#include <cmath>
#include <algorithm>
#include "core/UiElement.hpp"
#include "core/Component.hpp"
#include "components/TransformComponent.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward/inverse pair for anchor-based positioning. AnchorAdjustedItemPos maps a
// component-space position to an item pos inside parentRect; InverseAnchorComponentPos
// maps an item pos back to the component-space position. They are exact inverses of
// each other and must change together.
static QPointF AnchorAdjustedItemPos(const QPointF& pos, AnchorFlags anchors, const QRectF& parentRect, double w, double h)
{
    double x = pos.x();
    double y = pos.y();

    if (anchors.testFlag(Anchor::RIGHT))
        x = parentRect.width() - w - pos.x();
    else if (anchors.testFlag(Anchor::CENTER_X))
        x = (parentRect.width() - w) * 0.5 + pos.x();

    if (anchors.testFlag(Anchor::BOTTOM))
        y = parentRect.height() - h - pos.y();
    else if (anchors.testFlag(Anchor::CENTER_Y))
        y = (parentRect.height() - h) * 0.5 + pos.y();

    return parentRect.topLeft() + QPointF(x, y);
}

static QPointF InverseAnchorComponentPos(const QPointF& itemPos, AnchorFlags anchors, const QRectF& parentRect, double w, double h)
{
    QPointF p = itemPos - parentRect.topLeft();

    if (anchors.testFlag(Anchor::RIGHT))
        p.setX(parentRect.width() - w - p.x());
    else if (anchors.testFlag(Anchor::CENTER_X))
        p.setX(p.x() - (parentRect.width() - w) * 0.5);

    if (anchors.testFlag(Anchor::BOTTOM))
        p.setY(parentRect.height() - h - p.y());
    else if (anchors.testFlag(Anchor::CENTER_Y))
        p.setY(p.y() - (parentRect.height() - h) * 0.5);

    return p;
}

SceneElementItem::SceneElementItem(UiElement* element) : QGraphicsObject(nullptr), element(element), localRect(-50.0, -25.0, 100.0, 50.0)
{
    const bool isSlot = element && element->IsSlot();

    setFlag(QGraphicsItem::ItemIsMovable, !isSlot);
    setFlag(QGraphicsItem::ItemIsSelectable, !isSlot);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    if (isSlot)
        setAcceptedMouseButtons(Qt::NoButton);

    for (auto* comp : element->GetComponents())
        QObject::connect(comp, &Component::ComponentChanged, this, &SceneElementItem::OnComponentChanged, Qt::UniqueConnection);

    QObject::connect(element, &UiElement::ComponentListChanged, this, &SceneElementItem::RefreshFromComponents);

    RefreshFromComponents();
}

void SceneElementItem::OnComponentChanged()
{
    if (pendingRefresh)
        return;

    pendingRefresh = true;

    QPointer<SceneElementItem> guard(this);

    QMetaObject::invokeMethod(this, [guard]()
    {
        if (!guard) return;
        guard->RefreshFromComponents();
        guard->update();
        guard->pendingRefresh = false;
    }, Qt::QueuedConnection);
}


void SceneElementItem::RefreshFromComponents()
{
    for (auto* comp : element->GetComponents())
        QObject::connect(comp, &Component::ComponentChanged, this, &SceneElementItem::OnComponentChanged, Qt::UniqueConnection);

    if (!element->GetComponent<TransformComponent>())
        element->AddComponent<TransformComponent>();

    QRectF parentRect;

    if (auto* p = parentItem())
        parentRect = p->boundingRect();
    else if (scene())
        parentRect = scene()->sceneRect();

    QRectF newRect(0.0, 0.0, 100.0, 50.0);
    auto comps = element->GetComponents();
    std::sort(comps.begin(), comps.end(), [](Component* a, Component* b){ return a->UpdateOrder() < b->UpdateOrder(); });
    for (auto* comp : comps)
        comp->Update(*this, newRect, parentRect);

    // Apply anchor-based positioning using the FINAL rect size after all component Updates
    // have settled. Doing this here (rather than inside TransformComponent::Update) ensures
    // the anchor math agrees with itemChange::ItemPositionHasChanged, which always uses the
    // committed localRect width/height. Otherwise layouts that overwrite rect after Transform
    // runs would cause per-frame position drift while dragging.
    if (auto* xform = element->GetComponent<TransformComponent>())
    {
        bool parentHasLayout = false;
        if (auto* parentElement = qobject_cast<UiElement*>(element->parent()))
        {
            for (auto* comp : parentElement->GetComponents())
            {
                if (comp->IsLayout())
                {
                    parentHasLayout = true;
                    break;
                }
            }
        }

        if (!parentHasLayout)
            setPosFromComponent(AnchorAdjustedItemPos(xform->GetPosition(), xform->GetAnchors(), parentRect, newRect.width(), newRect.height()));

        setTransformOriginPoint(newRect.center());
        setRotationFromComponent(xform->GetRotationDegrees());
    }

    const bool rectChanged = (newRect != localRect);

    if (rectChanged)
    {
        prepareGeometryChange();
        localRect = newRect;

        // Upward cascade: if our parent owns a layout component, re-run the parent's layout
        // so siblings (and us) get repositioned given the new sizes. Skipped when this
        // refresh was itself triggered as part of a parent->child downward cascade, to
        // prevent ping-ponging between parent and child refreshes.
        if (!inLayoutRefresh && !inDownwardCascade)
        {
            if (auto* parentSEI = dynamic_cast<SceneElementItem*>(parentItem()))
            {
                if (!parentSEI->inLayoutRefresh)
                {
                    auto* parentElement = parentSEI->GetElement();
                    for (auto* comp : parentElement->GetComponents())
                    {
                        if (comp->IsLayout())
                        {
                            parentSEI->inLayoutRefresh = true;
                            parentSEI->RefreshFromComponents();
                            parentSEI->update();
                            parentSEI->inLayoutRefresh = false;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Downward cascade: any change to our committed localRect can shift the
    // effective size and position of descendants that anchor/stretch against
    // us (CENTER/RIGHT/BOTTOM anchors, LEFT|RIGHT or TOP|BOTTOM stretch). Walk
    // direct SEI children and re-run their refresh; the recursive descent is
    // implicit because each child's refresh fires its own downward cascade.
    //
    // The inDownwardCascade flag on the visited children disables their upward
    // cascade for the duration of the visit - we are the one driving the
    // resize and don't want it to bounce back into our refresh.
    if (rectChanged && !inLayoutRefresh)
    {
        for (auto* child : childItems())
        {
            if (auto* childSEI = dynamic_cast<SceneElementItem*>(child))
            {
                childSEI->inDownwardCascade = true;
                childSEI->RefreshFromComponents();
                childSEI->update();
                childSEI->inDownwardCascade = false;
            }
        }
    }
}

void SceneElementItem::setPosFromComponent(const QPointF& p)
{
    ignorePositionFeedback = true;
    setPos(p);
    ignorePositionFeedback = false;
}

void SceneElementItem::setRotationFromComponent(double deg)
{
    ignoreRotationFeedback = true;
    setRotation(deg);
    ignoreRotationFeedback = false;
}

QVariant SceneElementItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged)
    {
        if (!ignorePositionFeedback)
        {
            if (auto* xform = element->GetComponent<TransformComponent>())
            {
                QRectF parentRect;

                if (auto* p = parentItem())
                    parentRect = p->boundingRect();
                else if (scene())
                    parentRect = scene()->sceneRect();

                xform->SetPosition(InverseAnchorComponentPos(pos(), xform->GetAnchors(), parentRect, localRect.width(), localRect.height()));
            }
        }
    }

    if (change == ItemRotationHasChanged)
    {
        if (!ignoreRotationFeedback)
        {
            if (auto* xform = element->GetComponent<TransformComponent>())
                xform->SetRotationDegrees(rotation());
        }
    }

    if (change == ItemScaleHasChanged)
    {
        // If you ever let users scale the QGraphicsItem directly, you can write back here.
        // Currently we scale via rect, so leaving as-is is fine or remove this block.
    }

    return QGraphicsObject::itemChange(change, value);
}

void SceneElementItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();
    bool painted = false;
    auto comps = element->GetComponents();
    std::sort(comps.begin(), comps.end(), [](Component* a, Component* b){ return a->UpdateOrder() < b->UpdateOrder(); });
    for (auto* comp : comps)
        painted |= comp->Paint(painter, localRect, isSelected());

    if (!painted && !element->IsSlot())
    {
        painter->setBrush(QColor(80, 80, 80));
        painter->setPen(Qt::NoPen);
        painter->drawRect(localRect);
        if (isSelected())
        {
            painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(localRect);
        }
    }

    painter->restore();
}

QRectF SceneElementItem::boundingRect() const
{
    return localRect;
}

UiElement* SceneElementItem::GetElement() const noexcept
{
    return element;
}
