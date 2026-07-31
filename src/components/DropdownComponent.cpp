#include "components/DropdownComponent.hpp"

#include <algorithm>

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QStringList>

REGISTER_COMPONENT(DropdownComponent, "Dropdown")

DropdownComponent::DropdownComponent(QObject* parent)
    : Component(parent)
    , m_options("Option A,Option B,Option C")
    , m_selectedIndex(0)
    , m_backgroundColor(QColor(45, 45, 50))
    , m_textColor(Qt::white)
    , m_borderColor(QColor(90, 90, 100))
    , m_fontFamily("Inter")
    , m_pixelSize(16) { }

QString DropdownComponent::GetTypeName() const { return QStringLiteral("Dropdown"); }

void DropdownComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);
    Q_UNUSED(parentRect);

    QFont font(m_fontFamily);
    font.setPixelSize(m_pixelSize);
    QFontMetrics fm(font);

    QStringList items = m_options.split(',', Qt::SkipEmptyParts);
    double maxW = 80.0;
    for (const QString& s : items)
        maxW = std::max(maxW, (double)fm.horizontalAdvance(s.trimmed()));

    rect = QRectF(0, 0, maxW + 40.0, fm.height() + 16.0);
}

bool DropdownComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen borderPen(m_borderColor, 1);
    borderPen.setCosmetic(true);
    painter->setPen(borderPen);
    painter->setBrush(m_backgroundColor);
    painter->drawRoundedRect(rect, 4.0, 4.0);

    QFont font(m_fontFamily);
    font.setPixelSize(m_pixelSize);
    painter->setFont(font);
    painter->setPen(m_textColor);

    QStringList items = m_options.split(',', Qt::SkipEmptyParts);
    QString displayText = (m_selectedIndex >= 0 && m_selectedIndex < items.size())
        ? items[m_selectedIndex].trimmed() : "Select...";

    QRectF textRect = rect.adjusted(8, 0, -24, 0);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, displayText);

    // Draw dropdown arrow
    double arrowX = rect.right() - 18.0;
    double arrowY = rect.center().y();
    QPolygonF arrow;
    arrow << QPointF(arrowX - 4, arrowY - 3)
          << QPointF(arrowX + 4, arrowY - 3)
          << QPointF(arrowX, arrowY + 3);
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_textColor);
    painter->drawPolygon(arrow);

    if (selected)
    {
        QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
        selPen.setCosmetic(true);
        painter->setPen(selPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rect, 4.0, 4.0);
    }

    painter->restore();
    return true;
}

QString DropdownComponent::GetOptions() const noexcept { return m_options; }
void DropdownComponent::SetOptions(const QString& v) { if (m_options == v) return; m_options = v; NotifyChanged(); }

int DropdownComponent::GetSelectedIndex() const noexcept { return m_selectedIndex; }
void DropdownComponent::SetSelectedIndex(int v) { if (m_selectedIndex == v) return; m_selectedIndex = v; NotifyChanged(); }

QColor DropdownComponent::GetBackgroundColor() const noexcept { return m_backgroundColor; }
void DropdownComponent::SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

QColor DropdownComponent::GetTextColor() const noexcept { return m_textColor; }
void DropdownComponent::SetTextColor(const QColor& v) { if (m_textColor == v) return; m_textColor = v; NotifyChanged(); }

QColor DropdownComponent::GetBorderColor() const noexcept { return m_borderColor; }
void DropdownComponent::SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

QString DropdownComponent::GetFontFamily() const noexcept { return m_fontFamily; }
void DropdownComponent::SetFontFamily(const QString& v) { if (m_fontFamily == v) return; m_fontFamily = v; NotifyChanged(); }

int DropdownComponent::GetPixelSize() const noexcept { return m_pixelSize; }
void DropdownComponent::SetPixelSize(int v) { if (m_pixelSize == v) return; m_pixelSize = v; NotifyChanged(); }

void DropdownComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Dropdown";
    out["options"] = m_options;
    out["selectedIndex"] = m_selectedIndex;
    out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
    out["textColor"] = m_textColor.name(QColor::HexArgb);
    out["borderColor"] = m_borderColor.name(QColor::HexArgb);
    out["fontFamily"] = m_fontFamily;
    out["pixelSize"] = m_pixelSize;
}

void DropdownComponent::FromJson(const QJsonObject& in)
{
    SetOptions(in["options"].toString("Option A,Option B,Option C"));
    SetSelectedIndex(in["selectedIndex"].toInt(0));
    SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF2D2D32")));
    SetTextColor(QColor(in["textColor"].toString("#FFFFFFFF")));
    SetBorderColor(QColor(in["borderColor"].toString("#FF5A5A64")));
    SetFontFamily(in["fontFamily"].toString("Inter"));
    SetPixelSize(in["pixelSize"].toInt(16));
}
