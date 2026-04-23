#ifndef COMPONENTS_DRAGSLOTCOMPONENT_HPP
#define COMPONENTS_DRAGSLOTCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QPixmap>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"

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
    Q_PROPERTY(QString assetPath READ GetAssetPath WRITE SetAssetPath NOTIFY ComponentChanged)

public:

    explicit DragSlotComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_slotSize(64)
        , m_backgroundColor(QColor(40, 40, 48))
        , m_borderColor(QColor(80, 80, 95))
        , m_emptyColor(QColor(55, 55, 65, 120))
        , m_cornerRadius(4.0)
        , m_isEmpty(true)
    { }

    QString GetTypeName() const override { return QStringLiteral("DragSlot"); }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        rect = QRectF(0, 0, m_slotSize, m_slotSize);
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

        QPen borderPen(m_borderColor, 1);
        borderPen.setCosmetic(true);
        painter->setPen(borderPen);
        painter->setBrush(m_backgroundColor);
        painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);

        if (m_isEmpty)
        {
            // Draw empty slot indicator (inner rect with subtle color)
            QRectF inner = rect.adjusted(4, 4, -4, -4);
            painter->setPen(Qt::NoPen);
            painter->setBrush(m_emptyColor);
            painter->drawRoundedRect(inner, m_cornerRadius, m_cornerRadius);
        }
        else if (!m_iconPixmap.isNull())
        {
            // Draw icon centered with padding
            QRectF iconRect = rect.adjusted(6, 6, -6, -6);
            painter->drawPixmap(iconRect, m_iconPixmap, QRectF(QPointF(0, 0), QSizeF(m_iconPixmap.size())));
        }
        else
        {
            // Filled but no icon - draw a placeholder square
            QRectF inner = rect.adjusted(8, 8, -8, -8);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(120, 100, 60, 150));
            painter->drawRoundedRect(inner, 2.0, 2.0);
        }

        if (selected)
        {
            QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
            selPen.setCosmetic(true);
            painter->setPen(selPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);
        }

        painter->restore();
        return true;
    }

    int GetSlotSize() const noexcept { return m_slotSize; }
    void SetSlotSize(int v) { if (m_slotSize == v) return; m_slotSize = v; NotifyChanged(); }

    QColor GetBackgroundColor() const noexcept { return m_backgroundColor; }
    void SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

    QColor GetBorderColor() const noexcept { return m_borderColor; }
    void SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

    QColor GetEmptyColor() const noexcept { return m_emptyColor; }
    void SetEmptyColor(const QColor& v) { if (m_emptyColor == v) return; m_emptyColor = v; NotifyChanged(); }

    double GetCornerRadius() const noexcept { return m_cornerRadius; }
    void SetCornerRadius(double v) { if (m_cornerRadius == v) return; m_cornerRadius = v; NotifyChanged(); }

    bool IsEmpty() const noexcept { return m_isEmpty; }
    void SetEmpty(bool v) { if (m_isEmpty == v) return; m_isEmpty = v; NotifyChanged(); }

    QString GetIconPath() const noexcept { return m_iconPath; }
    void SetIconPath(const QString& v)
    {
        if (m_iconPath == v) return;
        m_iconPath = v;
        m_iconPixmap = QPixmap();
        if (!m_iconPath.isEmpty())
        {
            QPixmap loaded(m_iconPath);
            if (!loaded.isNull())
                m_iconPixmap = loaded;
        }
        NotifyChanged();
    }

    QString GetAssetPath() const noexcept { return m_assetPath; }
    void SetAssetPath(const QString& v) { if (m_assetPath == v) return; m_assetPath = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "DragSlot";
        out["slotSize"] = m_slotSize;
        out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
        out["borderColor"] = m_borderColor.name(QColor::HexArgb);
        out["emptyColor"] = m_emptyColor.name(QColor::HexArgb);
        out["cornerRadius"] = m_cornerRadius;
        out["isEmpty"] = m_isEmpty;
        out["iconPath"] = m_iconPath;
        out["assetPath"] = m_assetPath;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetSlotSize(in["slotSize"].toInt(64));
        SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF282830")));
        SetBorderColor(QColor(in["borderColor"].toString("#FF50505F")));
        SetEmptyColor(QColor(in["emptyColor"].toString("#78373741")));
        SetCornerRadius(in["cornerRadius"].toDouble(4.0));
        SetEmpty(in["isEmpty"].toBool(true));
        SetIconPath(in["iconPath"].toString());
        SetAssetPath(in["assetPath"].toString());
    }

private:

    int m_slotSize;
    QColor m_backgroundColor;
    QColor m_borderColor;
    QColor m_emptyColor;
    double m_cornerRadius;
    bool m_isEmpty;
    QString m_iconPath;
    QString m_assetPath;
    QPixmap m_iconPixmap;
};

#endif
