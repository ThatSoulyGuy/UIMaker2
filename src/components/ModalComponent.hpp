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

    explicit ModalComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_overlayColor(QColor(0, 0, 0, 140))
        , m_panelColor(QColor(45, 45, 50))
        , m_borderColor(QColor(90, 90, 100))
        , m_cornerRadius(8.0)
        , m_visible(true) { }

    QString GetTypeName() const override { return QStringLiteral("Modal"); }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Draw semi-transparent overlay
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_overlayColor);
        painter->drawRect(rect);

        // Draw centered panel (70% of rect)
        double panelW = rect.width() * 0.7;
        double panelH = rect.height() * 0.7;
        QRectF panelRect(
            rect.x() + (rect.width() - panelW) / 2.0,
            rect.y() + (rect.height() - panelH) / 2.0,
            panelW, panelH);

        QPen borderPen(m_borderColor, 1);
        borderPen.setCosmetic(true);
        painter->setPen(borderPen);
        painter->setBrush(m_panelColor);
        painter->drawRoundedRect(panelRect, m_cornerRadius, m_cornerRadius);

        // "X" close button indicator
        double btnSize = 20.0;
        QRectF closeBtn(panelRect.right() - btnSize - 8, panelRect.y() + 8, btnSize, btnSize);
        painter->setPen(QPen(QColor(180, 180, 190), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawLine(closeBtn.topLeft() + QPointF(4, 4), closeBtn.bottomRight() - QPointF(4, 4));
        painter->drawLine(closeBtn.topRight() + QPointF(-4, 4), closeBtn.bottomLeft() + QPointF(4, -4));

        if (!m_visible)
        {
            // Draw "hidden" indicator
            painter->setPen(QPen(QColor(255, 100, 100, 150), 2, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawLine(rect.topLeft(), rect.bottomRight());
            painter->drawLine(rect.topRight(), rect.bottomLeft());
        }

        if (selected)
        {
            QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
            selPen.setCosmetic(true);
            painter->setPen(selPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(rect);
        }

        painter->restore();
        return true;
    }

    QColor GetOverlayColor() const noexcept { return m_overlayColor; }
    void SetOverlayColor(const QColor& v) { if (m_overlayColor == v) return; m_overlayColor = v; NotifyChanged(); }

    QColor GetPanelColor() const noexcept { return m_panelColor; }
    void SetPanelColor(const QColor& v) { if (m_panelColor == v) return; m_panelColor = v; NotifyChanged(); }

    QColor GetBorderColor() const noexcept { return m_borderColor; }
    void SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

    double GetCornerRadius() const noexcept { return m_cornerRadius; }
    void SetCornerRadius(double v) { if (m_cornerRadius == v) return; m_cornerRadius = v; NotifyChanged(); }

    bool IsVisible() const noexcept { return m_visible; }
    void SetVisible(bool v) { if (m_visible == v) return; m_visible = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Modal";
        out["overlayColor"] = m_overlayColor.name(QColor::HexArgb);
        out["panelColor"] = m_panelColor.name(QColor::HexArgb);
        out["borderColor"] = m_borderColor.name(QColor::HexArgb);
        out["cornerRadius"] = m_cornerRadius;
        out["visible"] = m_visible;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetOverlayColor(QColor(in["overlayColor"].toString("#8C000000")));
        SetPanelColor(QColor(in["panelColor"].toString("#FF2D2D32")));
        SetBorderColor(QColor(in["borderColor"].toString("#FF5A5A64")));
        SetCornerRadius(in["cornerRadius"].toDouble(8.0));
        SetVisible(in["visible"].toBool(true));
    }

private:

    QColor m_overlayColor;
    QColor m_panelColor;
    QColor m_borderColor;
    double m_cornerRadius;
    bool m_visible;
};

#endif
