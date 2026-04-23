#ifndef COMPONENTS_PROGRESSBARCOMPONENT_HPP
#define COMPONENTS_PROGRESSBARCOMPONENT_HPP

#include <QColor>
#include <QPainter>
#include <QPen>
#include <algorithm>

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

    enum Direction { Horizontal = 0, Vertical = 1 };

    explicit ProgressBarComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_value(0.5)
        , m_fillColor(QColor(50, 200, 80))
        , m_backgroundColor(QColor(40, 40, 45))
        , m_borderColor(QColor(80, 80, 90))
        , m_direction(Horizontal)
        , m_cornerRadius(4.0) { }

    QString GetTypeName() const override { return QStringLiteral("ProgressBar"); }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QPen borderPen(m_borderColor, 1);
        borderPen.setCosmetic(true);
        painter->setPen(borderPen);
        painter->setBrush(m_backgroundColor);
        painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);

        double clamped = std::clamp(m_value, 0.0, 1.0);
        QRectF fillRect;

        if (m_direction == Horizontal)
            fillRect = QRectF(rect.x(), rect.y(), rect.width() * clamped, rect.height());
        else
            fillRect = QRectF(rect.x(), rect.y() + rect.height() * (1.0 - clamped), rect.width(), rect.height() * clamped);

        if (clamped > 0.0)
        {
            painter->setPen(Qt::NoPen);
            painter->setBrush(m_fillColor);
            painter->drawRoundedRect(fillRect, m_cornerRadius, m_cornerRadius);
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

    double GetValue() const noexcept { return m_value; }
    void SetValue(double v) { v = std::clamp(v, 0.0, 1.0); if (m_value == v) return; m_value = v; NotifyChanged(); }

    QColor GetFillColor() const noexcept { return m_fillColor; }
    void SetFillColor(const QColor& v) { if (m_fillColor == v) return; m_fillColor = v; NotifyChanged(); }

    QColor GetBackgroundColor() const noexcept { return m_backgroundColor; }
    void SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

    QColor GetBorderColor() const noexcept { return m_borderColor; }
    void SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

    int GetDirectionInt() const noexcept { return m_direction; }
    void SetDirectionInt(int v) { SetDirection(static_cast<Direction>(v)); }

    Direction GetDirection() const noexcept { return m_direction; }
    void SetDirection(Direction v) { if (m_direction == v) return; m_direction = v; NotifyChanged(); }

    double GetCornerRadius() const noexcept { return m_cornerRadius; }
    void SetCornerRadius(double v) { if (m_cornerRadius == v) return; m_cornerRadius = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "ProgressBar";
        out["value"] = m_value;
        out["fillColor"] = m_fillColor.name(QColor::HexArgb);
        out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
        out["borderColor"] = m_borderColor.name(QColor::HexArgb);
        out["direction"] = static_cast<int>(m_direction);
        out["cornerRadius"] = m_cornerRadius;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetValue(in["value"].toDouble(0.5));
        SetFillColor(QColor(in["fillColor"].toString("#FF32C850")));
        SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF28282D")));
        SetBorderColor(QColor(in["borderColor"].toString("#FF50505A")));
        SetDirection(static_cast<Direction>(in["direction"].toInt(0)));
        SetCornerRadius(in["cornerRadius"].toDouble(4.0));
    }

private:

    double m_value;
    QColor m_fillColor;
    QColor m_backgroundColor;
    QColor m_borderColor;
    Direction m_direction;
    double m_cornerRadius;
};

#endif
