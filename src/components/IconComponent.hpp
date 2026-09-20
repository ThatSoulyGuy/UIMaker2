#ifndef COMPONENTS_ICONCOMPONENT_HPP
#define COMPONENTS_ICONCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QPixmap>
#include <QDateTime>

#include "core/Component.hpp"
#include "core/PixelDraw.hpp"

class IconComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(QColor tintColor READ GetTintColor WRITE SetTintColor NOTIFY ComponentChanged)
    Q_PROPERTY(int iconSize READ GetIconSize WRITE SetIconSize NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)

    // Pixel-perfect drawing; see core/PixelDraw.hpp. textureFill 0 = Stretch,
    // 1 = Wrap (repeat and crop, one texel per virtual pixel).
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

    explicit IconComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QString GetImagePath() const noexcept;
    void SetImagePath(const QString& v);

    QColor GetTintColor() const noexcept;
    void SetTintColor(const QColor& v);

    int GetIconSize() const noexcept;
    void SetIconSize(int v);

    QString GetAssetDomain() const noexcept;
    void SetAssetDomain(const QString& v);

    QString GetAssetRegistryValue() const noexcept;
    void SetAssetRegistryValue(const QString& v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

    int GetTextureFill() const noexcept;
    void SetTextureFill(int v);

    int GetCropAnchor() const noexcept;
    void SetCropAnchor(int v);

    int GetCropOffsetX() const noexcept;
    void SetCropOffsetX(int v);

    int GetCropOffsetY() const noexcept;
    void SetCropOffsetY(int v);

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

private:

    PixelDraw::Slice slice;


    PixelDraw::TextureParams tex;


    void ReloadPixmap();

    const QPixmap& EnsureTintedPixmap();

    QString m_imagePath;
    QColor m_tintColor;
    int m_iconSize;
    QString m_assetDomain;
    QString m_assetRegistryValue;
    QPixmap m_pixmap;
    QString m_resolvedPath;
    QDateTime m_resolvedMtime;
    QPixmap m_tintedPixmap;
    QColor m_tintedPixmapColor;
};

#endif
