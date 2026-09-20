#ifndef COMPONENTS_IMAGECOMPONENT_HPP
#define COMPONENTS_IMAGECOMPONENT_HPP

#include <QString>
#include <QColor>
#include <QPixmap>
#include <QSize>
#include <QDateTime>

#include "core/Component.hpp"
#include "core/PixelDraw.hpp"

class ImageComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(QColor tint READ GetTint WRITE SetTint NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)
    Q_PROPERTY(bool pixelated READ IsPixelated WRITE SetPixelated NOTIFY ComponentChanged)

    // Pixel-perfect drawing. textureFill 0 = Stretch (scale to fit, the
    // pre-existing behaviour), 1 = Wrap (repeat and crop, one texel per virtual
    // pixel). cropAnchor 0..8 reads top-left..bottom-right and decides which
    // part survives a crop / where the tiling phase starts.
    Q_PROPERTY(int textureFill READ GetTextureFill WRITE SetTextureFill NOTIFY ComponentChanged)
    Q_PROPERTY(int cropAnchor READ GetCropAnchor WRITE SetCropAnchor NOTIFY ComponentChanged)
    Q_PROPERTY(int cropOffsetX READ GetCropOffsetX WRITE SetCropOffsetX NOTIFY ComponentChanged)
    Q_PROPERTY(int cropOffsetY READ GetCropOffsetY WRITE SetCropOffsetY NOTIFY ComponentChanged)

    // 9-slice insets in TEXELS. Populated from the sidecar .xml when the image
    // path is set, and editable afterwards. These are what reaches the engine:
    // the sidecar file itself is not embedded in a bake.
    Q_PROPERTY(int sliceLeft READ GetSliceLeft WRITE SetSliceLeft NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceTop READ GetSliceTop WRITE SetSliceTop NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceRight READ GetSliceRight WRITE SetSliceRight NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceBottom READ GetSliceBottom WRITE SetSliceBottom NOTIFY ComponentChanged)

public:

    explicit ImageComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    int GetTextureFill() const noexcept;
    void SetTextureFill(int v);

    int GetCropAnchor() const noexcept;
    void SetCropAnchor(int v);

    int GetCropOffsetX() const noexcept;
    void SetCropOffsetX(int v);

    int GetCropOffsetY() const noexcept;
    void SetCropOffsetY(int v);

    QString GetImagePath() const noexcept;

    void SetImagePath(const QString& v);

    QString GetAssetDomain() const noexcept;

    void SetAssetDomain(const QString& v);

    QString GetAssetRegistryValue() const noexcept;

    void SetAssetRegistryValue(const QString& v);

    QColor GetTint() const noexcept;

    void SetTint(const QColor& v);

    bool IsPixelated() const noexcept;

    void SetPixelated(bool v);

    // Native pixel resolution of the loaded texture (a null size if none is
    // loaded). Used by the pixel-unit "calibrate to resolution" flow.
    QSize GetTextureSize() const;

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;


    int GetSliceLeft() const noexcept;
    void SetSliceLeft(int v);

    int GetSliceTop() const noexcept;
    void SetSliceTop(int v);

    int GetSliceRight() const noexcept;
    void SetSliceRight(int v);

    int GetSliceBottom() const noexcept;
    void SetSliceBottom(int v);

    // Pull the slice from the sidecar next to `path`, if one exists.
    void AdoptSidecarSlice(const QString& path);

    // If the owning element is still at the factory default size, resize it to
    // show this texture 1:1. Only fires on the untouched default, so it never
    // overrides a size the user chose.
    void SizeOwnerToTexture();

private:

    PixelDraw::Slice slice;


    void ReloadPixmap();

    const QPixmap& EnsureTintedPixmap();

    QString imagePath;
    QColor tint;
    QString assetDomain;
    QString assetRegistryValue;
    bool pixelated;
    QPixmap pixmap;
    QString resolvedPath;
    PixelDraw::TextureParams tex;

    QDateTime resolvedMtime;
    QPixmap tintedPixmap;
    QColor tintedPixmapColor;

};

#endif
