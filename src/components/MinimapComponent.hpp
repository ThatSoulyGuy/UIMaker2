#ifndef COMPONENTS_MINIMAPCOMPONENT_HPP
#define COMPONENTS_MINIMAPCOMPONENT_HPP

#include <QColor>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"

class MinimapComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor viewportColor READ GetViewportColor WRITE SetViewportColor NOTIFY ComponentChanged)
    Q_PROPERTY(double borderWidth READ GetBorderWidth WRITE SetBorderWidth NOTIFY ComponentChanged)
    Q_PROPERTY(int shape READ GetShapeInt WRITE SetShapeInt NOTIFY ComponentChanged)

public:

    enum Shape { Rectangle = 0, Circle = 1 };

    explicit MinimapComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QColor GetBackgroundColor() const noexcept;
    void SetBackgroundColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    QColor GetViewportColor() const noexcept;
    void SetViewportColor(const QColor& v);

    double GetBorderWidth() const noexcept;
    void SetBorderWidth(double v);

    int GetShapeInt() const noexcept;
    void SetShapeInt(int v);

    Shape GetShape() const noexcept;
    void SetShape(Shape v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    QColor m_backgroundColor;
    QColor m_borderColor;
    QColor m_viewportColor;
    double m_borderWidth;
    Shape m_shape;
};

#endif
