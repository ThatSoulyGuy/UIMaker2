#ifndef COMPONENTS_TRANSFORMCOMPONENT_HPP
#define COMPONENTS_TRANSFORMCOMPONENT_HPP

#include <QPointF>

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

    explicit TransformComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    int UpdateOrder() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    QPointF GetPosition() const noexcept;

    void SetPosition(const QPointF& v);

    double GetRotationDegrees() const noexcept;

    void SetRotationDegrees(double v);

    QPointF GetScale() const noexcept;

    void SetScale(const QPointF& v);

    AnchorFlags GetAnchors() const noexcept;

    void SetAnchors(AnchorFlags value);

    AnchorFlags GetStretch() const noexcept;

    void SetStretch(AnchorFlags v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;


private:

    QPointF position;
    double rotationDegrees;
    QPointF scale;
    AnchorFlags anchors;
    AnchorFlags stretch;

};

#endif
