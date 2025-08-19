#ifndef ENTITYCOMPONENTSYSTEM_HPP
#define ENTITYCOMPONENTSYSTEM_HPP

#include <QObject>
#include <QPointF>
#include <QColor>
#include <QFont>
#include <QString>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaProperty>
#include <QMetaType>
#include <QFontDatabase>
#include <QRectF>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QHash>
#include <vector>
#include <functional>
#include <algorithm>
#include "SceneElementItem.hpp"

class Component : public QObject
{
    Q_OBJECT

public:

    explicit Component(QObject* parent = nullptr) : QObject(parent) { }

    virtual ~Component() = default;
    virtual QString GetTypeName() const = 0;

    virtual int UpdateOrder() const
    {
        return 0;
    }

    virtual void Update(class SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
    {
        Q_UNUSED(item);
        Q_UNUSED(rect);
        Q_UNUSED(parentRect);
    }

    virtual bool Paint(QPainter* painter, const QRectF& rect, bool selected)
    {
        Q_UNUSED(painter);
        Q_UNUSED(rect);
        Q_UNUSED(selected);

        return false;
    }

    virtual void ToJson(QJsonObject& out) const = 0;
    virtual void FromJson(const QJsonObject& in) = 0;

    using ComponentFactory = std::function<Component*(QObject*)>;
    static QHash<QString, ComponentFactory>& Registry()
    {
        static QHash<QString, ComponentFactory> registry;

        return registry;
    }

    static void Register(const QString& name, ComponentFactory factory)
    {
        Registry().insert(name, factory);
    }

    static Component* Create(const QString& name, QObject* parent)
    {
        auto it = Registry().find(name);
        return it != Registry().end() ? it.value()(parent) : nullptr;
    }

public slots:

    void EmitComponentChanged()
    {
        emit ComponentChanged();
    }

protected:

    void NotifyChanged()
    {
        QMetaObject::invokeMethod(this, "EmitComponentChanged", Qt::QueuedConnection);
    }

signals:

    void ComponentChanged();

};

class EnumHolder : public QObject
{

    Q_OBJECT

public:

    enum class Anchor
    {
        NONE = 0,
        LEFT = 0x1,
        RIGHT = 0x2,
        TOP = 0x4,
        BOTTOM = 0x8,
        CENTER_X = 0x10,
        CENTER_Y = 0x20
    };

    Q_ENUM(Anchor)
    Q_DECLARE_FLAGS(AnchorFlags, Anchor)
    Q_FLAG(AnchorFlags)
};

using Anchor = EnumHolder::Anchor;
using AnchorFlags = EnumHolder::AnchorFlags;

Q_DECLARE_METATYPE(AnchorFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(AnchorFlags)

class TransformComponent : public Component
{

    Q_OBJECT

public:

    Q_PROPERTY(QPointF position READ GetPosition WRITE SetPosition NOTIFY ComponentChanged)
    Q_PROPERTY(double rotationDegrees READ GetRotationDegrees WRITE SetRotationDegrees NOTIFY ComponentChanged)
    Q_PROPERTY(QPointF scale READ GetScale WRITE SetScale NOTIFY ComponentChanged)
    Q_PROPERTY(AnchorFlags anchors READ GetAnchors WRITE SetAnchors NOTIFY ComponentChanged)
    Q_PROPERTY(AnchorFlags stretch READ GetStretch WRITE SetStretch NOTIFY ComponentChanged)

    explicit TransformComponent(QObject* parent = nullptr) : Component(parent), position(0.0, 0.0), rotationDegrees(0.0), scale(100.0, 100.0), anchors(Anchor::LEFT | Anchor::TOP), stretch(Anchor::NONE) { }

    QString GetTypeName() const override
    {
        return QStringLiteral("Transform");
    }

    int UpdateOrder() const override
    {
        return 1;
    }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);

        QPointF pos = position;

        double targetW = rect.width();
        double targetH = rect.height();

        if (scale.x() > 0.0) targetW = scale.x();
        if (scale.y() > 0.0) targetH = scale.y();

        if (stretch.testFlag(Anchor::LEFT) && stretch.testFlag(Anchor::RIGHT))
            targetW = parentRect.width() - pos.x() * 2.0;

        if (stretch.testFlag(Anchor::TOP) && stretch.testFlag(Anchor::BOTTOM))
            targetH = parentRect.height() - pos.y() * 2.0;

        targetW = std::max(0.0001, targetW);
        targetH = std::max(0.0001, targetH);

        rect.setSize(QSizeF(targetW, targetH));

        double x = pos.x();
        double y = pos.y();

        if (anchors.testFlag(Anchor::RIGHT))
            x = parentRect.width() - rect.width() - pos.x();
        else if (anchors.testFlag(Anchor::CENTER_X))
            x = (parentRect.width() - rect.width()) * 0.5 + pos.x();

        if (anchors.testFlag(Anchor::BOTTOM))
            y = parentRect.height() - rect.height() - pos.y();
        else if (anchors.testFlag(Anchor::CENTER_Y))
            y = (parentRect.height() - rect.height()) * 0.5 + pos.y();

        item.setPosFromComponent(parentRect.topLeft() + QPointF(x, y));
        item.setRotationFromComponent(rotationDegrees);
    }

    QPointF GetPosition() const noexcept
    {
        return position;
    }

    void SetPosition(const QPointF& v)
    {
        if (position == v)
            return;

        position = v;

        NotifyChanged();
    }

    double GetRotationDegrees() const noexcept
    {
        return rotationDegrees;
    }

    void SetRotationDegrees(double v)
    {
        if (rotationDegrees == v)
            return;

        rotationDegrees = v;

        NotifyChanged();
    }

    QPointF GetScale() const noexcept
    {
        return scale;
    }

    void SetScale(const QPointF& v)
    {
        if (scale == v)
            return;

        scale = v;

        NotifyChanged();
    }

    AnchorFlags GetAnchors() const noexcept
    {
        return anchors;
    }

    void SetAnchors(AnchorFlags value)
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

        if (anchors == sanitized)
            return;

        anchors = sanitized;

        NotifyChanged();
    }

    AnchorFlags GetStretch() const noexcept
    {
        return stretch;
    }

    void SetStretch(AnchorFlags v)
    {
        if (stretch == v)
            return;

        stretch = v;

        NotifyChanged();
    }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Transform";
        out["x"] = position.x();
        out["y"] = position.y();
        out["rotationDegrees"] = rotationDegrees;
        out["scaleX"] = scale.x();
        out["scaleY"] = scale.y();
        out["anchors"] = static_cast<int>(anchors);
        out["stretch"] = static_cast<int>(stretch);
    }

    void FromJson(const QJsonObject& in) override
    {
        double x = in["x"].toDouble(0.0);
        double y = in["y"].toDouble(0.0);

        auto sane = [](double v){ return std::isfinite(v) && std::abs(v) < 1e7 ? v : 0.0; };

        SetPosition(QPointF(sane(x), sane(y)));

        SetRotationDegrees(in["rotationDegrees"].toDouble(0.0));
        SetScale(QPointF(in["scaleX"].toDouble(1.0), in["scaleY"].toDouble(1.0)));
        SetAnchors(static_cast<AnchorFlags>(in["anchors"].toInt(static_cast<int>((int)Anchor::LEFT | (int)Anchor::TOP))));
        SetStretch(static_cast<AnchorFlags>(in["stretch"].toInt(0)));
    }


private:

    QPointF position;
    double rotationDegrees;
    QPointF scale;
    AnchorFlags anchors;
    AnchorFlags stretch;

    static const bool registered;

};
class ImageComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(QColor tint READ GetTint WRITE SetTint NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetPath READ GetAssetPath WRITE SetAssetPath NOTIFY ComponentChanged)

public:

    explicit ImageComponent(QObject* parent = nullptr) : Component(parent), tint(Qt::white) { }

    QString GetTypeName() const override
    {
        return QStringLiteral("Image");
    }

    void Update(class SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        if (!imagePath.isEmpty())
        {
            QPixmap loaded(imagePath);

            if (!loaded.isNull())
            {
                pixmap = loaded;
                rect = QRectF(QPointF(0.0, 0.0), loaded.size());
            }
        }
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        if (pixmap.isNull())
            return false;

        painter->save();
        painter->setOpacity(1.0);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->drawPixmap(rect, pixmap, QRectF(QPointF(0.0, 0.0), QSizeF(pixmap.width(), pixmap.height())));

        if (selected)
        {
            painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(rect);
        }

        painter->restore();

        return true;
    }

    QString GetImagePath() const noexcept
    {
        return imagePath;
    }

    void SetImagePath(const QString& v)
    {
        if (imagePath == v)
            return;

        imagePath = v;

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

    QColor GetTint() const noexcept
    {
        return tint;
    }

    void SetTint(const QColor& v)
    {
        if (tint == v)
            return;

        tint = v;

        NotifyChanged();
    }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Image";
        out["imagePath"] = imagePath;
        out["tint"] = tint.name(QColor::HexArgb);
        out["assetPath"] = assetPath;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetImagePath(in["imagePath"].toString());
        SetTint(QColor(in["tint"].toString("#FFFFFFFF")));
        SetAssetPath(in["assetPath"].toString());
    }


private:

    QString imagePath;
    QColor tint;
    QString assetPath;
    QPixmap pixmap;

    static const bool registered;

};

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

    void Update(class SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
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

    static const bool registered;

};

class ButtonComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString text READ GetText WRITE SetText NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontPath READ GetFontPath WRITE SetFontPath NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetPath READ GetAssetPath WRITE SetAssetPath NOTIFY ComponentChanged)
    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceLeft READ GetSliceLeft WRITE SetSliceLeft NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceTop READ GetSliceTop WRITE SetSliceTop NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceRight READ GetSliceRight WRITE SetSliceRight NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceBottom READ GetSliceBottom WRITE SetSliceBottom NOTIFY ComponentChanged)

public:

    explicit ButtonComponent(QObject* parent = nullptr) : Component(parent), backgroundColor(QColor(40, 40, 40)), textColor(Qt::white), fontFamily("Inter"), pixelSize(24) { }

    QString GetTypeName() const override
    {
        return QStringLiteral("Button");
    }

    void Update(class SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        QFont font(fontFamily, pixelSize);
        QFontMetrics fm(font);

        const QSize size(fm.horizontalAdvance(text) + 40, fm.height() + 20);

        rect = QRectF(QPointF(0.0, 0.0), size);
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QPixmap& skin = !customSkin.isNull() ? customSkin : EnsureDefaultSkin();

        DrawNineSlice(painter, rect, skin, sliceLeft, sliceTop, sliceRight, sliceBottom);

        painter->setPen(textColor);

        QFont font(fontFamily, pixelSize);

        painter->setFont(font);
        painter->drawText(rect, Qt::AlignCenter, text);

        if (selected)
        {
            painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(rect, 6.0, 6.0);
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

    QColor GetBackgroundColor() const noexcept
    {
        return backgroundColor;
    }

    void SetBackgroundColor(const QColor& v)
    {
        if (backgroundColor == v)
            return;

        backgroundColor = v;

        NotifyChanged();
    }

    QColor GetTextColor() const noexcept
    {
        return textColor;
    }

    void SetTextColor(const QColor& v)
    {
        if (textColor == v)
            return;

        textColor = v;

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

    QString GetImagePath() const noexcept
    {
        return imagePath;
    }

    void SetImagePath(const QString& v)
    {
        if (imagePath == v)
            return;

        imagePath = v;
        customSkin = QPixmap();

        if (!imagePath.isEmpty())
        {
            QPixmap loaded(imagePath);

            if (!loaded.isNull())
                customSkin = loaded;
        }

        NotifyChanged();
    }

    int GetSliceLeft() const noexcept
    {
        return sliceLeft;
    }

    int GetSliceTop() const noexcept
    {
        return sliceTop;
    }

    int GetSliceRight() const noexcept
    {
        return sliceRight;
    }

    int GetSliceBottom() const noexcept
    {
        return sliceBottom;
    }

    void SetSliceLeft(int v)
    {
        v = std::max(0, v);

        if (sliceLeft == v)
            return;

        sliceLeft = v;

        InvalidateDefaultSkin();
        NotifyChanged();
    }

    void SetSliceTop(int v)
    {
        v = std::max(0, v);

        if (sliceTop == v)
            return;

        sliceTop = v;

        InvalidateDefaultSkin();
        NotifyChanged();
    }

    void SetSliceRight(int v)
    {
        v = std::max(0, v);

        if (sliceRight == v)
            return;

        sliceRight = v;

        InvalidateDefaultSkin();
        NotifyChanged();
    }

    void SetSliceBottom(int v)
    {
        v = std::max(0, v);

        if (sliceBottom == v)
            return;

        sliceBottom = v;

        InvalidateDefaultSkin();
        NotifyChanged();
    }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Button";
        out["text"] = text;
        out["backgroundColor"] = backgroundColor.name(QColor::HexArgb);
        out["textColor"] = textColor.name(QColor::HexArgb);
        out["fontFamily"] = fontFamily;
        out["pixelSize"] = pixelSize;
        out["fontPath"] = fontPath;
        out["assetPath"] = assetPath;
        out["imagePath"] = imagePath;
        out["sliceLeft"] = sliceLeft;
        out["sliceTop"] = sliceTop;
        out["sliceRight"] = sliceRight;
        out["sliceBottom"] = sliceBottom;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetText(in["text"].toString("Button"));
        SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF282828")));
        SetTextColor(QColor(in["textColor"].toString("#FFFFFFFF")));
        SetFontFamily(in["fontFamily"].toString(fontFamily));
        SetPixelSize(in["pixelSize"].toInt(pixelSize));
        SetFontPath(in["fontPath"].toString());
        SetAssetPath(in["assetPath"].toString());
        SetImagePath(in["imagePath"].toString());
        SetSliceLeft(in["sliceLeft"].toInt(6));
        SetSliceTop(in["sliceTop"].toInt(6));
        SetSliceRight(in["sliceRight"].toInt(6));
        SetSliceBottom(in["sliceBottom"].toInt(6));
    }

private:

    mutable QPixmap defaultSkin;
    mutable QColor defaultSkinColor;

    void InvalidateDefaultSkin()
    {
        defaultSkin = QPixmap();
    }

    const QPixmap& EnsureDefaultSkin() const
    {
        if (!defaultSkin.isNull() && defaultSkinColor == backgroundColor)
            return defaultSkin;

        const int baseW = std::max(48, sliceLeft + sliceRight + 24);
        const int baseH = std::max(32, sliceTop  + sliceBottom + 16);

        QPixmap pm(baseW, baseH);
        pm.fill(Qt::transparent);

        QPainter p(&pm);

        p.setRenderHint(QPainter::Antialiasing, true);

        QLinearGradient g(0, 0, 0, baseH);

        QColor top = backgroundColor.lighter(115);
        QColor bot = backgroundColor.darker(105);

        g.setColorAt(0.0, top);
        g.setColorAt(1.0, bot);

        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawRoundedRect(QRectF(0.5, 0.5, baseW - 1.0, baseH - 1.0), 6.0, 6.0);

        p.setPen(QPen(QColor(0, 0, 0, 110), 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(0.5, 0.5, baseW - 1.0, baseH - 1.0), 6.0, 6.0);

        p.setPen(QPen(QColor(255, 255, 255, 35), 1.0));
        p.drawLine(QPointF(6.0, 1.0), QPointF(baseW - 6.0, 1.0));

        p.end();

        defaultSkin = pm;
        defaultSkinColor = backgroundColor;

        return defaultSkin;
    }

    static void DrawNineSlice(QPainter* painter, const QRectF& dest, const QPixmap& img, int l, int t, int r, int b)
    {
        if (img.isNull() || dest.isEmpty())
            return;

        const int iw = img.width();
        const int ih = img.height();

        const int midW = iw - l - r;
        const int midH = ih - t - b;

        if (midW <= 0 || midH <= 0)
        {
            painter->drawPixmap(dest, img, QRectF(0, 0, iw, ih));
            return;
        }

        const qreal dl = std::min<qreal>(l, dest.width()  * 0.5);
        const qreal dt = std::min<qreal>(t, dest.height() * 0.5);
        const qreal dr = std::min<qreal>(r, dest.width()  - dl);
        const qreal db = std::min<qreal>(b, dest.height() - dt);
        const qreal dw = std::max<qreal>(0.0, dest.width()  - dl - dr);
        const qreal dh = std::max<qreal>(0.0, dest.height() - dt - db);

        const qreal x0 = dest.x();
        const qreal y0 = dest.y();
        const qreal x1 = x0 + dl;
        const qreal x2 = x1 + dw;
        const qreal y1 = y0 + dt;
        const qreal y2 = y1 + dh;

        auto SR = [](int x, int y, int w, int h){ return QRectF(x, y, w, h); };
        auto DR = [](qreal x, qreal y, qreal w, qreal h){ return QRectF(x, y, w, h); };

        const QRectF sTL = SR(0,       0,       l,   t);
        const QRectF sT = SR(l,       0,       midW,t);
        const QRectF sTR = SR(l+midW,  0,       r,   t);
        const QRectF sL = SR(0,       t,       l,   midH);
        const QRectF sC = SR(l,       t,       midW,midH);
        const QRectF sR = SR(l+midW,  t,       r,   midH);
        const QRectF sBL = SR(0,       t+midH,  l,   b);
        const QRectF sB = SR(l,       t+midH,  midW,b);
        const QRectF sBR = SR(l+midW,  t+midH,  r,   b);

        const QRectF dTL = DR(x0, y0, dl, dt);
        const QRectF dT = DR(x1, y0, dw, dt);
        const QRectF dTR = DR(x2, y0, dr, dt);
        const QRectF dL = DR(x0, y1, dl, dh);
        const QRectF dC = DR(x1, y1, dw, dh);
        const QRectF dR = DR(x2, y1, dr, dh);
        const QRectF dBL = DR(x0, y2, dl, db);
        const QRectF dB = DR(x1, y2, dw, db);
        const QRectF dBR = DR(x2, y2, dr, db);

        painter->drawPixmap(dTL, img, sTL);
        painter->drawPixmap(dT, img, sT);
        painter->drawPixmap(dTR, img, sTR);
        painter->drawPixmap(dL, img, sL);
        painter->drawPixmap(dC, img, sC);
        painter->drawPixmap(dR, img, sR);
        painter->drawPixmap(dBL, img, sBL);
        painter->drawPixmap(dB, img, sB);
        painter->drawPixmap(dBR, img, sBR);
    }

    QString text;
    QColor backgroundColor;
    QColor textColor;
    QString fontFamily;
    int pixelSize;
    QString fontPath;
    QString assetPath;

    QString imagePath;
    QPixmap customSkin;

    int sliceLeft   = 6;
    int sliceTop    = 6;
    int sliceRight  = 6;
    int sliceBottom = 6;

    static const bool registered;

};

class UiElement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ GetName WRITE SetName NOTIFY NameChanged)

public:

    explicit UiElement(const QString& name, UiElement* parent = nullptr) : QObject(parent), id(QUuid::createUuid()), name(name) { }

    QUuid GetId() const noexcept
    {
        return id;
    }

    QString GetName() const noexcept
    {
        return name;
    }

    void SetName(const QString& value)
    {
        if (name == value)
            return;

        name = value;

        emit NameChanged(name);
        emit StructureChanged();
    }

    std::vector<Component*> GetComponents() const
    {
        std::vector<Component*> out;

        for (QObject* child : children())
        {
            if (auto* comp = qobject_cast<Component*>(child))
                out.push_back(comp);
        }

        return out;
    }

    template <typename T> T* GetComponent() const
    {
        for (QObject* child : children())
        {
            if (auto* comp = qobject_cast<T*>(child))
                return comp;
        }

        return nullptr;
    }

    template <typename T, typename... Args> T* AddComponent(Args&&... args)
    {
        if (GetComponent<T>() != nullptr)
            return GetComponent<T>();

        auto* c = new T(this, std::forward<Args>(args)...);

        emit ComponentListChanged(this);

        return c;
    }

    UiElement* AddChild(const QString& childName)
    {
        auto* e = new UiElement(childName, this);

        emit StructureChanged();

        return e;
    }

    void ReparentTo(UiElement* newParent, int insertPos = -1)
    {
        if (newParent == nullptr || newParent == this)
            return;

        for (auto* p = qobject_cast<UiElement*>(newParent); p != nullptr; p = qobject_cast<UiElement*>(p->parent()))
        {
            if (p == this)
                return;
        }

        UiElement* oldParent = qobject_cast<UiElement*>(parent());

        if (oldParent == newParent)
        {
            QObjectList& siblings = const_cast<QObjectList&>(newParent->children());
            int currentIndex = siblings.indexOf(this);

            if (insertPos < 0)
                insertPos = siblings.size() - 1;

            if (insertPos > currentIndex)
                --insertPos;

            if (insertPos < 0)
                insertPos = 0;

            if (insertPos >= siblings.size())
                insertPos = siblings.size() - 1;

            if (currentIndex != insertPos)
            {
                siblings.move(currentIndex, insertPos);
                emit StructureChanged();
            }

            return;
        }

        setParent(newParent);

        if (insertPos >= 0)
        {
            QObjectList& siblings = const_cast<QObjectList&>(newParent->children());
            int currentIndex = siblings.indexOf(this);

            if (insertPos >= siblings.size())
                insertPos = siblings.size() - 1;

            if (currentIndex != insertPos)
                siblings.move(currentIndex, insertPos);
        }

        emit StructureChanged();
    }

    void ToJson(QJsonObject& out) const
    {
        out["id"] = id.toString(QUuid::WithoutBraces);
        out["name"] = name;

        QJsonArray comps;

        for (auto* comp : GetComponents())
        {
            QJsonObject c;

            comp->ToJson(c);
            comps.push_back(c);
        }

        out["components"] = comps;

        QJsonArray kids;

        for (QObject* c : children())
        {
            if (auto* e = qobject_cast<UiElement*>(c))
            {
                QJsonObject child;

                e->ToJson(child);

                kids.push_back(child);
            }
        }

        out["children"] = kids;
    }

signals:

    void NameChanged(const QString& newName);
    void StructureChanged();
    void ComponentListChanged(UiElement*);

private:

    QUuid id;
    QString name;
};

#endif
