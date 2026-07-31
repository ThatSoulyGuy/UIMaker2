#ifndef COMPONENTS_SPRITECOMPONENT_HPP
#define COMPONENTS_SPRITECOMPONENT_HPP

#include <QString>
#include <QPixmap>

#include "core/Component.hpp"

class SpriteComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(int frameWidth READ GetFrameWidth WRITE SetFrameWidth NOTIFY ComponentChanged)
    Q_PROPERTY(int frameHeight READ GetFrameHeight WRITE SetFrameHeight NOTIFY ComponentChanged)
    Q_PROPERTY(int frameCount READ GetFrameCount WRITE SetFrameCount NOTIFY ComponentChanged)
    Q_PROPERTY(int currentFrame READ GetCurrentFrame WRITE SetCurrentFrame NOTIFY ComponentChanged)
    Q_PROPERTY(int columns READ GetColumns WRITE SetColumns NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)

public:

    explicit SpriteComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QString GetImagePath() const noexcept;
    void SetImagePath(const QString& v);

    int GetFrameWidth() const noexcept;
    void SetFrameWidth(int v);

    int GetFrameHeight() const noexcept;
    void SetFrameHeight(int v);

    int GetFrameCount() const noexcept;
    void SetFrameCount(int v);

    int GetCurrentFrame() const noexcept;
    void SetCurrentFrame(int v);

    int GetColumns() const noexcept;
    void SetColumns(int v);

    QString GetAssetDomain() const noexcept;
    void SetAssetDomain(const QString& v);

    QString GetAssetRegistryValue() const noexcept;
    void SetAssetRegistryValue(const QString& v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    QString m_imagePath;
    int m_frameWidth;
    int m_frameHeight;
    int m_frameCount;
    int m_currentFrame;
    int m_columns;
    QString m_assetDomain;
    QString m_assetRegistryValue;
    QPixmap m_pixmap;
};

#endif
