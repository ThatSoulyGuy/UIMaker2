#ifndef COMPONENTS_PANELCOMPONENT_HPP
#define COMPONENTS_PANELCOMPONENT_HPP

#include <QColor>

#include "core/Component.hpp"

class PanelComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(double borderWidth READ GetBorderWidth WRITE SetBorderWidth NOTIFY ComponentChanged)
    Q_PROPERTY(double cornerRadius READ GetCornerRadius WRITE SetCornerRadius NOTIFY ComponentChanged)

public:

    explicit PanelComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QColor GetBackgroundColor() const noexcept;
    void SetBackgroundColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    double GetBorderWidth() const noexcept;
    void SetBorderWidth(double v);

    double GetCornerRadius() const noexcept;
    void SetCornerRadius(double v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    QColor m_backgroundColor;
    QColor m_borderColor;
    double m_borderWidth;
    double m_cornerRadius;
};

#endif
