#ifndef COMPONENTS_RADIALMENUCOMPONENT_HPP
#define COMPONENTS_RADIALMENUCOMPONENT_HPP

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QtMath>

#include "core/Component.hpp"

class RadialMenuComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(int sliceCount READ GetSliceCount WRITE SetSliceCount NOTIFY ComponentChanged)
    Q_PROPERTY(double innerRadius READ GetInnerRadius WRITE SetInnerRadius NOTIFY ComponentChanged)
    Q_PROPERTY(double outerRadius READ GetOuterRadius WRITE SetOuterRadius NOTIFY ComponentChanged)
    Q_PROPERTY(QColor sliceColor READ GetSliceColor WRITE SetSliceColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor highlightColor READ GetHighlightColor WRITE SetHighlightColor NOTIFY ComponentChanged)
    Q_PROPERTY(int highlightIndex READ GetHighlightIndex WRITE SetHighlightIndex NOTIFY ComponentChanged)

public:

    explicit RadialMenuComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    int GetSliceCount() const noexcept;
    void SetSliceCount(int v);

    double GetInnerRadius() const noexcept;
    void SetInnerRadius(double v);

    double GetOuterRadius() const noexcept;
    void SetOuterRadius(double v);

    QColor GetSliceColor() const noexcept;
    void SetSliceColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    QColor GetHighlightColor() const noexcept;
    void SetHighlightColor(const QColor& v);

    int GetHighlightIndex() const noexcept;
    void SetHighlightIndex(int v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    int m_sliceCount;
    double m_innerRadius;
    double m_outerRadius;
    QColor m_sliceColor;
    QColor m_borderColor;
    QColor m_highlightColor;
    int m_highlightIndex;
};

#endif
