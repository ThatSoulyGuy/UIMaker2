#ifndef COMPONENTS_GRIDLAYOUTCOMPONENT_HPP
#define COMPONENTS_GRIDLAYOUTCOMPONENT_HPP

#include <QPainter>
#include <QPen>
#include <algorithm>
#include <vector>

#include "core/Component.hpp"

class GridLayoutComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(int columns READ GetColumns WRITE SetColumns NOTIFY ComponentChanged)
    Q_PROPERTY(double spacingH READ GetSpacingH WRITE SetSpacingH NOTIFY ComponentChanged)
    Q_PROPERTY(double spacingV READ GetSpacingV WRITE SetSpacingV NOTIFY ComponentChanged)
    Q_PROPERTY(double padding READ GetPadding WRITE SetPadding NOTIFY ComponentChanged)

public:

    explicit GridLayoutComponent(QObject* parent = nullptr)
        : Component(parent), m_columns(2), m_spacingH(8.0), m_spacingV(8.0), m_padding(8.0) { }

    QString GetTypeName() const override { return QStringLiteral("GridLayout"); }
    int UpdateOrder() const override { return 100; }
    bool IsLayout() const override { return true; }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;
    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    int GetColumns() const noexcept { return m_columns; }
    void SetColumns(int v) { v = std::max(1, v); if (m_columns == v) return; m_columns = v; NotifyChanged(); }

    double GetSpacingH() const noexcept { return m_spacingH; }
    void SetSpacingH(double v) { if (m_spacingH == v) return; m_spacingH = v; NotifyChanged(); }

    double GetSpacingV() const noexcept { return m_spacingV; }
    void SetSpacingV(double v) { if (m_spacingV == v) return; m_spacingV = v; NotifyChanged(); }

    double GetPadding() const noexcept { return m_padding; }
    void SetPadding(double v) { if (m_padding == v) return; m_padding = v; NotifyChanged(); }

private slots:

    void OnChildChanged() { NotifyChanged(); }

public:

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "GridLayout";
        out["columns"] = m_columns;
        out["spacingH"] = m_spacingH;
        out["spacingV"] = m_spacingV;
        out["padding"] = m_padding;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetColumns(in["columns"].toInt(2));
        SetSpacingH(in["spacingH"].toDouble(8.0));
        SetSpacingV(in["spacingV"].toDouble(8.0));
        SetPadding(in["padding"].toDouble(8.0));
    }

private:

    int m_columns;
    double m_spacingH;
    double m_spacingV;
    double m_padding;
};

#endif
