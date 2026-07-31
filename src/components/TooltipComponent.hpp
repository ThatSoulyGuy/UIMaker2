#ifndef COMPONENTS_TOOLTIPCOMPONENT_HPP
#define COMPONENTS_TOOLTIPCOMPONENT_HPP

#include <QColor>
#include <QString>

#include "core/Component.hpp"

class TooltipComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString tooltipText READ GetTooltipText WRITE SetTooltipText NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)

public:

    explicit TooltipComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QString GetTooltipText() const noexcept;
    void SetTooltipText(const QString& v);

    QColor GetBackgroundColor() const noexcept;
    void SetBackgroundColor(const QColor& v);

    QColor GetTextColor() const noexcept;
    void SetTextColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    QString GetFontFamily() const noexcept;
    void SetFontFamily(const QString& v);

    int GetPixelSize() const noexcept;
    void SetPixelSize(int v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    QString m_tooltipText;
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_borderColor;
    QString m_fontFamily;
    int m_pixelSize;
};

#endif
