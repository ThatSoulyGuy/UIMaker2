#include "components/StackLayoutComponent.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"

#include <QJsonObject>

REGISTER_COMPONENT(StackLayoutComponent, "StackLayout")

StackLayoutComponent::StackLayoutComponent(QObject* parent)
    : Component(parent), m_direction(Vertical), m_spacing(8.0), m_padding(8.0) { }

QString StackLayoutComponent::GetTypeName() const { return QStringLiteral("StackLayout"); }
int StackLayoutComponent::UpdateOrder() const { return 100; }
bool StackLayoutComponent::IsLayout() const { return true; }

void StackLayoutComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(parentRect);

    auto* element = item.GetElement();

    // Drop connections to ex-children so removed/reparented items stop
    // triggering relayout of this container, then re-wire the current set.
    for (const auto& conn : m_childConnections)
        QObject::disconnect(conn);
    m_childConnections.clear();

    m_childConnections.append(QObject::connect(element, &UiElement::StructureChanged, this, &StackLayoutComponent::OnChildChanged));

    double offset = m_padding;
    double maxCross = 0.0;

    for (auto* child : item.childItems())
    {
        auto* childItem = dynamic_cast<SceneElementItem*>(child);
        if (!childItem)
            continue;

        auto* childElement = childItem->GetElement();
        for (auto* comp : childElement->GetComponents())
            m_childConnections.append(QObject::connect(comp, &Component::ComponentChanged, this, &StackLayoutComponent::OnChildChanged));

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

int StackLayoutComponent::GetDirectionInt() const noexcept { return m_direction; }
void StackLayoutComponent::SetDirectionInt(int v) { SetDirection(static_cast<Direction>(v)); }

StackLayoutComponent::Direction StackLayoutComponent::GetDirection() const noexcept { return m_direction; }
void StackLayoutComponent::SetDirection(Direction v) { if (m_direction == v) return; m_direction = v; NotifyChanged(); }

double StackLayoutComponent::GetSpacing() const noexcept { return m_spacing; }
void StackLayoutComponent::SetSpacing(double v) { if (m_spacing == v) return; m_spacing = v; NotifyChanged(); }

double StackLayoutComponent::GetPadding() const noexcept { return m_padding; }
void StackLayoutComponent::SetPadding(double v) { if (m_padding == v) return; m_padding = v; NotifyChanged(); }

void StackLayoutComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "StackLayout";
    out["direction"] = static_cast<int>(m_direction);
    out["spacing"] = m_spacing;
    out["padding"] = m_padding;
}

void StackLayoutComponent::FromJson(const QJsonObject& in)
{
    SetDirection(static_cast<Direction>(in["direction"].toInt(0)));
    SetSpacing(in["spacing"].toDouble(8.0));
    SetPadding(in["padding"].toDouble(8.0));
}

void StackLayoutComponent::OnChildChanged() { NotifyChanged(); }
