#include "components/ImageComponent.hpp"

#include <QPainter>
#include <QPen>
#include <QFileInfo>

#include "core/AssetContext.hpp"
#include "core/PixelDraw.hpp"
#include "core/PixelModel.hpp"
#include "core/SpriteSidecar.hpp"
#include "core/UiElement.hpp"
#include "components/TransformComponent.hpp"

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
        rect = QRectF(QPointF(0.0, 0.0), PixelDraw::NaturalSize(pixmap.size()));
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
    PixelDraw::DrawTexture(painter, rect, drawn,
                           slice, tex.anchor, tex.cropOffsetX, tex.cropOffsetY,
                           tex.fill);

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
    AdoptSidecarSlice(imagePath);
    SizeOwnerToTexture();

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
    out["sliceLeft"] = slice.left;
    out["sliceTop"] = slice.top;
    out["sliceRight"] = slice.right;
    out["sliceBottom"] = slice.bottom;
}

void ImageComponent::FromJson(const QJsonObject& in)
{
    SetImagePath(in["imagePath"].toString());
    SetTint(QColor(in["tint"].toString("#FFFFFFFF")));
    SetAssetDomain(in["assetDomain"].toString());
    SetAssetRegistryValue(in["assetRegistryValue"].toString());
    SetPixelated(in["pixelated"].toBool(false));
    SetTextureFill(in["textureFill"].toInt(PixelDraw::FillAuto));
    SetCropAnchor(in["cropAnchor"].toInt(PixelDraw::Center));
    SetCropOffsetX(in["cropOffsetX"].toInt(0));
    SetCropOffsetY(in["cropOffsetY"].toInt(0));
    SetSliceLeft(in["sliceLeft"].toInt(0));
    SetSliceTop(in["sliceTop"].toInt(0));
    SetSliceRight(in["sliceRight"].toInt(0));
    SetSliceBottom(in["sliceBottom"].toInt(0));
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

int ImageComponent::GetSliceLeft() const noexcept { return slice.left; }
void ImageComponent::SetSliceLeft(int v) { if (slice.left == qMax(0, v)) return; slice.left = qMax(0, v); NotifyChanged(); }

int ImageComponent::GetSliceTop() const noexcept { return slice.top; }
void ImageComponent::SetSliceTop(int v) { if (slice.top == qMax(0, v)) return; slice.top = qMax(0, v); NotifyChanged(); }

int ImageComponent::GetSliceRight() const noexcept { return slice.right; }
void ImageComponent::SetSliceRight(int v) { if (slice.right == qMax(0, v)) return; slice.right = qMax(0, v); NotifyChanged(); }

int ImageComponent::GetSliceBottom() const noexcept { return slice.bottom; }
void ImageComponent::SetSliceBottom(int v) { if (slice.bottom == qMax(0, v)) return; slice.bottom = qMax(0, v); NotifyChanged(); }

// The sidecar describes the art, so adopting it on assignment makes the
// properties the single source of truth at paint AND at bake time - the
// sidecar file is not embedded in a .uibin, so anything only it knows would
// be lost to the engine.
void ImageComponent::AdoptSidecarSlice(const QString& path)
{
    const SpriteSidecar::Meta meta = SpriteSidecar::MetaFor(path);

    if (meta.hasSlice)
        slice = meta.slice;
}

void ImageComponent::SizeOwnerToTexture()
{
    if (pixmap.isNull())
        return;

    auto* owner = qobject_cast<UiElement*>(parent());

    if (!owner)
        return;

    auto* xform = owner->GetComponent<TransformComponent>();

    if (!xform)
        return;

    // TransformComponent's factory default - or that default already rounded
    // to the grid, because in PixelGrid mode the first refresh writes the
    // snapped size back to scale before any image is assigned. Comparing only
    // against the raw default therefore never matched. Anything else is a size
    // the user chose and must be left alone.
    const QPointF current = xform->GetScale();

    const bool untouched =
        (qFuzzyCompare(current.x(), 100.0) && qFuzzyCompare(current.y(), 100.0))
     || (qFuzzyCompare(current.x(), PixelDraw::SnapLength(100.0))
         && qFuzzyCompare(current.y(), PixelDraw::SnapLength(100.0)));

    if (!untouched)
        return;

    const QSizeF natural = PixelDraw::NaturalSize(pixmap.size());

    xform->SetScale(QPointF(natural.width(), natural.height()));
}
