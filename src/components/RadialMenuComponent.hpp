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

    explicit RadialMenuComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_sliceCount(6)
        , m_innerRadius(40.0)
        , m_outerRadius(100.0)
        , m_sliceColor(QColor(50, 50, 58, 200))
        , m_borderColor(QColor(100, 100, 115))
        , m_highlightColor(QColor(80, 160, 255, 100))
        , m_highlightIndex(-1) { }

    QString GetTypeName() const override { return QStringLiteral("RadialMenu"); }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        double size = m_outerRadius * 2.0;
        rect = QRectF(0, 0, size, size);
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QPointF center = rect.center();
        int count = std::max(2, m_sliceCount);
        double anglePerSlice = 360.0 / count;

        for (int i = 0; i < count; ++i)
        {
            double startAngle = i * anglePerSlice - 90.0;

            QPainterPath slicePath;
            slicePath.moveTo(center + QPointF(
                m_innerRadius * qCos(qDegreesToRadians(startAngle)),
                m_innerRadius * qSin(qDegreesToRadians(startAngle))));

            // Outer arc
            for (int d = 0; d <= 20; ++d)
            {
                double a = startAngle + anglePerSlice * d / 20.0;
                slicePath.lineTo(center + QPointF(
                    m_outerRadius * qCos(qDegreesToRadians(a)),
                    m_outerRadius * qSin(qDegreesToRadians(a))));
            }

            // Inner arc (reverse)
            for (int d = 20; d >= 0; --d)
            {
                double a = startAngle + anglePerSlice * d / 20.0;
                slicePath.lineTo(center + QPointF(
                    m_innerRadius * qCos(qDegreesToRadians(a)),
                    m_innerRadius * qSin(qDegreesToRadians(a))));
            }

            slicePath.closeSubpath();

            bool highlighted = (i == m_highlightIndex);
            painter->setBrush(highlighted ? m_highlightColor : m_sliceColor);

            QPen slicePen(m_borderColor, 1);
            slicePen.setCosmetic(true);
            painter->setPen(slicePen);
            painter->drawPath(slicePath);
        }

        if (selected)
        {
            QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
            selPen.setCosmetic(true);
            painter->setPen(selPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(center, m_outerRadius, m_outerRadius);
        }

        painter->restore();
        return true;
    }

    int GetSliceCount() const noexcept { return m_sliceCount; }
    void SetSliceCount(int v) { v = std::max(2, v); if (m_sliceCount == v) return; m_sliceCount = v; NotifyChanged(); }

    double GetInnerRadius() const noexcept { return m_innerRadius; }
    void SetInnerRadius(double v) { if (m_innerRadius == v) return; m_innerRadius = v; NotifyChanged(); }

    double GetOuterRadius() const noexcept { return m_outerRadius; }
    void SetOuterRadius(double v) { if (m_outerRadius == v) return; m_outerRadius = v; NotifyChanged(); }

    QColor GetSliceColor() const noexcept { return m_sliceColor; }
    void SetSliceColor(const QColor& v) { if (m_sliceColor == v) return; m_sliceColor = v; NotifyChanged(); }

    QColor GetBorderColor() const noexcept { return m_borderColor; }
    void SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

    QColor GetHighlightColor() const noexcept { return m_highlightColor; }
    void SetHighlightColor(const QColor& v) { if (m_highlightColor == v) return; m_highlightColor = v; NotifyChanged(); }

    int GetHighlightIndex() const noexcept { return m_highlightIndex; }
    void SetHighlightIndex(int v) { if (m_highlightIndex == v) return; m_highlightIndex = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "RadialMenu";
        out["sliceCount"] = m_sliceCount;
        out["innerRadius"] = m_innerRadius;
        out["outerRadius"] = m_outerRadius;
        out["sliceColor"] = m_sliceColor.name(QColor::HexArgb);
        out["borderColor"] = m_borderColor.name(QColor::HexArgb);
        out["highlightColor"] = m_highlightColor.name(QColor::HexArgb);
        out["highlightIndex"] = m_highlightIndex;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetSliceCount(in["sliceCount"].toInt(6));
        SetInnerRadius(in["innerRadius"].toDouble(40.0));
        SetOuterRadius(in["outerRadius"].toDouble(100.0));
        SetSliceColor(QColor(in["sliceColor"].toString("#C832323A")));
        SetBorderColor(QColor(in["borderColor"].toString("#FF646473")));
        SetHighlightColor(QColor(in["highlightColor"].toString("#6450A0FF")));
        SetHighlightIndex(in["highlightIndex"].toInt(-1));
    }

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
