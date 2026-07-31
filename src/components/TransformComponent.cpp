#include "components/TransformComponent.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"

#include <cmath>
#include <algorithm>

REGISTER_COMPONENT(TransformComponent, "Transform")

TransformComponent::TransformComponent(QObject* parent) : Component(parent), position(0.0, 0.0), rotationDegrees(0.0), scale(100.0, 100.0), anchors(Anchor::LEFT | Anchor::TOP), stretch(Anchor::NONE) { }

QString TransformComponent::GetTypeName() const
{
    return QStringLiteral("Transform");
}

int TransformComponent::UpdateOrder() const
{
    return 1;
}

void TransformComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);

    // Only compute the size here. SceneElementItem applies position/origin/rotation after
    // all component Updates have settled, using the final rect — this keeps the anchor
    // formula agreeing with the localRect that itemChange::ItemPositionHasChanged reads.
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
}

QPointF TransformComponent::GetPosition() const noexcept
{
    return position;
}

void TransformComponent::SetPosition(const QPointF& v)
{
    if (position == v)
        return;

    position = v;

    NotifyChanged();
}

double TransformComponent::GetRotationDegrees() const noexcept
{
    return rotationDegrees;
}

void TransformComponent::SetRotationDegrees(double v)
{
    if (rotationDegrees == v)
        return;

    rotationDegrees = v;

    NotifyChanged();
}

QPointF TransformComponent::GetScale() const noexcept
{
    return scale;
}

void TransformComponent::SetScale(const QPointF& v)
{
    if (scale == v)
        return;

    scale = v;

    NotifyChanged();
}

AnchorFlags TransformComponent::GetAnchors() const noexcept
{
    return anchors;
}

void TransformComponent::SetAnchors(AnchorFlags value)
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

AnchorFlags TransformComponent::GetStretch() const noexcept
{
    return stretch;
}

void TransformComponent::SetStretch(AnchorFlags v)
{
    if (stretch == v)
        return;

    stretch = v;

    NotifyChanged();
}

void TransformComponent::ToJson(QJsonObject& out) const
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

void TransformComponent::FromJson(const QJsonObject& in)
{
    double x = in["x"].toDouble(0.0);
    double y = in["y"].toDouble(0.0);

    auto sane = [](double v){ return std::isfinite(v) && std::abs(v) < 1e7 ? v : 0.0; };

    SetPosition(QPointF(sane(x), sane(y)));

    SetRotationDegrees(in["rotationDegrees"].toDouble(0.0));
    SetScale(QPointF(in["scaleX"].toDouble(100.0), in["scaleY"].toDouble(100.0)));
    SetAnchors(static_cast<AnchorFlags>(in["anchors"].toInt(static_cast<int>((int)Anchor::LEFT | (int)Anchor::TOP))));
    SetStretch(static_cast<AnchorFlags>(in["stretch"].toInt(0)));
}
