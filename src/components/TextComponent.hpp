#ifndef COMPONENTS_TEXTCOMPONENT_HPP
#define COMPONENTS_TEXTCOMPONENT_HPP

#include <QString>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"
#include "core/Anchor.hpp"

class TextComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString text READ GetText WRITE SetText NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)
    Q_PROPERTY(QColor color READ GetColor WRITE SetColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontPath READ GetFontPath WRITE SetFontPath NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetPath READ GetAssetPath WRITE SetAssetPath NOTIFY ComponentChanged)
    Q_PROPERTY(AnchorFlags alignment READ GetAlignment WRITE SetAlignment NOTIFY ComponentChanged)

public:

    explicit TextComponent(QObject* parent = nullptr) : Component(parent), fontFamily("Inter"), pixelSize(24), color(Qt::white), alignment(Anchor::LEFT | Anchor::TOP) { }

    QString GetTypeName() const override
    {
        return QStringLiteral("Text");
    }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);
        QFont font(fontFamily, pixelSize);
        QFontMetrics fm(font);

        const QSize size(fm.horizontalAdvance(text), fm.height());

        rect = QRectF(QPointF(0.0, 0.0), size);
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();

        QFont font(fontFamily, pixelSize);

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

    QString GetText() const noexcept
    {
        return text;
    }

    void SetText(const QString& v)
    {
        if (text == v)
            return;

        text = v;

        NotifyChanged();
    }

    AnchorFlags GetAlignment() const noexcept
    {
        return alignment;
    }

    void SetAlignment(AnchorFlags value)
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

    QString GetFontFamily() const noexcept
    {
        return fontFamily;
    }

    void SetFontFamily(const QString& v)
    {
        if (fontFamily == v)
            return;

        fontFamily = v;

        NotifyChanged();
    }

    int GetPixelSize() const noexcept
    {
        return pixelSize;
    }

    void SetPixelSize(int v)
    {
        if (pixelSize == v)
            return;

        pixelSize = v;

        NotifyChanged();
    }

    QColor GetColor() const noexcept
    {
        return color;
    }

    void SetColor(const QColor& v)
    {
        if (color == v)
            return;

        color = v;

        NotifyChanged();
    }

    QString GetFontPath() const noexcept
    {
        return fontPath;
    }

    void SetFontPath(const QString& v)
    {
        if (fontPath == v)
            return;

        fontPath = v;

        if (!fontPath.isEmpty())
        {
            int id = QFontDatabase::addApplicationFont(fontPath);

            if (id != -1)
            {
                const QStringList fams = QFontDatabase::applicationFontFamilies(id);
                if (!fams.isEmpty())
                    fontFamily = fams.first();
            }
        }

        NotifyChanged();
    }

    QString GetAssetPath() const noexcept
    {
        return assetPath;
    }

    void SetAssetPath(const QString& v)
    {
        if (assetPath == v)
            return;

        assetPath = v;

        NotifyChanged();
    }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Text";
        out["text"] = text;
        out["fontFamily"] = fontFamily;
        out["pixelSize"] = pixelSize;
        out["color"] = color.name(QColor::HexArgb);
        out["fontPath"] = fontPath;
        out["assetPath"] = assetPath;
        out["alignment"] = static_cast<int>(alignment);
    }

    void FromJson(const QJsonObject& in) override
    {
        SetText(in["text"].toString());
        SetFontFamily(in["fontFamily"].toString(fontFamily));
        SetPixelSize(in["pixelSize"].toInt(pixelSize));
        SetColor(QColor(in["color"].toString("#FFFFFFFF")));
        SetFontPath(in["fontPath"].toString());
        SetAssetPath(in["assetPath"].toString());
        SetAlignment(static_cast<AnchorFlags>(in["alignment"].toInt(static_cast<int>((int)Anchor::LEFT | (int)Anchor::TOP))));
    }


private:

    QString text;
    QString fontFamily;
    int pixelSize;
    QColor color;
    QString fontPath;
    QString assetPath;
    AnchorFlags alignment;

};

#endif
