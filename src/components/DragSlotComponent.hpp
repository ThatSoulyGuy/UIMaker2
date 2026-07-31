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
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)

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

private:

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
