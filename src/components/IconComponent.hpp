#ifndef COMPONENTS_ICONCOMPONENT_HPP
#define COMPONENTS_ICONCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QPixmap>
#include <QDateTime>

#include "core/Component.hpp"

class IconComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(QColor tintColor READ GetTintColor WRITE SetTintColor NOTIFY ComponentChanged)
    Q_PROPERTY(int iconSize READ GetIconSize WRITE SetIconSize NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)

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

private:

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
