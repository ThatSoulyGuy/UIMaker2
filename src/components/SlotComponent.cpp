#include "components/SlotComponent.hpp"

#include <algorithm>

#include <QJsonObject>

#include "core/UiElement.hpp"
#include "components/TabContainerComponent.hpp"
#include "scene/SceneElementItem.hpp"

REGISTER_COMPONENT(SlotComponent, "Slot")

SlotComponent::SlotComponent(QObject* parent) : Component(parent), m_slotIndex(0) { }

QString SlotComponent::GetTypeName() const
{
    return QStringLiteral("Slot");
}

int SlotComponent::UpdateOrder() const
{
    return 100;
}

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

int SlotComponent::GetSlotIndex() const noexcept
{
    return m_slotIndex;
}

void SlotComponent::SetSlotIndex(int v)
{
    if (m_slotIndex == v)
        return;

    m_slotIndex = v;

    NotifyChanged();
}

QString SlotComponent::GetMasterKind() const noexcept
{
    return m_masterKind;
}

void SlotComponent::SetMasterKind(const QString& v)
{
    if (m_masterKind == v)
        return;

    m_masterKind = v;

    NotifyChanged();
}

void SlotComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Slot";
    out["slotIndex"] = m_slotIndex;
    out["masterKind"] = m_masterKind;
}

void SlotComponent::FromJson(const QJsonObject& in)
{
    SetSlotIndex(in["slotIndex"].toInt(0));
    SetMasterKind(in["masterKind"].toString());
}
