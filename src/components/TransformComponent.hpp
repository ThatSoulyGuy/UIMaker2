#ifndef COMPONENTS_TRANSFORMCOMPONENT_HPP
#define COMPONENTS_TRANSFORMCOMPONENT_HPP

#include <QPointF>
#include <cmath>
#include <algorithm>

#include "core/Component.hpp"
#include "core/Anchor.hpp"

class SceneElementItem;

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

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

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

};

#endif
