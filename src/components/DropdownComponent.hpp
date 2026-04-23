#ifndef COMPONENTS_DROPDOWNCOMPONENT_HPP
#define COMPONENTS_DROPDOWNCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"

class DropdownComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString options READ GetOptions WRITE SetOptions NOTIFY ComponentChanged)
    Q_PROPERTY(int selectedIndex READ GetSelectedIndex WRITE SetSelectedIndex NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)

public:

    explicit DropdownComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_options("Option A,Option B,Option C")
        , m_selectedIndex(0)
        , m_backgroundColor(QColor(45, 45, 50))
        , m_textColor(Qt::white)
        , m_borderColor(QColor(90, 90, 100))
        , m_fontFamily("Inter")
        , m_pixelSize(16) { }

    QString GetTypeName() const override { return QStringLiteral("Dropdown"); }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        QFont font(m_fontFamily, m_pixelSize);
        QFontMetrics fm(font);

        QStringList items = m_options.split(',', Qt::SkipEmptyParts);
        double maxW = 80.0;
        for (const QString& s : items)
            maxW = std::max(maxW, (double)fm.horizontalAdvance(s.trimmed()));

        rect = QRectF(0, 0, maxW + 40.0, fm.height() + 16.0);
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QPen borderPen(m_borderColor, 1);
        borderPen.setCosmetic(true);
        painter->setPen(borderPen);
        painter->setBrush(m_backgroundColor);
        painter->drawRoundedRect(rect, 4.0, 4.0);

        QFont font(m_fontFamily, m_pixelSize);
        painter->setFont(font);
        painter->setPen(m_textColor);

        QStringList items = m_options.split(',', Qt::SkipEmptyParts);
        QString displayText = (m_selectedIndex >= 0 && m_selectedIndex < items.size())
            ? items[m_selectedIndex].trimmed() : "Select...";

        QRectF textRect = rect.adjusted(8, 0, -24, 0);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, displayText);

        // Draw dropdown arrow
        double arrowX = rect.right() - 18.0;
        double arrowY = rect.center().y();
        QPolygonF arrow;
        arrow << QPointF(arrowX - 4, arrowY - 3)
              << QPointF(arrowX + 4, arrowY - 3)
              << QPointF(arrowX, arrowY + 3);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_textColor);
        painter->drawPolygon(arrow);

        if (selected)
        {
            QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
            selPen.setCosmetic(true);
            painter->setPen(selPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(rect, 4.0, 4.0);
        }

        painter->restore();
        return true;
    }

    QString GetOptions() const noexcept { return m_options; }
    void SetOptions(const QString& v) { if (m_options == v) return; m_options = v; NotifyChanged(); }

    int GetSelectedIndex() const noexcept { return m_selectedIndex; }
    void SetSelectedIndex(int v) { if (m_selectedIndex == v) return; m_selectedIndex = v; NotifyChanged(); }

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
        out["kind"] = "Dropdown";
        out["options"] = m_options;
        out["selectedIndex"] = m_selectedIndex;
        out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
        out["textColor"] = m_textColor.name(QColor::HexArgb);
        out["borderColor"] = m_borderColor.name(QColor::HexArgb);
        out["fontFamily"] = m_fontFamily;
        out["pixelSize"] = m_pixelSize;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetOptions(in["options"].toString("Option A,Option B,Option C"));
        SetSelectedIndex(in["selectedIndex"].toInt(0));
        SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF2D2D32")));
        SetTextColor(QColor(in["textColor"].toString("#FFFFFFFF")));
        SetBorderColor(QColor(in["borderColor"].toString("#FF5A5A64")));
        SetFontFamily(in["fontFamily"].toString("Inter"));
        SetPixelSize(in["pixelSize"].toInt(16));
    }

private:

    QString m_options;
    int m_selectedIndex;
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_borderColor;
    QString m_fontFamily;
    int m_pixelSize;
};

#endif
