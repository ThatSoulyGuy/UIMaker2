#include "SceneElementItem.hpp"
#include <QFontMetrics>
#include <QGraphicsScene>
#include <cmath>
#include <algorithm>
#include "EntityComponentSystem.hpp"

SceneElementItem::SceneElementItem(UiElement* element) : QGraphicsObject(nullptr), element(element), localRect(-50.0, -25.0, 100.0, 50.0)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

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

    QMetaObject::invokeMethod(this, [this](){ RefreshFromComponents(); update(); pendingRefresh = false; }, Qt::QueuedConnection);
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
            if (auto* xform = element->GetComponent<TransformComponent>()) {
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

    if (!painted)
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
