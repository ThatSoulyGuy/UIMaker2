#include "components/DragSlotComponent.hpp"
#include "core/PixelDraw.hpp"
#include "core/SpriteSidecar.hpp"
#include "core/PixelModel.hpp"

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
    painter->setRenderHint(QPainter::Antialiasing, !PixelModel::PixelSnap());
    painter->setRenderHint(QPainter::SmoothPixmapTransform, !PixelModel::PixelSnap());

    QPen borderPen(m_borderColor, 1);
    borderPen.setCosmetic(true);
    painter->setPen(borderPen);
    painter->setBrush(m_backgroundColor);
    painter->drawRoundedRect(rect, PixelDraw::Radius(m_cornerRadius), PixelDraw::Radius(m_cornerRadius));

    if (m_isEmpty)
    {
        // Draw empty slot indicator (inner rect with subtle color)
        QRectF inner = rect.adjusted(4, 4, -4, -4);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_emptyColor);
        painter->drawRoundedRect(inner, PixelDraw::Radius(m_cornerRadius), PixelDraw::Radius(m_cornerRadius));
    }
    else if (!m_iconPixmap.isNull())
    {
        // Draw icon centered with padding
        QRectF iconRect = rect.adjusted(6, 6, -6, -6);
        PixelDraw::DrawTexture(painter, iconRect, m_iconPixmap,
                               slice,
                               tex.anchor, tex.cropOffsetX, tex.cropOffsetY,
                               tex.fill);
    }
    else
    {
        // Filled but no icon - draw a placeholder square
        QRectF inner = rect.adjusted(8, 8, -8, -8);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(120, 100, 60, 150));
        painter->drawRoundedRect(inner, PixelDraw::Radius(2.0), PixelDraw::Radius(2.0));
    }

    if (selected)
    {
        QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
        selPen.setCosmetic(true);
        painter->setPen(selPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rect, PixelDraw::Radius(m_cornerRadius), PixelDraw::Radius(m_cornerRadius));
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
        // Normalise the device pixel ratio to 1 so size() is the true TEXEL count.
        // Qt's @2x convention can hand back a pixmap reporting device pixels - a
        // 200x20 image as 400x40 - which would double every texel index in the
        // wrap/slice path and halve a calibrated unit.
        if (!loaded.isNull())
        loaded.setDevicePixelRatio(1.0);
        if (!loaded.isNull())
            m_iconPixmap = loaded;
    }
    AdoptSidecarSlice(m_iconPath);
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
    out["textureFill"] = tex.fill;
    out["cropAnchor"] = tex.anchor;
    out["cropOffsetX"] = tex.cropOffsetX;
    out["cropOffsetY"] = tex.cropOffsetY;
    out["sliceLeft"] = slice.left;
    out["sliceTop"] = slice.top;
    out["sliceRight"] = slice.right;
    out["sliceBottom"] = slice.bottom;
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
    SetTextureFill(in["textureFill"].toInt(PixelDraw::FillAuto));
    SetCropAnchor(in["cropAnchor"].toInt(PixelDraw::Center));
    SetCropOffsetX(in["cropOffsetX"].toInt(0));
    SetCropOffsetY(in["cropOffsetY"].toInt(0));
    SetSliceLeft(in["sliceLeft"].toInt(0));
    SetSliceTop(in["sliceTop"].toInt(0));
    SetSliceRight(in["sliceRight"].toInt(0));
    SetSliceBottom(in["sliceBottom"].toInt(0));
}

int DragSlotComponent::GetTextureFill() const noexcept { return tex.fill; }

void DragSlotComponent::SetTextureFill(int v)
{
    const int c = PixelDraw::ClampFill(v);
    if (tex.fill == c) return;
    tex.fill = c;
    NotifyChanged();
}

int DragSlotComponent::GetCropAnchor() const noexcept { return tex.anchor; }

void DragSlotComponent::SetCropAnchor(int v)
{
    const int c = PixelDraw::ClampAnchor(v);
    if (tex.anchor == c) return;
    tex.anchor = c;
    NotifyChanged();
}

int DragSlotComponent::GetCropOffsetX() const noexcept { return tex.cropOffsetX; }

void DragSlotComponent::SetCropOffsetX(int v)
{
    if (tex.cropOffsetX == v) return;
    tex.cropOffsetX = v;
    NotifyChanged();
}

int DragSlotComponent::GetCropOffsetY() const noexcept { return tex.cropOffsetY; }

void DragSlotComponent::SetCropOffsetY(int v)
{
    if (tex.cropOffsetY == v) return;
    tex.cropOffsetY = v;
    NotifyChanged();
}

int DragSlotComponent::GetSliceLeft() const noexcept { return slice.left; }
void DragSlotComponent::SetSliceLeft(int v) { if (slice.left == qMax(0, v)) return; slice.left = qMax(0, v); NotifyChanged(); }

int DragSlotComponent::GetSliceTop() const noexcept { return slice.top; }
void DragSlotComponent::SetSliceTop(int v) { if (slice.top == qMax(0, v)) return; slice.top = qMax(0, v); NotifyChanged(); }

int DragSlotComponent::GetSliceRight() const noexcept { return slice.right; }
void DragSlotComponent::SetSliceRight(int v) { if (slice.right == qMax(0, v)) return; slice.right = qMax(0, v); NotifyChanged(); }

int DragSlotComponent::GetSliceBottom() const noexcept { return slice.bottom; }
void DragSlotComponent::SetSliceBottom(int v) { if (slice.bottom == qMax(0, v)) return; slice.bottom = qMax(0, v); NotifyChanged(); }

// The sidecar describes the art, so adopting it on assignment makes the
// properties the single source of truth at paint AND at bake time - the
// sidecar file is not embedded in a .uibin, so anything only it knows would
// be lost to the engine.
void DragSlotComponent::AdoptSidecarSlice(const QString& path)
{
    const SpriteSidecar::Meta meta = SpriteSidecar::MetaFor(path);

    if (meta.hasSlice)
        slice = meta.slice;
}
