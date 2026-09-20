#ifndef COMPONENTS_PROGRESSBARCOMPONENT_HPP
#define COMPONENTS_PROGRESSBARCOMPONENT_HPP

#include <QColor>

#include "core/Component.hpp"

class ProgressBarComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(double value READ GetValue WRITE SetValue NOTIFY ComponentChanged)
    Q_PROPERTY(QColor fillColor READ GetFillColor WRITE SetFillColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(int direction READ GetDirectionInt WRITE SetDirectionInt NOTIFY ComponentChanged)
    Q_PROPERTY(double cornerRadius READ GetCornerRadius WRITE SetCornerRadius NOTIFY ComponentChanged)

public:

    // Vertical = 0, matching StackLayout, ScrollBox and ListRepeater and the
    // property panel's Vertical-first combo. This used to be Horizontal = 0 -
    // the only component in the project that disagreed - so choosing "Vertical"
    // in the inspector stored 0 and rendered a HORIZONTAL bar.
    enum Direction { Vertical = 0, Horizontal = 1 };

    explicit ProgressBarComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    double GetValue() const noexcept;
    void SetValue(double v);

    QColor GetFillColor() const noexcept;
    void SetFillColor(const QColor& v);

    QColor GetBackgroundColor() const noexcept;
    void SetBackgroundColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    int GetDirectionInt() const noexcept;
    void SetDirectionInt(int v);

    Direction GetDirection() const noexcept;
    void SetDirection(Direction v);

    double GetCornerRadius() const noexcept;
    void SetCornerRadius(double v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    double m_value;
    QColor m_fillColor;
    QColor m_backgroundColor;
    QColor m_borderColor;
    Direction m_direction;
    double m_cornerRadius;
};

#endif
