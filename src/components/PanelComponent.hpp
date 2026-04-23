#ifndef COMPONENTS_PANELCOMPONENT_HPP
#define COMPONENTS_PANELCOMPONENT_HPP

#include <QColor>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"

class PanelComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(double borderWidth READ GetBorderWidth WRITE SetBorderWidth NOTIFY ComponentChanged)
    Q_PROPERTY(double cornerRadius READ GetCornerRadius WRITE SetCornerRadius NOTIFY ComponentChanged)

public:

    explicit PanelComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_backgroundColor(QColor(50, 50, 55, 200))
        , m_borderColor(QColor(100, 100, 110))
        , m_borderWidth(1.0)
        , m_cornerRadius(6.0) { }

    QString GetTypeName() const override { return QStringLiteral("Panel"); }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        if (m_borderWidth > 0.0)
        {
            QPen pen(m_borderColor, m_borderWidth);
            pen.setCosmetic(true);
            painter->setPen(pen);
        }
        else
        {
            painter->setPen(Qt::NoPen);
        }

        painter->setBrush(m_backgroundColor);
        painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);

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

    QColor GetBackgroundColor() const noexcept { return m_backgroundColor; }
    void SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

    QColor GetBorderColor() const noexcept { return m_borderColor; }
    void SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

    double GetBorderWidth() const noexcept { return m_borderWidth; }
    void SetBorderWidth(double v) { if (m_borderWidth == v) return; m_borderWidth = v; NotifyChanged(); }

    double GetCornerRadius() const noexcept { return m_cornerRadius; }
    void SetCornerRadius(double v) { if (m_cornerRadius == v) return; m_cornerRadius = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Panel";
        out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
        out["borderColor"] = m_borderColor.name(QColor::HexArgb);
        out["borderWidth"] = m_borderWidth;
        out["cornerRadius"] = m_cornerRadius;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetBackgroundColor(QColor(in["backgroundColor"].toString("#C8323237")));
        SetBorderColor(QColor(in["borderColor"].toString("#FF64646E")));
        SetBorderWidth(in["borderWidth"].toDouble(1.0));
        SetCornerRadius(in["cornerRadius"].toDouble(6.0));
    }

private:

    QColor m_backgroundColor;
    QColor m_borderColor;
    double m_borderWidth;
    double m_cornerRadius;
};

#endif
