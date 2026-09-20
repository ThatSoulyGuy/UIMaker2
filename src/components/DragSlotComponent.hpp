#ifndef COMPONENTS_DRAGSLOTCOMPONENT_HPP
#define COMPONENTS_DRAGSLOTCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QPixmap>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"
#include "core/PixelDraw.hpp"

class DragSlotComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(int slotSize READ GetSlotSize WRITE SetSlotSize NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor emptyColor READ GetEmptyColor WRITE SetEmptyColor NOTIFY ComponentChanged)
    Q_PROPERTY(double cornerRadius READ GetCornerRadius WRITE SetCornerRadius NOTIFY ComponentChanged)
    Q_PROPERTY(bool isEmpty READ IsEmpty WRITE SetEmpty NOTIFY ComponentChanged)
    Q_PROPERTY(QString iconPath READ GetIconPath WRITE SetIconPath NOTIFY ComponentChanged)
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

    explicit DragSlotComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    int GetSlotSize() const noexcept;
    void SetSlotSize(int v);

    QColor GetBackgroundColor() const noexcept;
    void SetBackgroundColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    QColor GetEmptyColor() const noexcept;
    void SetEmptyColor(const QColor& v);

    double GetCornerRadius() const noexcept;
    void SetCornerRadius(double v);

    bool IsEmpty() const noexcept;
    void SetEmpty(bool v);

    QString GetIconPath() const noexcept;
    void SetIconPath(const QString& v);

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


    int m_slotSize;
    QColor m_backgroundColor;
    QColor m_borderColor;
    QColor m_emptyColor;
    double m_cornerRadius;
    bool m_isEmpty;
    QString m_iconPath;
    QString m_assetDomain;
    QString m_assetRegistryValue;
    QPixmap m_iconPixmap;
};

#endif
