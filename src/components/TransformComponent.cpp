#include "components/TransformComponent.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"

REGISTER_COMPONENT(TransformComponent, "Transform")

void TransformComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    QPointF pos = position;

    double targetW = rect.width();
    double targetH = rect.height();

    if (scale.x() > 0.0) targetW = scale.x();
    if (scale.y() > 0.0) targetH = scale.y();

    if (stretch.testFlag(Anchor::LEFT) && stretch.testFlag(Anchor::RIGHT))
        targetW = parentRect.width() - pos.x() * 2.0;

    if (stretch.testFlag(Anchor::TOP) && stretch.testFlag(Anchor::BOTTOM))
        targetH = parentRect.height() - pos.y() * 2.0;

    targetW = std::max(0.0001, targetW);
    targetH = std::max(0.0001, targetH);

    rect.setSize(QSizeF(targetW, targetH));

    // Check if parent element has a layout component - if so, layout handles positioning
    bool parentHasLayout = false;
    auto* element = qobject_cast<UiElement*>(this->parent());
    if (element)
    {
        auto* parentElement = qobject_cast<UiElement*>(element->parent());
        if (parentElement)
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
    }

    if (!parentHasLayout)
    {
        double x = pos.x();
        double y = pos.y();

        if (anchors.testFlag(Anchor::RIGHT))
            x = parentRect.width() - rect.width() - pos.x();
        else if (anchors.testFlag(Anchor::CENTER_X))
            x = (parentRect.width() - rect.width()) * 0.5 + pos.x();

        if (anchors.testFlag(Anchor::BOTTOM))
            y = parentRect.height() - rect.height() - pos.y();
        else if (anchors.testFlag(Anchor::CENTER_Y))
            y = (parentRect.height() - rect.height()) * 0.5 + pos.y();

        item.setPosFromComponent(parentRect.topLeft() + QPointF(x, y));
    }

    item.setTransformOriginPoint(rect.center());
    item.setRotationFromComponent(rotationDegrees);
}
