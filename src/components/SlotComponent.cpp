#include "components/SlotComponent.hpp"

#include <algorithm>

#include "core/UiElement.hpp"
#include "components/TabContainerComponent.hpp"
#include "scene/SceneElementItem.hpp"

REGISTER_COMPONENT(SlotComponent, "Slot")

void SlotComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    double topInset = 0.0;

    if (m_masterKind == QLatin1String("TabContainer"))
    {
        if (auto* slave = qobject_cast<UiElement*>(parent()))
        {
            if (auto* master = qobject_cast<UiElement*>(slave->parent()))
            {
                if (auto* tc = master->GetComponent<TabContainerComponent>())
                    topInset = static_cast<double>(tc->GetTabHeight());
            }
        }
    }

    const double w = parentRect.width();
    const double h = std::max(0.0, parentRect.height() - topInset);

    rect = QRectF(0.0, 0.0, w, h);

    item.setPosFromComponent(parentRect.topLeft() + QPointF(0.0, topInset));
}
