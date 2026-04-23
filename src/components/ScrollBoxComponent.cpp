#include "components/ScrollBoxComponent.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"

REGISTER_COMPONENT(ScrollBoxComponent, "ScrollBox")

void ScrollBoxComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(parentRect);

    auto* element = item.GetElement();

    item.setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    QObject::connect(element, &UiElement::StructureChanged, this, &ScrollBoxComponent::OnChildChanged, Qt::UniqueConnection);

    // The scroll box keeps its size from TransformComponent on the scroll axis,
    // and fits children on the cross axis.
    // Vertical scroll: fixed height (from TransformComponent scale), width fits children
    // Horizontal scroll: fixed width (from TransformComponent scale), height fits children
    double fixedExtent = (m_direction == Vertical) ? rect.height() : rect.width();

    double offset = m_padding;
    double maxCross = 0.0;

    for (auto* child : item.childItems())
    {
        auto* childItem = dynamic_cast<SceneElementItem*>(child);
        if (!childItem)
            continue;

        auto* childElement = childItem->GetElement();
        for (auto* comp : childElement->GetComponents())
            QObject::connect(comp, &Component::ComponentChanged, this, &ScrollBoxComponent::OnChildChanged, Qt::UniqueConnection);

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

    // Cross axis fits children, scroll axis stays fixed
    if (m_direction == Vertical)
        rect.setSize(QSizeF(maxCross + 2 * m_padding, fixedExtent));
    else
        rect.setSize(QSizeF(fixedExtent, maxCross + 2 * m_padding));
}

bool ScrollBoxComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();

    // Clip children to the scroll box bounds
    painter->setClipRect(rect);

    QPen pen(QColor(140, 220, 160, 120), 1, Qt::DashLine);
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
