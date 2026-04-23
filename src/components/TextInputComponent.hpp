#ifndef COMPONENTS_TEXTINPUTCOMPONENT_HPP
#define COMPONENTS_TEXTINPUTCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"

class TextInputComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString placeholder READ GetPlaceholder WRITE SetPlaceholder NOTIFY ComponentChanged)
    Q_PROPERTY(QString text READ GetText WRITE SetText NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor placeholderColor READ GetPlaceholderColor WRITE SetPlaceholderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)

public:

    explicit TextInputComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_placeholder("Enter text...")
        , m_text("")
        , m_backgroundColor(QColor(30, 30, 35))
        , m_textColor(Qt::white)
        , m_placeholderColor(QColor(120, 120, 130))
        , m_borderColor(QColor(80, 80, 90))
        , m_fontFamily("Inter")
        , m_pixelSize(16) { }

    QString GetTypeName() const override { return QStringLiteral("TextInput"); }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        QFont font(m_fontFamily, m_pixelSize);
        QFontMetrics fm(font);
        rect = QRectF(0, 0, 200.0, fm.height() + 16.0);
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

        QRectF textRect = rect.adjusted(8, 0, -8, 0);

        if (m_text.isEmpty())
        {
            painter->setPen(m_placeholderColor);
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_placeholder);
        }
        else
        {
            painter->setPen(m_textColor);
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_text);
        }

        // Draw cursor line
        QFontMetrics fm(font);
        double cursorX = rect.x() + 8.0 + fm.horizontalAdvance(m_text.isEmpty() ? "" : m_text);
        cursorX = std::min(cursorX, rect.right() - 8.0);
        painter->setPen(QPen(m_textColor, 1));
        painter->drawLine(QPointF(cursorX, rect.y() + 6), QPointF(cursorX, rect.bottom() - 6));

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

    QString GetPlaceholder() const noexcept { return m_placeholder; }
    void SetPlaceholder(const QString& v) { if (m_placeholder == v) return; m_placeholder = v; NotifyChanged(); }

    QString GetText() const noexcept { return m_text; }
    void SetText(const QString& v) { if (m_text == v) return; m_text = v; NotifyChanged(); }

    QColor GetBackgroundColor() const noexcept { return m_backgroundColor; }
    void SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

    QColor GetTextColor() const noexcept { return m_textColor; }
    void SetTextColor(const QColor& v) { if (m_textColor == v) return; m_textColor = v; NotifyChanged(); }

    QColor GetPlaceholderColor() const noexcept { return m_placeholderColor; }
    void SetPlaceholderColor(const QColor& v) { if (m_placeholderColor == v) return; m_placeholderColor = v; NotifyChanged(); }

    QColor GetBorderColor() const noexcept { return m_borderColor; }
    void SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

    QString GetFontFamily() const noexcept { return m_fontFamily; }
    void SetFontFamily(const QString& v) { if (m_fontFamily == v) return; m_fontFamily = v; NotifyChanged(); }

    int GetPixelSize() const noexcept { return m_pixelSize; }
    void SetPixelSize(int v) { if (m_pixelSize == v) return; m_pixelSize = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "TextInput";
        out["placeholder"] = m_placeholder;
        out["text"] = m_text;
        out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
        out["textColor"] = m_textColor.name(QColor::HexArgb);
        out["placeholderColor"] = m_placeholderColor.name(QColor::HexArgb);
        out["borderColor"] = m_borderColor.name(QColor::HexArgb);
        out["fontFamily"] = m_fontFamily;
        out["pixelSize"] = m_pixelSize;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetPlaceholder(in["placeholder"].toString("Enter text..."));
        SetText(in["text"].toString(""));
        SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF1E1E23")));
        SetTextColor(QColor(in["textColor"].toString("#FFFFFFFF")));
        SetPlaceholderColor(QColor(in["placeholderColor"].toString("#FF787882")));
        SetBorderColor(QColor(in["borderColor"].toString("#FF50505A")));
        SetFontFamily(in["fontFamily"].toString("Inter"));
        SetPixelSize(in["pixelSize"].toInt(16));
    }

private:

    QString m_placeholder;
    QString m_text;
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_placeholderColor;
    QColor m_borderColor;
    QString m_fontFamily;
    int m_pixelSize;
};

#endif
