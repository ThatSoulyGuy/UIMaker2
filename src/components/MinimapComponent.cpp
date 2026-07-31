#include "components/MinimapComponent.hpp"

#include <QColor>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>

REGISTER_COMPONENT(MinimapComponent, "Minimap")

MinimapComponent::MinimapComponent(QObject* parent)
    : Component(parent)
    , m_backgroundColor(QColor(20, 30, 20, 200))
    , m_borderColor(QColor(100, 120, 100))
    , m_viewportColor(QColor(255, 255, 255, 60))
    , m_borderWidth(2.0)
    , m_shape(Rectangle) { }

QString MinimapComponent::GetTypeName() const { return QStringLiteral("Minimap"); }

bool MinimapComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen borderPen(m_borderColor, m_borderWidth);
    borderPen.setCosmetic(true);
    painter->setPen(borderPen);
    painter->setBrush(m_backgroundColor);

    if (m_shape == Circle)
        painter->drawEllipse(rect);
    else
        painter->drawRect(rect);

    // Draw viewport indicator (small rect in center)
    double vw = rect.width() * 0.3;
    double vh = rect.height() * 0.25;
    QRectF vpRect(
        rect.center().x() - vw / 2.0,
        rect.center().y() - vh / 2.0,
        vw, vh);

    QPen vpPen(m_viewportColor, 1);
    vpPen.setCosmetic(true);
    painter->setPen(vpPen);
    painter->setBrush(QColor(m_viewportColor.red(), m_viewportColor.green(), m_viewportColor.blue(), 30));
    painter->drawRect(vpRect);

    // Draw some terrain dots for visual flair
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(80, 140, 80, 100));
    for (int i = 0; i < 5; ++i)
    {
        double dx = rect.x() + rect.width() * (0.15 + 0.15 * i);
        double dy = rect.y() + rect.height() * (0.3 + 0.08 * (i % 3));
        painter->drawEllipse(QPointF(dx, dy), 3.0, 3.0);
    }

    if (selected)
    {
        QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
        selPen.setCosmetic(true);
        painter->setPen(selPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    painter->restore();
    return true;
}

QColor MinimapComponent::GetBackgroundColor() const noexcept { return m_backgroundColor; }
void MinimapComponent::SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

QColor MinimapComponent::GetBorderColor() const noexcept { return m_borderColor; }
void MinimapComponent::SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

QColor MinimapComponent::GetViewportColor() const noexcept { return m_viewportColor; }
void MinimapComponent::SetViewportColor(const QColor& v) { if (m_viewportColor == v) return; m_viewportColor = v; NotifyChanged(); }

double MinimapComponent::GetBorderWidth() const noexcept { return m_borderWidth; }
void MinimapComponent::SetBorderWidth(double v) { if (m_borderWidth == v) return; m_borderWidth = v; NotifyChanged(); }

int MinimapComponent::GetShapeInt() const noexcept { return m_shape; }
void MinimapComponent::SetShapeInt(int v) { SetShape(static_cast<Shape>(v)); }

MinimapComponent::Shape MinimapComponent::GetShape() const noexcept { return m_shape; }
void MinimapComponent::SetShape(Shape v) { if (m_shape == v) return; m_shape = v; NotifyChanged(); }

void MinimapComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Minimap";
    out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
    out["borderColor"] = m_borderColor.name(QColor::HexArgb);
    out["viewportColor"] = m_viewportColor.name(QColor::HexArgb);
    out["borderWidth"] = m_borderWidth;
    out["shape"] = static_cast<int>(m_shape);
}

void MinimapComponent::FromJson(const QJsonObject& in)
{
    SetBackgroundColor(QColor(in["backgroundColor"].toString("#C8141E14")));
    SetBorderColor(QColor(in["borderColor"].toString("#FF647864")));
    SetViewportColor(QColor(in["viewportColor"].toString("#3CFFFFFF")));
    SetBorderWidth(in["borderWidth"].toDouble(2.0));
    SetShape(static_cast<Shape>(in["shape"].toInt(0)));
}
