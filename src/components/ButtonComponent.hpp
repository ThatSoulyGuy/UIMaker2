#ifndef COMPONENTS_BUTTONCOMPONENT_HPP
#define COMPONENTS_BUTTONCOMPONENT_HPP

#include <QString>
#include <QColor>
#include <QPixmap>

#include "core/Component.hpp"

class ButtonComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString text READ GetText WRITE SetText NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontPath READ GetFontPath WRITE SetFontPath NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)
    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceLeft READ GetSliceLeft WRITE SetSliceLeft NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceTop READ GetSliceTop WRITE SetSliceTop NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceRight READ GetSliceRight WRITE SetSliceRight NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceBottom READ GetSliceBottom WRITE SetSliceBottom NOTIFY ComponentChanged)

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

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

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
    int sliceLeft   = 6;
    int sliceTop    = 6;
    int sliceRight  = 6;
    int sliceBottom = 6;

};

#endif
