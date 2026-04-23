#ifndef COMPONENTS_SCROLLBOXCOMPONENT_HPP
#define COMPONENTS_SCROLLBOXCOMPONENT_HPP

#include <QPainter>
#include <QPen>
#include <algorithm>

#include "core/Component.hpp"

class ScrollBoxComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(int direction READ GetDirectionInt WRITE SetDirectionInt NOTIFY ComponentChanged)
    Q_PROPERTY(double spacing READ GetSpacing WRITE SetSpacing NOTIFY ComponentChanged)
    Q_PROPERTY(double padding READ GetPadding WRITE SetPadding NOTIFY ComponentChanged)

public:

    enum Direction { Vertical = 0, Horizontal = 1 };

    explicit ScrollBoxComponent(QObject* parent = nullptr)
        : Component(parent), m_direction(Vertical), m_spacing(8.0), m_padding(8.0) { }

    QString GetTypeName() const override { return QStringLiteral("ScrollBox"); }
    int UpdateOrder() const override { return 100; }
    bool IsLayout() const override { return true; }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;
    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    int GetDirectionInt() const noexcept { return m_direction; }
    void SetDirectionInt(int v) { SetDirection(static_cast<Direction>(v)); }

    Direction GetDirection() const noexcept { return m_direction; }
    void SetDirection(Direction v) { if (m_direction == v) return; m_direction = v; NotifyChanged(); }

    double GetSpacing() const noexcept { return m_spacing; }
    void SetSpacing(double v) { if (m_spacing == v) return; m_spacing = v; NotifyChanged(); }

    double GetPadding() const noexcept { return m_padding; }
    void SetPadding(double v) { if (m_padding == v) return; m_padding = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "ScrollBox";
        out["direction"] = static_cast<int>(m_direction);
        out["spacing"] = m_spacing;
        out["padding"] = m_padding;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetDirection(static_cast<Direction>(in["direction"].toInt(0)));
        SetSpacing(in["spacing"].toDouble(8.0));
        SetPadding(in["padding"].toDouble(8.0));
    }

private slots:

    void OnChildChanged() { NotifyChanged(); }

private:

    Direction m_direction;
    double m_spacing;
    double m_padding;
};

#endif
