#ifndef COMPONENTS_IMAGECOMPONENT_HPP
#define COMPONENTS_IMAGECOMPONENT_HPP

#include <QString>
#include <QColor>
#include <QPixmap>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"
#include "core/AssetContext.hpp"

class ImageComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(QColor tint READ GetTint WRITE SetTint NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)
    Q_PROPERTY(bool pixelated READ IsPixelated WRITE SetPixelated NOTIFY ComponentChanged)

public:

    explicit ImageComponent(QObject* parent = nullptr) : Component(parent), tint(Qt::white), pixelated(false) { }

    QString GetTypeName() const override
    {
        return QStringLiteral("Image");
    }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        if (!imagePath.isEmpty())
        {
            QPixmap loaded(AssetContext::Resolve(imagePath));

            if (!loaded.isNull())
            {
                pixmap = loaded;
                rect = QRectF(QPointF(0.0, 0.0), loaded.size());
            }
        }
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        if (pixmap.isNull())
            return false;

        painter->save();
        painter->setOpacity(1.0);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, !pixelated);
        painter->drawPixmap(rect, pixmap, QRectF(QPointF(0.0, 0.0), QSizeF(pixmap.width(), pixmap.height())));

        if (selected)
        {
            painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(rect);
        }

        painter->restore();

        return true;
    }

    QString GetImagePath() const noexcept
    {
        return imagePath;
    }

    void SetImagePath(const QString& v)
    {
        if (imagePath == v)
            return;

        imagePath = v;

        NotifyChanged();
    }

    QString GetAssetDomain() const noexcept
    {
        return assetDomain;
    }

    void SetAssetDomain(const QString& v)
    {
        if (assetDomain == v)
            return;

        assetDomain = v;

        NotifyChanged();
    }

    QString GetAssetRegistryValue() const noexcept
    {
        return assetRegistryValue;
    }

    void SetAssetRegistryValue(const QString& v)
    {
        if (assetRegistryValue == v)
            return;

        assetRegistryValue = v;

        NotifyChanged();
    }

    QColor GetTint() const noexcept
    {
        return tint;
    }

    void SetTint(const QColor& v)
    {
        if (tint == v)
            return;

        tint = v;

        NotifyChanged();
    }

    bool IsPixelated() const noexcept
    {
        return pixelated;
    }

    void SetPixelated(bool v)
    {
        if (pixelated == v)
            return;

        pixelated = v;

        NotifyChanged();
    }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Image";
        out["imagePath"] = imagePath;
        out["tint"] = tint.name(QColor::HexArgb);
        out["assetDomain"] = assetDomain;
        out["assetRegistryValue"] = assetRegistryValue;
        out["pixelated"] = pixelated;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetImagePath(in["imagePath"].toString());
        SetTint(QColor(in["tint"].toString("#FFFFFFFF")));
        SetAssetDomain(in["assetDomain"].toString());
        SetAssetRegistryValue(in["assetRegistryValue"].toString());
        SetPixelated(in["pixelated"].toBool(false));
    }


private:

    QString imagePath;
    QColor tint;
    QString assetDomain;
    QString assetRegistryValue;
    bool pixelated;
    QPixmap pixmap;

};

#endif
