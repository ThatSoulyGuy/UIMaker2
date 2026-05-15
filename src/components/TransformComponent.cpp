#include "components/TransformComponent.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"

REGISTER_COMPONENT(TransformComponent, "Transform")

void TransformComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);

    // Only compute the size here. SceneElementItem applies position/origin/rotation after
    // all component Updates have settled, using the final rect — this keeps the anchor
    // formula agreeing with the localRect that itemChange::ItemPositionHasChanged reads.
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
}
