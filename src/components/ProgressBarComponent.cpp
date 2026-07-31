#include "components/ProgressBarComponent.hpp"

#include <QPainter>
#include <QPen>
#include <algorithm>

REGISTER_COMPONENT(ProgressBarComponent, "ProgressBar")

ProgressBarComponent::ProgressBarComponent(QObject* parent)
    : Component(parent)
    , m_value(0.5)
    , m_fillColor(QColor(50, 200, 80))
    , m_backgroundColor(QColor(40, 40, 45))
    , m_borderColor(QColor(80, 80, 90))
    , m_direction(Horizontal)
    , m_cornerRadius(4.0) { }

QString ProgressBarComponent::GetTypeName() const { return QStringLiteral("ProgressBar"); }

bool ProgressBarComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen borderPen(m_borderColor, 1);
    borderPen.setCosmetic(true);
    painter->setPen(borderPen);
    painter->setBrush(m_backgroundColor);
    painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);

    double clamped = std::clamp(m_value, 0.0, 1.0);
    QRectF fillRect;

    if (m_direction == Horizontal)
        fillRect = QRectF(rect.x(), rect.y(), rect.width() * clamped, rect.height());
    else
        fillRect = QRectF(rect.x(), rect.y() + rect.height() * (1.0 - clamped), rect.width(), rect.height() * clamped);

    if (clamped > 0.0)
    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_fillColor);
        painter->drawRoundedRect(fillRect, m_cornerRadius, m_cornerRadius);
    }

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

double ProgressBarComponent::GetValue() const noexcept { return m_value; }
void ProgressBarComponent::SetValue(double v) { v = std::clamp(v, 0.0, 1.0); if (m_value == v) return; m_value = v; NotifyChanged(); }

QColor ProgressBarComponent::GetFillColor() const noexcept { return m_fillColor; }
void ProgressBarComponent::SetFillColor(const QColor& v) { if (m_fillColor == v) return; m_fillColor = v; NotifyChanged(); }

QColor ProgressBarComponent::GetBackgroundColor() const noexcept { return m_backgroundColor; }
void ProgressBarComponent::SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

QColor ProgressBarComponent::GetBorderColor() const noexcept { return m_borderColor; }
void ProgressBarComponent::SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

int ProgressBarComponent::GetDirectionInt() const noexcept { return m_direction; }
void ProgressBarComponent::SetDirectionInt(int v) { SetDirection(static_cast<Direction>(v)); }

ProgressBarComponent::Direction ProgressBarComponent::GetDirection() const noexcept { return m_direction; }
void ProgressBarComponent::SetDirection(Direction v) { if (m_direction == v) return; m_direction = v; NotifyChanged(); }

double ProgressBarComponent::GetCornerRadius() const noexcept { return m_cornerRadius; }
void ProgressBarComponent::SetCornerRadius(double v) { if (m_cornerRadius == v) return; m_cornerRadius = v; NotifyChanged(); }

void ProgressBarComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "ProgressBar";
    out["value"] = m_value;
    out["fillColor"] = m_fillColor.name(QColor::HexArgb);
    out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
    out["borderColor"] = m_borderColor.name(QColor::HexArgb);
    out["direction"] = static_cast<int>(m_direction);
    out["cornerRadius"] = m_cornerRadius;
}

void ProgressBarComponent::FromJson(const QJsonObject& in)
{
    SetValue(in["value"].toDouble(0.5));
    SetFillColor(QColor(in["fillColor"].toString("#FF32C850")));
    SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF28282D")));
    SetBorderColor(QColor(in["borderColor"].toString("#FF50505A")));
    SetDirection(static_cast<Direction>(in["direction"].toInt(0)));
    SetCornerRadius(in["cornerRadius"].toDouble(4.0));
}
