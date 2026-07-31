#ifndef COMPONENTS_GRIDLAYOUTCOMPONENT_HPP
#define COMPONENTS_GRIDLAYOUTCOMPONENT_HPP

#include <QPainter>
#include <QPen>
#include <QList>
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

    explicit GridLayoutComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;
    int UpdateOrder() const override;
    bool IsLayout() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;
    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    int GetColumns() const noexcept;
    void SetColumns(int v);

    double GetSpacingH() const noexcept;
    void SetSpacingH(double v);

    double GetSpacingV() const noexcept;
    void SetSpacingV(double v);

    double GetPadding() const noexcept;
    void SetPadding(double v);

private slots:

    void OnChildChanged();

public:

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    int m_columns;
    double m_spacingH;
    double m_spacingV;
    double m_padding;

    QList<QMetaObject::Connection> m_childConnections;
};

#endif
