#include "components/TextComponent.hpp"

#include <QFont>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QPainter>
#include <QPen>

#include "core/AssetContext.hpp"

REGISTER_COMPONENT(TextComponent, "Text")

TextComponent::TextComponent(QObject* parent) : Component(parent), fontFamily("Inter"), pixelSize(24), color(Qt::white), alignment(Anchor::LEFT | Anchor::TOP), hasBackground(false) { }

QString TextComponent::GetTypeName() const
{
    return QStringLiteral("Text");
}

void TextComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);
    Q_UNUSED(parentRect);
    QFont font(fontFamily);
    font.setPixelSize(pixelSize);
    QFontMetrics fm(font);

    const QSize size(fm.horizontalAdvance(text), fm.height());

    rect = QRectF(QPointF(0.0, 0.0), size);
}

bool TextComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();

    QFont font(fontFamily);
    font.setPixelSize(pixelSize);

    painter->setFont(font);
    painter->setPen(color);

    Qt::Alignment a = Qt::Alignment();

    if (alignment.testFlag(Anchor::RIGHT))
        a |= Qt::AlignRight;
    else if (alignment.testFlag(Anchor::CENTER_X))
        a |= Qt::AlignHCenter;
    else
        a |= Qt::AlignLeft;

    if (alignment.testFlag(Anchor::BOTTOM))
        a |= Qt::AlignBottom;
    else if (alignment.testFlag(Anchor::CENTER_Y))
        a |= Qt::AlignVCenter;
    else
        a |= Qt::AlignTop;

    painter->setClipRect(rect);

    if (hasBackground)
    {
        const qreal offset = qMax(1.0, pixelSize / 16.0);
        painter->save();
        painter->translate(offset, offset);
        painter->setPen(QColor(0, 0, 0, 160));
        painter->drawText(rect, a, text);
        painter->restore();
    }

    painter->drawText(rect, a, text);

    if (selected)
    {
        painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    painter->restore();
    return true;
}

QString TextComponent::GetText() const noexcept
{
    return text;
}

void TextComponent::SetText(const QString& v)
{
    if (text == v)
        return;

    text = v;

    NotifyChanged();
}

AnchorFlags TextComponent::GetAlignment() const noexcept
{
    return alignment;
}

void TextComponent::SetAlignment(AnchorFlags value)
{
    AnchorFlags sanitized = value;

    if (sanitized.testFlag(Anchor::CENTER_X))
        sanitized &= ~AnchorFlags((int)Anchor::LEFT | (int)Anchor::RIGHT);
    else if (sanitized.testFlag(Anchor::LEFT) || sanitized.testFlag(Anchor::RIGHT))
        sanitized &= ~AnchorFlags(Anchor::CENTER_X);

    if (sanitized.testFlag(Anchor::CENTER_Y))
        sanitized &= ~AnchorFlags((int)Anchor::TOP | (int)Anchor::BOTTOM);
    else if (sanitized.testFlag(Anchor::TOP) || sanitized.testFlag(Anchor::BOTTOM))
        sanitized &= ~AnchorFlags(Anchor::CENTER_Y);

    if (alignment == sanitized)
        return;

    alignment = sanitized;

    NotifyChanged();
}

QString TextComponent::GetFontFamily() const noexcept
{
    return fontFamily;
}

void TextComponent::SetFontFamily(const QString& v)
{
    if (fontFamily == v)
        return;

    fontFamily = v;

    NotifyChanged();
}

int TextComponent::GetPixelSize() const noexcept
{
    return pixelSize;
}

void TextComponent::SetPixelSize(int v)
{
    if (pixelSize == v)
        return;

    pixelSize = v;

    NotifyChanged();
}

QColor TextComponent::GetColor() const noexcept
{
    return color;
}

void TextComponent::SetColor(const QColor& v)
{
    if (color == v)
        return;

    color = v;

    NotifyChanged();
}

QString TextComponent::GetFontPath() const noexcept
{
    return fontPath;
}

void TextComponent::SetFontPath(const QString& v)
{
    if (fontPath == v)
        return;

    fontPath = v;

    if (!fontPath.isEmpty())
    {
        int id = QFontDatabase::addApplicationFont(AssetContext::Resolve(fontPath));

        if (id != -1)
        {
            const QStringList fams = QFontDatabase::applicationFontFamilies(id);
            if (!fams.isEmpty())
                fontFamily = fams.first();
        }
    }

    NotifyChanged();
}

bool TextComponent::GetHasBackground() const noexcept
{
    return hasBackground;
}

void TextComponent::SetHasBackground(bool v)
{
    if (hasBackground == v)
        return;

    hasBackground = v;

    NotifyChanged();
}

QString TextComponent::GetAssetDomain() const noexcept
{
    return assetDomain;
}

void TextComponent::SetAssetDomain(const QString& v)
{
    if (assetDomain == v)
        return;

    assetDomain = v;

    NotifyChanged();
}

QString TextComponent::GetAssetRegistryValue() const noexcept
{
    return assetRegistryValue;
}

void TextComponent::SetAssetRegistryValue(const QString& v)
{
    if (assetRegistryValue == v)
        return;

    assetRegistryValue = v;

    NotifyChanged();
}

void TextComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Text";
    out["text"] = text;
    out["fontFamily"] = fontFamily;
    out["pixelSize"] = pixelSize;
    out["color"] = color.name(QColor::HexArgb);
    out["fontPath"] = fontPath;
    out["assetDomain"] = assetDomain;
    out["assetRegistryValue"] = assetRegistryValue;
    out["alignment"] = static_cast<int>(alignment);
    out["hasBackground"] = hasBackground;
}

void TextComponent::FromJson(const QJsonObject& in)
{
    SetText(in["text"].toString());
    SetFontFamily(in["fontFamily"].toString(fontFamily));
    SetPixelSize(in["pixelSize"].toInt(pixelSize));
    SetColor(QColor(in["color"].toString("#FFFFFFFF")));
    SetFontPath(in["fontPath"].toString());
    SetAssetDomain(in["assetDomain"].toString());
    SetAssetRegistryValue(in["assetRegistryValue"].toString());
    SetAlignment(static_cast<AnchorFlags>(in["alignment"].toInt(static_cast<int>((int)Anchor::LEFT | (int)Anchor::TOP))));
    SetHasBackground(in["hasBackground"].toBool(false));
}
