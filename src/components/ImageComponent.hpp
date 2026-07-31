#ifndef COMPONENTS_IMAGECOMPONENT_HPP
#define COMPONENTS_IMAGECOMPONENT_HPP

#include <QString>
#include <QColor>
#include <QPixmap>
#include <QDateTime>

#include "core/Component.hpp"

class ImageComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(QColor tint READ GetTint WRITE SetTint NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)
    Q_PROPERTY(bool pixelated READ IsPixelated WRITE SetPixelated NOTIFY ComponentChanged)

public:

    explicit ImageComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

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

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;


private:

    void ReloadPixmap();

    const QPixmap& EnsureTintedPixmap();

    QString imagePath;
    QColor tint;
    QString assetDomain;
    QString assetRegistryValue;
    bool pixelated;
    QPixmap pixmap;
    QString resolvedPath;
    QDateTime resolvedMtime;
    QPixmap tintedPixmap;
    QColor tintedPixmapColor;

};

#endif
