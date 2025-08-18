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
#include <vector>

enum class ComponentKind
{
    TRANSFORM, IMAGE, TEXT, BUTTON
};

class Component : public QObject
{
    Q_OBJECT

public:

    explicit Component(QObject* parent = nullptr) : QObject(parent) { }

    virtual ~Component() = default;
    virtual ComponentKind GetKind() const noexcept = 0;
    virtual void ToJson(QJsonObject& out) const = 0;
    virtual void FromJson(const QJsonObject& in) = 0;

signals:

    void ComponentChanged();

};

class TransformComponent : public Component
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

    Q_PROPERTY(QPointF position READ GetPosition WRITE SetPosition NOTIFY ComponentChanged)
    Q_PROPERTY(double rotationDegrees READ GetRotationDegrees WRITE SetRotationDegrees NOTIFY ComponentChanged)
    Q_PROPERTY(QPointF scale READ GetScale WRITE SetScale NOTIFY ComponentChanged)
    Q_PROPERTY(AnchorFlags anchors READ GetAnchors WRITE SetAnchors NOTIFY ComponentChanged)
    Q_PROPERTY(AnchorFlags stretch READ GetStretch WRITE SetStretch NOTIFY ComponentChanged)

    explicit TransformComponent(QObject* parent = nullptr) : Component(parent), position(0.0, 0.0), rotationDegrees(0.0), scale(1.0, 1.0), anchors((int)Anchor::LEFT | (int)Anchor::TOP), stretch(Anchor::NONE) { }

    ComponentKind GetKind() const noexcept override
    {
        return ComponentKind::TRANSFORM;
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
    }


    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Transform";
        out["x"] = position.x();
        out["y"] = position.y();
        out["rotationDegrees"] = rotationDegrees;
        out["scaleX"] = scale.x();
        out["scaleY"] = scale.y();
    }

    void FromJson(const QJsonObject& in) override
    {
        SetPosition(QPointF(in["x"].toDouble(0.0), in["y"].toDouble(0.0)));
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

};

using Anchor = TransformComponent::Anchor;
using AnchorFlags = TransformComponent::AnchorFlags;

Q_DECLARE_METATYPE(AnchorFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(AnchorFlags)

class ImageComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(QColor tint READ GetTint WRITE SetTint NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetPath READ GetAssetPath WRITE SetAssetPath NOTIFY ComponentChanged)

public:

    explicit ImageComponent(QObject* parent = nullptr) : Component(parent), tint(Qt::white) { }

    ComponentKind GetKind() const noexcept override
    {
        return ComponentKind::IMAGE;
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

public:

    explicit TextComponent(QObject* parent = nullptr) : Component(parent), fontFamily("Inter"), pixelSize(24), color(Qt::white) { }
    ComponentKind GetKind() const noexcept override { return ComponentKind::TEXT; }

    QString GetText() const noexcept
    {
        return text;
    }

    void SetText(const QString& v)
    {
        if (text == v)
            return;

        text = v;

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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
    }

    void FromJson(const QJsonObject& in) override
    {
        SetText(in["text"].toString());
        SetFontFamily(in["fontFamily"].toString(fontFamily));
        SetPixelSize(in["pixelSize"].toInt(pixelSize));
        SetColor(QColor(in["color"].toString("#FFFFFFFF")));
        SetFontPath(in["fontPath"].toString());
        SetAssetPath(in["assetPath"].toString());
    }

private:

    QString text;
    QString fontFamily;
    int pixelSize;
    QColor color;
    QString fontPath;
    QString assetPath;

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

public:

    explicit ButtonComponent(QObject* parent = nullptr) : Component(parent), backgroundColor(QColor(40, 40, 40)), textColor(Qt::white), fontFamily("Inter"), pixelSize(24) { }

    ComponentKind GetKind() const noexcept override { return ComponentKind::BUTTON; }

    QString GetText() const noexcept
    {
        return text;
    }

    void SetText(const QString& v)
    {
        if (text == v)
            return;

        text = v;

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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

        emit ComponentChanged();
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
    }

private:

    QString text;
    QColor backgroundColor;
    QColor textColor;
    QString fontFamily;
    int pixelSize;
    QString fontPath;
    QString assetPath;

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
