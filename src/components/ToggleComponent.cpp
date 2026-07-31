#include "components/ToggleComponent.hpp"

#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <algorithm>

REGISTER_COMPONENT(ToggleComponent, "Toggle")

ToggleComponent::ToggleComponent(QObject* parent)
    : Component(parent)
    , m_checked(false)
    , m_onColor(QColor(50, 200, 80))
    , m_offColor(QColor(80, 80, 90))
    , m_knobColor(Qt::white)
    , m_label("Toggle") { }

QString ToggleComponent::GetTypeName() const { return QStringLiteral("Toggle"); }

void ToggleComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);
    Q_UNUSED(parentRect);

    double trackW = 48.0;
    double trackH = 24.0;

    if (!m_label.isEmpty())
    {
        QFont font;
        font.setPixelSize(14);
        QFontMetrics fm(font);
        double textW = fm.horizontalAdvance(m_label);
        rect = QRectF(0, 0, trackW + 8.0 + textW, std::max(trackH, (double)fm.height()));
    }
    else
    {
        rect = QRectF(0, 0, trackW, trackH);
    }
}

bool ToggleComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    double trackW = 48.0;
    double trackH = 24.0;
    double trackY = rect.y() + (rect.height() - trackH) / 2.0;

    QRectF trackRect(rect.x(), trackY, trackW, trackH);
    double radius = trackH / 2.0;

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_checked ? m_onColor : m_offColor);
    painter->drawRoundedRect(trackRect, radius, radius);

    double knobRadius = trackH * 0.4;
    double knobX = m_checked ? (trackRect.right() - radius) : (trackRect.left() + radius);
    double knobY = trackRect.center().y();

    painter->setBrush(m_knobColor);
    painter->drawEllipse(QPointF(knobX, knobY), knobRadius, knobRadius);

    if (!m_label.isEmpty())
    {
        QFont font;
        font.setPixelSize(14);
        painter->setFont(font);
        painter->setPen(Qt::white);

        QRectF textRect(rect.x() + trackW + 8.0, rect.y(), rect.width() - trackW - 8.0, rect.height());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_label);
    }

    if (selected)
    {
        QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
        selPen.setCosmetic(true);
        painter->setPen(selPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    painter->restore();
    return true;
}

bool ToggleComponent::IsChecked() const noexcept { return m_checked; }
void ToggleComponent::SetChecked(bool v) { if (m_checked == v) return; m_checked = v; NotifyChanged(); }

QColor ToggleComponent::GetOnColor() const noexcept { return m_onColor; }
void ToggleComponent::SetOnColor(const QColor& v) { if (m_onColor == v) return; m_onColor = v; NotifyChanged(); }

QColor ToggleComponent::GetOffColor() const noexcept { return m_offColor; }
void ToggleComponent::SetOffColor(const QColor& v) { if (m_offColor == v) return; m_offColor = v; NotifyChanged(); }

QColor ToggleComponent::GetKnobColor() const noexcept { return m_knobColor; }
void ToggleComponent::SetKnobColor(const QColor& v) { if (m_knobColor == v) return; m_knobColor = v; NotifyChanged(); }

QString ToggleComponent::GetLabel() const noexcept { return m_label; }
void ToggleComponent::SetLabel(const QString& v) { if (m_label == v) return; m_label = v; NotifyChanged(); }

void ToggleComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Toggle";
    out["checked"] = m_checked;
    out["onColor"] = m_onColor.name(QColor::HexArgb);
    out["offColor"] = m_offColor.name(QColor::HexArgb);
    out["knobColor"] = m_knobColor.name(QColor::HexArgb);
    out["label"] = m_label;
}

void ToggleComponent::FromJson(const QJsonObject& in)
{
    SetChecked(in["checked"].toBool(false));
    SetOnColor(QColor(in["onColor"].toString("#FF32C850")));
    SetOffColor(QColor(in["offColor"].toString("#FF50505A")));
    SetKnobColor(QColor(in["knobColor"].toString("#FFFFFFFF")));
    SetLabel(in["label"].toString("Toggle"));
}
