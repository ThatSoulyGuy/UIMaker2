#ifndef COMPONENTS_TOOLTIPCOMPONENT_HPP
#define COMPONENTS_TOOLTIPCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QPainterPath>

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

    explicit TooltipComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_tooltipText("Tooltip text here")
        , m_backgroundColor(QColor(25, 25, 30, 230))
        , m_textColor(QColor(220, 220, 225))
        , m_borderColor(QColor(80, 80, 90))
        , m_fontFamily("Inter")
        , m_pixelSize(13) { }

    QString GetTypeName() const override { return QStringLiteral("Tooltip"); }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        QFont font(m_fontFamily, m_pixelSize);
        QFontMetrics fm(font);

        double textW = fm.horizontalAdvance(m_tooltipText);
        double arrowH = 8.0;

        rect = QRectF(0, 0, textW + 20.0, fm.height() + 14.0 + arrowH);
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        double arrowH = 8.0;
        QRectF bodyRect(rect.x(), rect.y(), rect.width(), rect.height() - arrowH);

        QPen borderPen(m_borderColor, 1);
        borderPen.setCosmetic(true);
        painter->setPen(borderPen);
        painter->setBrush(m_backgroundColor);
        painter->drawRoundedRect(bodyRect, 4.0, 4.0);

        // Draw arrow pointing down
        double arrowW = 12.0;
        double cx = bodyRect.center().x();
        QPolygonF arrow;
        arrow << QPointF(cx - arrowW / 2, bodyRect.bottom())
              << QPointF(cx + arrowW / 2, bodyRect.bottom())
              << QPointF(cx, bodyRect.bottom() + arrowH);

        painter->setPen(Qt::NoPen);
        painter->setBrush(m_backgroundColor);
        painter->drawPolygon(arrow);

        // Draw text
        QFont font(m_fontFamily, m_pixelSize);
        painter->setFont(font);
        painter->setPen(m_textColor);
        painter->drawText(bodyRect, Qt::AlignCenter, m_tooltipText);

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

    QString GetTooltipText() const noexcept { return m_tooltipText; }
    void SetTooltipText(const QString& v) { if (m_tooltipText == v) return; m_tooltipText = v; NotifyChanged(); }

    QColor GetBackgroundColor() const noexcept { return m_backgroundColor; }
    void SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

    QColor GetTextColor() const noexcept { return m_textColor; }
    void SetTextColor(const QColor& v) { if (m_textColor == v) return; m_textColor = v; NotifyChanged(); }

    QColor GetBorderColor() const noexcept { return m_borderColor; }
    void SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

    QString GetFontFamily() const noexcept { return m_fontFamily; }
    void SetFontFamily(const QString& v) { if (m_fontFamily == v) return; m_fontFamily = v; NotifyChanged(); }

    int GetPixelSize() const noexcept { return m_pixelSize; }
    void SetPixelSize(int v) { if (m_pixelSize == v) return; m_pixelSize = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Tooltip";
        out["tooltipText"] = m_tooltipText;
        out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
        out["textColor"] = m_textColor.name(QColor::HexArgb);
        out["borderColor"] = m_borderColor.name(QColor::HexArgb);
        out["fontFamily"] = m_fontFamily;
        out["pixelSize"] = m_pixelSize;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetTooltipText(in["tooltipText"].toString("Tooltip text here"));
        SetBackgroundColor(QColor(in["backgroundColor"].toString("#E619191E")));
        SetTextColor(QColor(in["textColor"].toString("#FFDCDCE1")));
        SetBorderColor(QColor(in["borderColor"].toString("#FF50505A")));
        SetFontFamily(in["fontFamily"].toString("Inter"));
        SetPixelSize(in["pixelSize"].toInt(13));
    }

private:

    QString m_tooltipText;
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_borderColor;
    QString m_fontFamily;
    int m_pixelSize;
};

#endif
