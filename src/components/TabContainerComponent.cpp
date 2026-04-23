#include "components/TabContainerComponent.hpp"

#include <QGraphicsItem>

#include "core/UiElement.hpp"
#include "scene/SceneElementItem.hpp"

REGISTER_COMPONENT(TabContainerComponent, "TabContainer")

void TabContainerComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(rect);
    Q_UNUSED(parentRect);

    for (QGraphicsItem* child : item.childItems())
    {
        auto* sei = dynamic_cast<SceneElementItem*>(child);

        if (!sei)
            continue;

        UiElement* elem = sei->GetElement();

        if (elem && elem->IsSlot())
        {
            sei->setVisible(elem->GetSlotIndex() == m_activeTab);
            sei->RefreshFromComponents();
        }
    }
}
