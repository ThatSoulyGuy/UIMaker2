#ifndef COMPONENTS_MODALCOMPONENT_HPP
#define COMPONENTS_MODALCOMPONENT_HPP

#include <QColor>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"

class ModalComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QColor overlayColor READ GetOverlayColor WRITE SetOverlayColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor panelColor READ GetPanelColor WRITE SetPanelColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(double cornerRadius READ GetCornerRadius WRITE SetCornerRadius NOTIFY ComponentChanged)
    Q_PROPERTY(bool visible READ IsVisible WRITE SetVisible NOTIFY ComponentChanged)

public:

    explicit ModalComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QColor GetOverlayColor() const noexcept;
    void SetOverlayColor(const QColor& v);

    QColor GetPanelColor() const noexcept;
    void SetPanelColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    double GetCornerRadius() const noexcept;
    void SetCornerRadius(double v);

    bool IsVisible() const noexcept;
    void SetVisible(bool v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    QColor m_overlayColor;
    QColor m_panelColor;
    QColor m_borderColor;
    double m_cornerRadius;
    bool m_visible;
};

#endif
