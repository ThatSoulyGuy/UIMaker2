#ifndef COMPONENTS_ICONCOMPONENT_HPP
#define COMPONENTS_ICONCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QPixmap>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"

class IconComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(QColor tintColor READ GetTintColor WRITE SetTintColor NOTIFY ComponentChanged)
    Q_PROPERTY(int iconSize READ GetIconSize WRITE SetIconSize NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetPath READ GetAssetPath WRITE SetAssetPath NOTIFY ComponentChanged)

public:

    explicit IconComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_tintColor(Qt::white)
        , m_iconSize(32)
    { }

    QString GetTypeName() const override { return QStringLiteral("Icon"); }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        rect = QRectF(0, 0, m_iconSize, m_iconSize);
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

        if (!m_pixmap.isNull())
        {
            painter->setOpacity(1.0);
            painter->drawPixmap(rect, m_pixmap, QRectF(QPointF(0, 0), QSizeF(m_pixmap.size())));
        }
        else
        {
            // Draw placeholder icon frame
            QPen pen(QColor(150, 150, 160), 1, Qt::DashLine);
            pen.setCosmetic(true);
            painter->setPen(pen);
            painter->setBrush(QColor(50, 50, 55, 100));
            painter->drawRect(rect);

            painter->setPen(QColor(150, 150, 160));
            QFont font;
            font.setPixelSize(std::max(10, m_iconSize / 3));
            painter->setFont(font);
            painter->drawText(rect, Qt::AlignCenter, "ICO");
        }

        if (selected)
        {
            QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
            selPen.setCosmetic(true);
            painter->setPen(selPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(rect);
        }

        painter->restore();
        return true;
    }

    QString GetImagePath() const noexcept { return m_imagePath; }
    void SetImagePath(const QString& v)
    {
        if (m_imagePath == v) return;
        m_imagePath = v;
        m_pixmap = QPixmap();
        if (!m_imagePath.isEmpty())
        {
            QPixmap loaded(m_imagePath);
            if (!loaded.isNull())
                m_pixmap = loaded;
        }
        NotifyChanged();
    }

    QColor GetTintColor() const noexcept { return m_tintColor; }
    void SetTintColor(const QColor& v) { if (m_tintColor == v) return; m_tintColor = v; NotifyChanged(); }

    int GetIconSize() const noexcept { return m_iconSize; }
    void SetIconSize(int v) { if (m_iconSize == v) return; m_iconSize = v; NotifyChanged(); }

    QString GetAssetPath() const noexcept { return m_assetPath; }
    void SetAssetPath(const QString& v) { if (m_assetPath == v) return; m_assetPath = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Icon";
        out["imagePath"] = m_imagePath;
        out["tintColor"] = m_tintColor.name(QColor::HexArgb);
        out["iconSize"] = m_iconSize;
        out["assetPath"] = m_assetPath;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetImagePath(in["imagePath"].toString());
        SetTintColor(QColor(in["tintColor"].toString("#FFFFFFFF")));
        SetIconSize(in["iconSize"].toInt(32));
        SetAssetPath(in["assetPath"].toString());
    }

private:

    QString m_imagePath;
    QColor m_tintColor;
    int m_iconSize;
    QString m_assetPath;
    QPixmap m_pixmap;
};

#endif
