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

    if (newRect != localRect)
    {
        prepareGeometryChange();
        localRect = newRect;

        // If parent has a layout component, trigger parent refresh so layout re-runs
        if (!inLayoutRefresh)
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

                auto anchors = xform->GetAnchors();
                QPointF p = pos() - parentRect.topLeft();

                if (anchors.testFlag(Anchor::RIGHT))
                    p.setX(parentRect.width() - localRect.width() - p.x());
                else if (anchors.testFlag(Anchor::CENTER_X))
                    p.setX(p.x() - (parentRect.width() - localRect.width()) * 0.5);

                if (anchors.testFlag(Anchor::BOTTOM))
                    p.setY(parentRect.height() - localRect.height() - p.y());
                else if (anchors.testFlag(Anchor::CENTER_Y))
                    p.setY(p.y() - (parentRect.height() - localRect.height()) * 0.5);

                xform->SetPosition(p);
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
