#include "components/PanelComponent.hpp"

#include <QPainter>
#include <QPen>

REGISTER_COMPONENT(PanelComponent, "Panel")

PanelComponent::PanelComponent(QObject* parent)
    : Component(parent)
    , m_backgroundColor(QColor(50, 50, 55, 200))
    , m_borderColor(QColor(100, 100, 110))
    , m_borderWidth(1.0)
    , m_cornerRadius(6.0) { }

QString PanelComponent::GetTypeName() const { return QStringLiteral("Panel"); }

bool PanelComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (m_borderWidth > 0.0)
    {
        QPen pen(m_borderColor, m_borderWidth);
        pen.setCosmetic(true);
        painter->setPen(pen);
    }
    else
    {
        painter->setPen(Qt::NoPen);
    }

    painter->setBrush(m_backgroundColor);
    painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);

    if (selected)
    {
        QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
        selPen.setCosmetic(true);
        painter->setPen(selPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);
    }

    painter->restore();
    return true;
}

QColor PanelComponent::GetBackgroundColor() const noexcept { return m_backgroundColor; }
void PanelComponent::SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

QColor PanelComponent::GetBorderColor() const noexcept { return m_borderColor; }
void PanelComponent::SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

double PanelComponent::GetBorderWidth() const noexcept { return m_borderWidth; }
void PanelComponent::SetBorderWidth(double v) { if (m_borderWidth == v) return; m_borderWidth = v; NotifyChanged(); }

double PanelComponent::GetCornerRadius() const noexcept { return m_cornerRadius; }
void PanelComponent::SetCornerRadius(double v) { if (m_cornerRadius == v) return; m_cornerRadius = v; NotifyChanged(); }

void PanelComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Panel";
    out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
    out["borderColor"] = m_borderColor.name(QColor::HexArgb);
    out["borderWidth"] = m_borderWidth;
    out["cornerRadius"] = m_cornerRadius;
}

void PanelComponent::FromJson(const QJsonObject& in)
{
    SetBackgroundColor(QColor(in["backgroundColor"].toString("#C8323237")));
    SetBorderColor(QColor(in["borderColor"].toString("#FF64646E")));
    SetBorderWidth(in["borderWidth"].toDouble(1.0));
    SetCornerRadius(in["cornerRadius"].toDouble(6.0));
}
