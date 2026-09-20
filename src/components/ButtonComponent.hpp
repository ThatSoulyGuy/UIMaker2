#ifndef COMPONENTS_BUTTONCOMPONENT_HPP
#define COMPONENTS_BUTTONCOMPONENT_HPP

#include <QString>
#include <QColor>
#include <QPixmap>
#include <QPointF>

#include "core/Component.hpp"
#include "core/PixelDraw.hpp"

class ButtonComponent : public Component
{
    Q_OBJECT

    // Inspector grouping only; see GroupsFor in ui/PropertyEditorPanel.cpp.
    // Property names are the .uibin field names and are deliberately untouched.
    Q_CLASSINFO("propertyGroup/font", "fontPath=path,fontDomain=domain,fontRegistryValue=registryValue")
    Q_CLASSINFO("propertyGroup/skinImage", "imagePath=path,assetDomain=domain,assetRegistryValue=registryValue")
    Q_CLASSINFO("propertyGroup/slice", "sliceLeft=left,sliceTop=top,sliceRight=right,sliceBottom=bottom")
    Q_CLASSINFO("propertyGroup/textureWrap", "textureFill=fill,cropAnchor=anchor,cropOffsetX=offsetX,cropOffsetY=offsetY")

    Q_PROPERTY(QString text READ GetText WRITE SetText NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontPath READ GetFontPath WRITE SetFontPath NOTIFY ComponentChanged)

    // Engine identity for fontPath specifically. Button is the only component
    // with TWO asset slots, and a single assetDomain/assetRegistryValue pair
    // cannot describe both: the skin's identity used to overwrite the font's.
    Q_PROPERTY(QString fontDomain READ GetFontDomain WRITE SetFontDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontRegistryValue READ GetFontRegistryValue WRITE SetFontRegistryValue NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)
    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceLeft READ GetSliceLeft WRITE SetSliceLeft NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceTop READ GetSliceTop WRITE SetSliceTop NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceRight READ GetSliceRight WRITE SetSliceRight NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceBottom READ GetSliceBottom WRITE SetSliceBottom NOTIFY ComponentChanged)

    // Pixel-perfect drawing; see core/PixelDraw.hpp. textureFill 0 = Stretch,
    // 1 = Wrap (repeat and crop, one texel per virtual pixel).
    Q_PROPERTY(int textureFill READ GetTextureFill WRITE SetTextureFill NOTIFY ComponentChanged)
    Q_PROPERTY(int cropAnchor READ GetCropAnchor WRITE SetCropAnchor NOTIFY ComponentChanged)
    Q_PROPERTY(int cropOffsetX READ GetCropOffsetX WRITE SetCropOffsetX NOTIFY ComponentChanged)
    Q_PROPERTY(int cropOffsetY READ GetCropOffsetY WRITE SetCropOffsetY NOTIFY ComponentChanged)
    Q_PROPERTY(QPointF textOffset READ GetTextOffset WRITE SetTextOffset NOTIFY ComponentChanged)

public:

    explicit ButtonComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QString GetText() const noexcept;
    void SetText(const QString& v);

    QColor GetBackgroundColor() const noexcept;
    void SetBackgroundColor(const QColor& v);

    QColor GetTextColor() const noexcept;
    void SetTextColor(const QColor& v);

    QString GetFontFamily() const noexcept;
    void SetFontFamily(const QString& v);

    int GetPixelSize() const noexcept;
    void SetPixelSize(int v);

    QString GetFontPath() const noexcept;
    void SetFontPath(const QString& v);

    QString GetAssetDomain() const noexcept;
    void SetAssetDomain(const QString& v);

    QString GetAssetRegistryValue() const noexcept;
    void SetAssetRegistryValue(const QString& v);

    QString GetImagePath() const noexcept;
    void SetImagePath(const QString& v);

    int GetSliceLeft() const noexcept;
    int GetSliceTop() const noexcept;
    int GetSliceRight() const noexcept;
    int GetSliceBottom() const noexcept;

    void SetSliceLeft(int v);
    void SetSliceTop(int v);
    void SetSliceRight(int v);
    void SetSliceBottom(int v);

    QString GetFontDomain() const noexcept;
    void SetFontDomain(const QString& v);

    QString GetFontRegistryValue() const noexcept;
    void SetFontRegistryValue(const QString& v);

    // See the other textured components: the sidecar describes the art, so it
    // is adopted into the properties, which are what the engine actually gets.
    void AdoptSidecarSlice(const QString& path);

    // Nudge applied to the glyph box only; see core/TextOffset.hpp.
    QPointF GetTextOffset() const noexcept;
    void SetTextOffset(const QPointF& v);

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

private:

    PixelDraw::TextureParams tex;


    mutable QPixmap defaultSkin;
    mutable QColor defaultSkinColor;

    void InvalidateDefaultSkin();

    const QPixmap& EnsureDefaultSkin() const;

    static void DrawNineSlice(QPainter* painter, const QRectF& dest, const QPixmap& img, int l, int t, int r, int b);

    QString text;
    QColor backgroundColor;
    QColor textColor;
    QString fontFamily;
    int pixelSize;
    QString fontPath;
    QString assetDomain;
    QString assetRegistryValue;
    QString imagePath;
    QPixmap customSkin;
    QString fontDomain;
    QString fontRegistryValue;

    int sliceLeft   = 6;
    int sliceTop    = 6;
    int sliceRight  = 6;
    int sliceBottom = 6;

    QPointF m_textOffset;
};

#endif
