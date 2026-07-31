#ifndef COMPONENTS_STACKLAYOUTCOMPONENT_HPP
#define COMPONENTS_STACKLAYOUTCOMPONENT_HPP

#include <QPainter>
#include <QPen>
#include <QList>
#include <algorithm>

#include "core/Component.hpp"

class StackLayoutComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(int direction READ GetDirectionInt WRITE SetDirectionInt NOTIFY ComponentChanged)
    Q_PROPERTY(double spacing READ GetSpacing WRITE SetSpacing NOTIFY ComponentChanged)
    Q_PROPERTY(double padding READ GetPadding WRITE SetPadding NOTIFY ComponentChanged)

public:

    enum Direction { Vertical = 0, Horizontal = 1 };

    explicit StackLayoutComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;
    int UpdateOrder() const override;
    bool IsLayout() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;
    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    int GetDirectionInt() const noexcept;
    void SetDirectionInt(int v);

    Direction GetDirection() const noexcept;
    void SetDirection(Direction v);

    double GetSpacing() const noexcept;
    void SetSpacing(double v);

    double GetPadding() const noexcept;
    void SetPadding(double v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private slots:

    void OnChildChanged();

private:

    Direction m_direction;
    double m_spacing;
    double m_padding;

    QList<QMetaObject::Connection> m_childConnections;
};

#endif
