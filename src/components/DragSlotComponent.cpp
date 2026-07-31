#include "components/DragSlotComponent.hpp"

#include <QColor>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>

#include "core/AssetContext.hpp"

REGISTER_COMPONENT(DragSlotComponent, "DragSlot")

DragSlotComponent::DragSlotComponent(QObject* parent)
    : Component(parent)
    , m_slotSize(64)
    , m_backgroundColor(QColor(40, 40, 48))
    , m_borderColor(QColor(80, 80, 95))
    , m_emptyColor(QColor(55, 55, 65, 120))
    , m_cornerRadius(4.0)
    , m_isEmpty(true)
{ }

QString DragSlotComponent::GetTypeName() const { return QStringLiteral("DragSlot"); }

void DragSlotComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);
    Q_UNUSED(parentRect);

    rect = QRectF(0, 0, m_slotSize, m_slotSize);
}

bool DragSlotComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPen borderPen(m_borderColor, 1);
    borderPen.setCosmetic(true);
    painter->setPen(borderPen);
    painter->setBrush(m_backgroundColor);
    painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);

    if (m_isEmpty)
    {
        // Draw empty slot indicator (inner rect with subtle color)
        QRectF inner = rect.adjusted(4, 4, -4, -4);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_emptyColor);
        painter->drawRoundedRect(inner, m_cornerRadius, m_cornerRadius);
    }
    else if (!m_iconPixmap.isNull())
    {
        // Draw icon centered with padding
        QRectF iconRect = rect.adjusted(6, 6, -6, -6);
        painter->drawPixmap(iconRect, m_iconPixmap, QRectF(QPointF(0, 0), QSizeF(m_iconPixmap.size())));
    }
    else
    {
        // Filled but no icon - draw a placeholder square
        QRectF inner = rect.adjusted(8, 8, -8, -8);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(120, 100, 60, 150));
        painter->drawRoundedRect(inner, 2.0, 2.0);
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

int DragSlotComponent::GetSlotSize() const noexcept { return m_slotSize; }
void DragSlotComponent::SetSlotSize(int v) { if (m_slotSize == v) return; m_slotSize = v; NotifyChanged(); }

QColor DragSlotComponent::GetBackgroundColor() const noexcept { return m_backgroundColor; }
void DragSlotComponent::SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

QColor DragSlotComponent::GetBorderColor() const noexcept { return m_borderColor; }
void DragSlotComponent::SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

QColor DragSlotComponent::GetEmptyColor() const noexcept { return m_emptyColor; }
void DragSlotComponent::SetEmptyColor(const QColor& v) { if (m_emptyColor == v) return; m_emptyColor = v; NotifyChanged(); }

double DragSlotComponent::GetCornerRadius() const noexcept { return m_cornerRadius; }
void DragSlotComponent::SetCornerRadius(double v) { if (m_cornerRadius == v) return; m_cornerRadius = v; NotifyChanged(); }

bool DragSlotComponent::IsEmpty() const noexcept { return m_isEmpty; }
void DragSlotComponent::SetEmpty(bool v) { if (m_isEmpty == v) return; m_isEmpty = v; NotifyChanged(); }

QString DragSlotComponent::GetIconPath() const noexcept { return m_iconPath; }
void DragSlotComponent::SetIconPath(const QString& v)
{
    if (m_iconPath == v) return;
    m_iconPath = v;
    m_iconPixmap = QPixmap();
    if (!m_iconPath.isEmpty())
    {
        QPixmap loaded(AssetContext::Resolve(m_iconPath));
        if (!loaded.isNull())
            m_iconPixmap = loaded;
    }
    NotifyChanged();
}

QString DragSlotComponent::GetAssetDomain() const noexcept { return m_assetDomain; }
void DragSlotComponent::SetAssetDomain(const QString& v) { if (m_assetDomain == v) return; m_assetDomain = v; NotifyChanged(); }

QString DragSlotComponent::GetAssetRegistryValue() const noexcept { return m_assetRegistryValue; }
void DragSlotComponent::SetAssetRegistryValue(const QString& v) { if (m_assetRegistryValue == v) return; m_assetRegistryValue = v; NotifyChanged(); }

void DragSlotComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "DragSlot";
    out["slotSize"] = m_slotSize;
    out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
    out["borderColor"] = m_borderColor.name(QColor::HexArgb);
    out["emptyColor"] = m_emptyColor.name(QColor::HexArgb);
    out["cornerRadius"] = m_cornerRadius;
    out["isEmpty"] = m_isEmpty;
    out["iconPath"] = m_iconPath;
    out["assetDomain"] = m_assetDomain;
    out["assetRegistryValue"] = m_assetRegistryValue;
}

void DragSlotComponent::FromJson(const QJsonObject& in)
{
    SetSlotSize(in["slotSize"].toInt(64));
    SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF282830")));
    SetBorderColor(QColor(in["borderColor"].toString("#FF50505F")));
    SetEmptyColor(QColor(in["emptyColor"].toString("#78373741")));
    SetCornerRadius(in["cornerRadius"].toDouble(4.0));
    SetEmpty(in["isEmpty"].toBool(true));
    SetIconPath(in["iconPath"].toString());
    SetAssetDomain(in["assetDomain"].toString());
    SetAssetRegistryValue(in["assetRegistryValue"].toString());
}
