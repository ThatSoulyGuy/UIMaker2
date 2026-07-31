#include "components/TextInputComponent.hpp"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <algorithm>

REGISTER_COMPONENT(TextInputComponent, "TextInput")

TextInputComponent::TextInputComponent(QObject* parent)
    : Component(parent)
    , m_placeholder("Enter text...")
    , m_text("")
    , m_backgroundColor(QColor(30, 30, 35))
    , m_textColor(Qt::white)
    , m_placeholderColor(QColor(120, 120, 130))
    , m_borderColor(QColor(80, 80, 90))
    , m_fontFamily("Inter")
    , m_pixelSize(16) { }

QString TextInputComponent::GetTypeName() const { return QStringLiteral("TextInput"); }

void TextInputComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);
    Q_UNUSED(parentRect);

    QFont font(m_fontFamily);
    font.setPixelSize(m_pixelSize);
    QFontMetrics fm(font);
    rect = QRectF(0, 0, 200.0, fm.height() + 16.0);
}

bool TextInputComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen borderPen(m_borderColor, 1);
    borderPen.setCosmetic(true);
    painter->setPen(borderPen);
    painter->setBrush(m_backgroundColor);
    painter->drawRoundedRect(rect, 4.0, 4.0);

    QFont font(m_fontFamily);
    font.setPixelSize(m_pixelSize);
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

QString TextInputComponent::GetPlaceholder() const noexcept { return m_placeholder; }
void TextInputComponent::SetPlaceholder(const QString& v) { if (m_placeholder == v) return; m_placeholder = v; NotifyChanged(); }

QString TextInputComponent::GetText() const noexcept { return m_text; }
void TextInputComponent::SetText(const QString& v) { if (m_text == v) return; m_text = v; NotifyChanged(); }

QColor TextInputComponent::GetBackgroundColor() const noexcept { return m_backgroundColor; }
void TextInputComponent::SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

QColor TextInputComponent::GetTextColor() const noexcept { return m_textColor; }
void TextInputComponent::SetTextColor(const QColor& v) { if (m_textColor == v) return; m_textColor = v; NotifyChanged(); }

QColor TextInputComponent::GetPlaceholderColor() const noexcept { return m_placeholderColor; }
void TextInputComponent::SetPlaceholderColor(const QColor& v) { if (m_placeholderColor == v) return; m_placeholderColor = v; NotifyChanged(); }

QColor TextInputComponent::GetBorderColor() const noexcept { return m_borderColor; }
void TextInputComponent::SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

QString TextInputComponent::GetFontFamily() const noexcept { return m_fontFamily; }
void TextInputComponent::SetFontFamily(const QString& v) { if (m_fontFamily == v) return; m_fontFamily = v; NotifyChanged(); }

int TextInputComponent::GetPixelSize() const noexcept { return m_pixelSize; }
void TextInputComponent::SetPixelSize(int v) { if (m_pixelSize == v) return; m_pixelSize = v; NotifyChanged(); }

void TextInputComponent::ToJson(QJsonObject& out) const
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

void TextInputComponent::FromJson(const QJsonObject& in)
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
