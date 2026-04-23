#include "components/StackLayoutComponent.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"

REGISTER_COMPONENT(StackLayoutComponent, "StackLayout")

void StackLayoutComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(parentRect);

    auto* element = item.GetElement();

    QObject::connect(element, &UiElement::StructureChanged, this, &StackLayoutComponent::OnChildChanged, Qt::UniqueConnection);

    double offset = m_padding;
    double maxCross = 0.0;

    for (auto* child : item.childItems())
    {
        auto* childItem = dynamic_cast<SceneElementItem*>(child);
        if (!childItem)
            continue;

        auto* childElement = childItem->GetElement();
        for (auto* comp : childElement->GetComponents())
            QObject::connect(comp, &Component::ComponentChanged, this, &StackLayoutComponent::OnChildChanged, Qt::UniqueConnection);

        QRectF childRect = childItem->boundingRect();

        if (m_direction == Vertical)
        {
            childItem->setPosFromComponent(QPointF(m_padding, offset));
            offset += childRect.height() + m_spacing;
            maxCross = std::max(maxCross, childRect.width());
        }
        else
        {
            childItem->setPosFromComponent(QPointF(offset, m_padding));
            offset += childRect.width() + m_spacing;
            maxCross = std::max(maxCross, childRect.height());
        }
    }

    if (offset > m_padding)
        offset -= m_spacing;

    offset += m_padding;

    if (m_direction == Vertical)
        rect.setSize(QSizeF(maxCross + 2 * m_padding, offset));
    else
        rect.setSize(QSizeF(offset, maxCross + 2 * m_padding));
}

bool StackLayoutComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();

    QPen pen(QColor(120, 180, 240, 120), 1, Qt::DashLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect);

    if (selected)
    {
        QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
        selPen.setCosmetic(true);
        painter->setPen(selPen);
        painter->drawRect(rect);
    }

    painter->restore();
    return true;
}
