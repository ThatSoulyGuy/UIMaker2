#include "components/IconComponent.hpp"
#include "core/PixelDraw.hpp"
#include "core/SpriteSidecar.hpp"
#include "core/PixelModel.hpp"

#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFileInfo>
#include <algorithm>

#include "core/AssetContext.hpp"

REGISTER_COMPONENT(IconComponent, "Icon")

IconComponent::IconComponent(QObject* parent)
    : Component(parent)
    , m_tintColor(Qt::white)
    , m_iconSize(32)
{ }

QString IconComponent::GetTypeName() const { return QStringLiteral("Icon"); }

void IconComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);
    Q_UNUSED(parentRect);

    // Same reload policy as ImageComponent: re-resolve on asset-root
    // swap, retry failed loads, pick up on-disk replacement.
    if (!m_imagePath.isEmpty())
    {
        const QString nowResolved = AssetContext::Resolve(m_imagePath);

        if (nowResolved != m_resolvedPath || QFileInfo(nowResolved).lastModified() != m_resolvedMtime)
            ReloadPixmap();
    }

    rect = QRectF(0, 0, m_iconSize, m_iconSize);
}

bool IconComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform, !PixelModel::PixelSnap());

    if (!m_pixmap.isNull())
    {
        painter->setOpacity(1.0);
        const QPixmap& drawn = (m_tintColor.isValid() && m_tintColor != QColor(Qt::white)) ? EnsureTintedPixmap() : m_pixmap;
        PixelDraw::DrawTexture(painter, rect, drawn,
                               SpriteSidecar::MetaFor(m_imagePath).slice,
                               tex.anchor, tex.cropOffsetX, tex.cropOffsetY,
                               static_cast<PixelDraw::Fill>(tex.fill));
    }
    else
    {
        // Draw placeholder icon frame
        QPen pen(QColor(150, 150, 160), 1, Qt::DashLine);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->setBrush(QColor(50, 50, 55, 100));
        painter->drawRect(rect);

        painter->setPen(QColor(150, 150, 160));
        QFont font;
        font.setPixelSize(std::max(10, m_iconSize / 3));
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignCenter, "ICO");
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

QString IconComponent::GetImagePath() const noexcept { return m_imagePath; }
void IconComponent::SetImagePath(const QString& v)
{
    if (m_imagePath == v) return;
    m_imagePath = v;
    ReloadPixmap();
    NotifyChanged();
}

QColor IconComponent::GetTintColor() const noexcept { return m_tintColor; }
void IconComponent::SetTintColor(const QColor& v) { if (m_tintColor == v) return; m_tintColor = v; NotifyChanged(); }

int IconComponent::GetIconSize() const noexcept { return m_iconSize; }
void IconComponent::SetIconSize(int v) { if (m_iconSize == v) return; m_iconSize = v; NotifyChanged(); }

QString IconComponent::GetAssetDomain() const noexcept { return m_assetDomain; }
void IconComponent::SetAssetDomain(const QString& v) { if (m_assetDomain == v) return; m_assetDomain = v; NotifyChanged(); }

QString IconComponent::GetAssetRegistryValue() const noexcept { return m_assetRegistryValue; }
void IconComponent::SetAssetRegistryValue(const QString& v) { if (m_assetRegistryValue == v) return; m_assetRegistryValue = v; NotifyChanged(); }

void IconComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Icon";
    out["imagePath"] = m_imagePath;
    out["tintColor"] = m_tintColor.name(QColor::HexArgb);
    out["iconSize"] = m_iconSize;
    out["assetDomain"] = m_assetDomain;
    out["assetRegistryValue"] = m_assetRegistryValue;
    out["textureFill"] = tex.fill;
    out["cropAnchor"] = tex.anchor;
    out["cropOffsetX"] = tex.cropOffsetX;
    out["cropOffsetY"] = tex.cropOffsetY;
}

void IconComponent::FromJson(const QJsonObject& in)
{
    SetImagePath(in["imagePath"].toString());
    SetTintColor(QColor(in["tintColor"].toString("#FFFFFFFF")));
    SetIconSize(in["iconSize"].toInt(32));
    SetAssetDomain(in["assetDomain"].toString());
    SetAssetRegistryValue(in["assetRegistryValue"].toString());
    SetTextureFill(in["textureFill"].toInt(PixelDraw::FillStretch));
    SetCropAnchor(in["cropAnchor"].toInt(PixelDraw::Center));
    SetCropOffsetX(in["cropOffsetX"].toInt(0));
    SetCropOffsetY(in["cropOffsetY"].toInt(0));
}

void IconComponent::ReloadPixmap()
{
    m_pixmap = QPixmap();
    m_tintedPixmap = QPixmap();
    m_resolvedPath.clear();
    m_resolvedMtime = QDateTime();

    if (m_imagePath.isEmpty())
        return;

    const QString candidate = AssetContext::Resolve(m_imagePath);
    QPixmap loaded(candidate);
    // Normalise the device pixel ratio to 1 so size() is the true TEXEL count.
    // Qt's @2x convention can hand back a pixmap reporting device pixels - a
    // 200x20 image as 400x40 - which would double every texel index in the
    // wrap/slice path and halve a calibrated unit.
    if (!loaded.isNull())
    loaded.setDevicePixelRatio(1.0);

    // Only a successful load is cached; a failure leaves m_resolvedPath
    // empty so Update() keeps retrying rather than negative-caching a
    // file that may appear later.
    if (!loaded.isNull())
    {
        m_pixmap = loaded;
        m_resolvedPath = candidate;
        m_resolvedMtime = QFileInfo(candidate).lastModified();
    }
}

// Multiply keeps the icon's shading while scaling each channel by the
// tint (white is a true identity, so the untinted default renders
// unchanged and there is no discontinuity at #FFFFFFFF); DestinationIn
// restores the alpha the opaque fill destroyed. Same scheme as
// ImageComponent.
const QPixmap& IconComponent::EnsureTintedPixmap()
{
    if (!m_tintedPixmap.isNull() && m_tintedPixmapColor == m_tintColor)
        return m_tintedPixmap;

    QPixmap tinted = m_pixmap.copy();

    QPainter p(&tinted);
    p.setCompositionMode(QPainter::CompositionMode_Multiply);
    p.fillRect(tinted.rect(), m_tintColor);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawPixmap(0, 0, m_pixmap);
    p.end();

    m_tintedPixmap = tinted;
    m_tintedPixmapColor = m_tintColor;

    return m_tintedPixmap;
}

int IconComponent::GetTextureFill() const noexcept { return tex.fill; }

void IconComponent::SetTextureFill(int v)
{
    const int c = PixelDraw::ClampFill(v);
    if (tex.fill == c) return;
    tex.fill = c;
    NotifyChanged();
}

int IconComponent::GetCropAnchor() const noexcept { return tex.anchor; }

void IconComponent::SetCropAnchor(int v)
{
    const int c = PixelDraw::ClampAnchor(v);
    if (tex.anchor == c) return;
    tex.anchor = c;
    NotifyChanged();
}

int IconComponent::GetCropOffsetX() const noexcept { return tex.cropOffsetX; }

void IconComponent::SetCropOffsetX(int v)
{
    if (tex.cropOffsetX == v) return;
    tex.cropOffsetX = v;
    NotifyChanged();
}

int IconComponent::GetCropOffsetY() const noexcept { return tex.cropOffsetY; }

void IconComponent::SetCropOffsetY(int v)
{
    if (tex.cropOffsetY == v) return;
    tex.cropOffsetY = v;
    NotifyChanged();
}
