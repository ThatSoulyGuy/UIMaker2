#include "components/ImageComponent.hpp"

#include <QPainter>
#include <QPen>
#include <QFileInfo>

#include "core/AssetContext.hpp"
#include "core/PixelDraw.hpp"
#include "core/PixelModel.hpp"
#include "core/SpriteSidecar.hpp"

REGISTER_COMPONENT(ImageComponent, "Image")

ImageComponent::ImageComponent(QObject* parent) : Component(parent), tint(Qt::white), pixelated(false) { }

QString ImageComponent::GetTypeName() const
{
    return QStringLiteral("Image");
}

void ImageComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);
    Q_UNUSED(parentRect);

    // Reload when the resolved location changes (asset root swap), when a
    // previous load failed (the file may have appeared since), or when the
    // file on disk was replaced. A stat per pass is cheap - the old
    // per-pass full decode was the lag source, not the stat.
    if (!imagePath.isEmpty())
    {
        const QString nowResolved = AssetContext::Resolve(imagePath);

        if (nowResolved != resolvedPath || QFileInfo(nowResolved).lastModified() != resolvedMtime)
            ReloadPixmap();
    }

    if (!pixmap.isNull())
        rect = QRectF(QPointF(0.0, 0.0), pixmap.size());
}

bool ImageComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    if (pixmap.isNull())
        return false;

    painter->save();

    // PixelGrid means pixel-art: nearest-neighbour regardless of the per-element
    // flag, which only governs Continuous mode.
    const bool crisp = pixelated || PixelModel::PixelSnap();
    painter->setRenderHint(QPainter::SmoothPixmapTransform, !crisp);

    const QPixmap& drawn = (tint.isValid() && tint != QColor(Qt::white)) ? EnsureTintedPixmap() : pixmap;

    // The tinted copy is pixel-identical in size to the source, so slice and
    // crop texel indices are valid against either.
    const SpriteSidecar::Meta meta = SpriteSidecar::MetaFor(imagePath);

    PixelDraw::DrawTexture(painter, rect, drawn,
                           meta.slice, tex.anchor, tex.cropOffsetX, tex.cropOffsetY,
                           static_cast<PixelDraw::Fill>(tex.fill));

    if (selected)
    {
        painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    painter->restore();

    return true;
}

int ImageComponent::GetTextureFill() const noexcept { return tex.fill; }

void ImageComponent::SetTextureFill(int v)
{
    const int c = PixelDraw::ClampFill(v);

    if (tex.fill == c)
        return;

    tex.fill = c;
    NotifyChanged();
}

int ImageComponent::GetCropAnchor() const noexcept { return tex.anchor; }

void ImageComponent::SetCropAnchor(int v)
{
    const int c = PixelDraw::ClampAnchor(v);

    if (tex.anchor == c)
        return;

    tex.anchor = c;
    NotifyChanged();
}

int ImageComponent::GetCropOffsetX() const noexcept { return tex.cropOffsetX; }

void ImageComponent::SetCropOffsetX(int v)
{
    if (tex.cropOffsetX == v)
        return;

    tex.cropOffsetX = v;
    NotifyChanged();
}

int ImageComponent::GetCropOffsetY() const noexcept { return tex.cropOffsetY; }

void ImageComponent::SetCropOffsetY(int v)
{
    if (tex.cropOffsetY == v)
        return;

    tex.cropOffsetY = v;
    NotifyChanged();
}

QString ImageComponent::GetImagePath() const noexcept
{
    return imagePath;
}

void ImageComponent::SetImagePath(const QString& v)
{
    if (imagePath == v)
        return;

    imagePath = v;

    ReloadPixmap();

    NotifyChanged();
}

QString ImageComponent::GetAssetDomain() const noexcept
{
    return assetDomain;
}

void ImageComponent::SetAssetDomain(const QString& v)
{
    if (assetDomain == v)
        return;

    assetDomain = v;

    NotifyChanged();
}

QString ImageComponent::GetAssetRegistryValue() const noexcept
{
    return assetRegistryValue;
}

void ImageComponent::SetAssetRegistryValue(const QString& v)
{
    if (assetRegistryValue == v)
        return;

    assetRegistryValue = v;

    NotifyChanged();
}

QColor ImageComponent::GetTint() const noexcept
{
    return tint;
}

void ImageComponent::SetTint(const QColor& v)
{
    if (tint == v)
        return;

    tint = v;

    NotifyChanged();
}

bool ImageComponent::IsPixelated() const noexcept
{
    return pixelated;
}

QSize ImageComponent::GetTextureSize() const
{
    return pixmap.size();
}

void ImageComponent::SetPixelated(bool v)
{
    if (pixelated == v)
        return;

    pixelated = v;

    NotifyChanged();
}

void ImageComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Image";
    out["imagePath"] = imagePath;
    out["tint"] = tint.name(QColor::HexArgb);
    out["assetDomain"] = assetDomain;
    out["assetRegistryValue"] = assetRegistryValue;
    out["pixelated"] = pixelated;
    out["textureFill"] = tex.fill;
    out["cropAnchor"] = tex.anchor;
    out["cropOffsetX"] = tex.cropOffsetX;
    out["cropOffsetY"] = tex.cropOffsetY;
}

void ImageComponent::FromJson(const QJsonObject& in)
{
    SetImagePath(in["imagePath"].toString());
    SetTint(QColor(in["tint"].toString("#FFFFFFFF")));
    SetAssetDomain(in["assetDomain"].toString());
    SetAssetRegistryValue(in["assetRegistryValue"].toString());
    SetPixelated(in["pixelated"].toBool(false));
    SetTextureFill(in["textureFill"].toInt(PixelDraw::FillStretch));
    SetCropAnchor(in["cropAnchor"].toInt(PixelDraw::Center));
    SetCropOffsetX(in["cropOffsetX"].toInt(0));
    SetCropOffsetY(in["cropOffsetY"].toInt(0));
}

void ImageComponent::ReloadPixmap()
{
    pixmap = QPixmap();
    tintedPixmap = QPixmap();
    resolvedPath.clear();
    resolvedMtime = QDateTime();

    if (imagePath.isEmpty())
        return;

    const QString candidate = AssetContext::Resolve(imagePath);
    QPixmap loaded(candidate);
    // Normalise the device pixel ratio to 1 so size() is the true TEXEL count.
    // Qt's @2x convention can hand back a pixmap reporting device pixels - a
    // 200x20 image as 400x40 - which would double every texel index in the
    // wrap/slice path and halve a calibrated unit.
    if (!loaded.isNull())
    loaded.setDevicePixelRatio(1.0);

    // Only a successful load is cached; a failure leaves resolvedPath
    // empty so Update() keeps retrying rather than negative-caching a
    // file that may appear later.
    if (!loaded.isNull())
    {
        pixmap = loaded;
        resolvedPath = candidate;
        resolvedMtime = QFileInfo(candidate).lastModified();
    }
}

// Multiply keeps the image's shading while scaling each channel by the tint;
// DestinationIn restores the alpha the opaque fill destroyed.
const QPixmap& ImageComponent::EnsureTintedPixmap()
{
    if (!tintedPixmap.isNull() && tintedPixmapColor == tint)
        return tintedPixmap;

    QPixmap tinted = pixmap.copy();

    QPainter p(&tinted);
    p.setCompositionMode(QPainter::CompositionMode_Multiply);
    p.fillRect(tinted.rect(), tint);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawPixmap(0, 0, pixmap);
    p.end();

    tintedPixmap = tinted;
    tintedPixmapColor = tint;

    return tintedPixmap;
}
