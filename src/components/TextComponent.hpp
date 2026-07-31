#ifndef COMPONENTS_TEXTCOMPONENT_HPP
#define COMPONENTS_TEXTCOMPONENT_HPP

#include <QString>
#include <QColor>

#include "core/Component.hpp"
#include "core/Anchor.hpp"

class TextComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString text READ GetText WRITE SetText NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)
    Q_PROPERTY(QColor color READ GetColor WRITE SetColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontPath READ GetFontPath WRITE SetFontPath NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetDomain READ GetAssetDomain WRITE SetAssetDomain NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetRegistryValue READ GetAssetRegistryValue WRITE SetAssetRegistryValue NOTIFY ComponentChanged)
    Q_PROPERTY(AnchorFlags alignment READ GetAlignment WRITE SetAlignment NOTIFY ComponentChanged)
    Q_PROPERTY(bool hasBackground READ GetHasBackground WRITE SetHasBackground NOTIFY ComponentChanged)

public:

    explicit TextComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QString GetText() const noexcept;

    void SetText(const QString& v);

    AnchorFlags GetAlignment() const noexcept;

    void SetAlignment(AnchorFlags value);

    QString GetFontFamily() const noexcept;

    void SetFontFamily(const QString& v);

    int GetPixelSize() const noexcept;

    void SetPixelSize(int v);

    QColor GetColor() const noexcept;

    void SetColor(const QColor& v);

    QString GetFontPath() const noexcept;

    void SetFontPath(const QString& v);

    bool GetHasBackground() const noexcept;

    void SetHasBackground(bool v);

    QString GetAssetDomain() const noexcept;

    void SetAssetDomain(const QString& v);

    QString GetAssetRegistryValue() const noexcept;

    void SetAssetRegistryValue(const QString& v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;


private:

    QString text;
    QString fontFamily;
    int pixelSize;
    QColor color;
    QString fontPath;
    QString assetDomain;
    QString assetRegistryValue;
    AnchorFlags alignment;
    bool hasBackground;

};

#endif
