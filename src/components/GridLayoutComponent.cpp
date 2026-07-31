#include "components/GridLayoutComponent.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"

#include <QJsonObject>

REGISTER_COMPONENT(GridLayoutComponent, "GridLayout")

GridLayoutComponent::GridLayoutComponent(QObject* parent)
    : Component(parent), m_columns(2), m_spacingH(8.0), m_spacingV(8.0), m_padding(8.0) { }

QString GridLayoutComponent::GetTypeName() const { return QStringLiteral("GridLayout"); }
int GridLayoutComponent::UpdateOrder() const { return 100; }
bool GridLayoutComponent::IsLayout() const { return true; }

void GridLayoutComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(parentRect);

    auto* element = item.GetElement();

    // Drop connections to ex-children so removed/reparented items stop
    // triggering relayout of this container, then re-wire the current set.
    for (const auto& conn : m_childConnections)
        QObject::disconnect(conn);
    m_childConnections.clear();

    m_childConnections.append(QObject::connect(element, &UiElement::StructureChanged, this, &GridLayoutComponent::OnChildChanged));

    // Collect children
    std::vector<SceneElementItem*> children;
    for (auto* child : item.childItems())
    {
        auto* childItem = dynamic_cast<SceneElementItem*>(child);
        if (!childItem)
            continue;

        auto* childElement = childItem->GetElement();
        for (auto* comp : childElement->GetComponents())
            m_childConnections.append(QObject::connect(comp, &Component::ComponentChanged, this, &GridLayoutComponent::OnChildChanged));

        children.push_back(childItem);
    }

    if (children.empty())
    {
        rect.setSize(QSizeF(2 * m_padding, 2 * m_padding));
        return;
    }

    int cols = std::max(1, m_columns);
    int rows = static_cast<int>((children.size() + cols - 1) / cols);

    // Calculate max width per column and max height per row
    std::vector<double> colWidths(cols, 0.0);
    std::vector<double> rowHeights(rows, 0.0);

    for (size_t i = 0; i < children.size(); ++i)
    {
        int col = static_cast<int>(i) % cols;
        int row = static_cast<int>(i) / cols;

        QRectF childRect = children[i]->boundingRect();
        colWidths[col] = std::max(colWidths[col], childRect.width());
        rowHeights[row] = std::max(rowHeights[row], childRect.height());
    }

    // Position children
    for (size_t i = 0; i < children.size(); ++i)
    {
        int col = static_cast<int>(i) % cols;
        int row = static_cast<int>(i) / cols;

        double x = m_padding;
        for (int c = 0; c < col; ++c)
            x += colWidths[c] + m_spacingH;

        double y = m_padding;
        for (int r = 0; r < row; ++r)
            y += rowHeights[r] + m_spacingV;

        children[i]->setPosFromComponent(QPointF(x, y));
    }

    // Compute total size
    double totalW = 0.0;
    for (double w : colWidths)
        totalW += w;
    totalW += m_spacingH * std::max(0, cols - 1) + 2 * m_padding;

    double totalH = 0.0;
    for (double h : rowHeights)
        totalH += h;
    totalH += m_spacingV * std::max(0, rows - 1) + 2 * m_padding;

    rect.setSize(QSizeF(totalW, totalH));
}

bool GridLayoutComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();

    QPen pen(QColor(180, 140, 240, 120), 1, Qt::DashLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect);

    if (selected)
    {
        QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
        selPen.setCosmetic(true);
        painter->setPen(selPen);
        painter->drawRect(rect);
    }

    painter->restore();
    return true;
}

int GridLayoutComponent::GetColumns() const noexcept { return m_columns; }
void GridLayoutComponent::SetColumns(int v) { v = std::max(1, v); if (m_columns == v) return; m_columns = v; NotifyChanged(); }

double GridLayoutComponent::GetSpacingH() const noexcept { return m_spacingH; }
void GridLayoutComponent::SetSpacingH(double v) { if (m_spacingH == v) return; m_spacingH = v; NotifyChanged(); }

double GridLayoutComponent::GetSpacingV() const noexcept { return m_spacingV; }
void GridLayoutComponent::SetSpacingV(double v) { if (m_spacingV == v) return; m_spacingV = v; NotifyChanged(); }

double GridLayoutComponent::GetPadding() const noexcept { return m_padding; }
void GridLayoutComponent::SetPadding(double v) { if (m_padding == v) return; m_padding = v; NotifyChanged(); }

void GridLayoutComponent::OnChildChanged() { NotifyChanged(); }

void GridLayoutComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "GridLayout";
    out["columns"] = m_columns;
    out["spacingH"] = m_spacingH;
    out["spacingV"] = m_spacingV;
    out["padding"] = m_padding;
}

void GridLayoutComponent::FromJson(const QJsonObject& in)
{
    SetColumns(in["columns"].toInt(2));
    SetSpacingH(in["spacingH"].toDouble(8.0));
    SetSpacingV(in["spacingV"].toDouble(8.0));
    SetPadding(in["padding"].toDouble(8.0));
}
