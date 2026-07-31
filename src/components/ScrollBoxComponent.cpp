#include "components/ScrollBoxComponent.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"

#include <QJsonObject>

REGISTER_COMPONENT(ScrollBoxComponent, "ScrollBox")

ScrollBoxComponent::ScrollBoxComponent(QObject* parent)
    : Component(parent), m_direction(Vertical), m_spacing(8.0), m_padding(8.0) { }

QString ScrollBoxComponent::GetTypeName() const { return QStringLiteral("ScrollBox"); }
int ScrollBoxComponent::UpdateOrder() const { return 100; }
bool ScrollBoxComponent::IsLayout() const { return true; }

void ScrollBoxComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(parentRect);

    auto* element = item.GetElement();

    item.setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    // Drop connections to ex-children so removed/reparented items stop
    // triggering relayout of this container, then re-wire the current set.
    for (const auto& conn : m_childConnections)
        QObject::disconnect(conn);
    m_childConnections.clear();

    m_childConnections.append(QObject::connect(element, &UiElement::StructureChanged, this, &ScrollBoxComponent::OnChildChanged));

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
            m_childConnections.append(QObject::connect(comp, &Component::ComponentChanged, this, &ScrollBoxComponent::OnChildChanged));

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

int ScrollBoxComponent::GetDirectionInt() const noexcept { return m_direction; }
void ScrollBoxComponent::SetDirectionInt(int v) { SetDirection(static_cast<Direction>(v)); }

ScrollBoxComponent::Direction ScrollBoxComponent::GetDirection() const noexcept { return m_direction; }
void ScrollBoxComponent::SetDirection(Direction v) { if (m_direction == v) return; m_direction = v; NotifyChanged(); }

double ScrollBoxComponent::GetSpacing() const noexcept { return m_spacing; }
void ScrollBoxComponent::SetSpacing(double v) { if (m_spacing == v) return; m_spacing = v; NotifyChanged(); }

double ScrollBoxComponent::GetPadding() const noexcept { return m_padding; }
void ScrollBoxComponent::SetPadding(double v) { if (m_padding == v) return; m_padding = v; NotifyChanged(); }

void ScrollBoxComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "ScrollBox";
    out["direction"] = static_cast<int>(m_direction);
    out["spacing"] = m_spacing;
    out["padding"] = m_padding;
}

void ScrollBoxComponent::FromJson(const QJsonObject& in)
{
    SetDirection(static_cast<Direction>(in["direction"].toInt(0)));
    SetSpacing(in["spacing"].toDouble(8.0));
    SetPadding(in["padding"].toDouble(8.0));
}

void ScrollBoxComponent::OnChildChanged() { NotifyChanged(); }
