#include "components/GridLayoutComponent.hpp"
#include "scene/SceneElementItem.hpp"
#include "core/UiElement.hpp"

REGISTER_COMPONENT(GridLayoutComponent, "GridLayout")

void GridLayoutComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(parentRect);

    auto* element = item.GetElement();

    QObject::connect(element, &UiElement::StructureChanged, this, &GridLayoutComponent::OnChildChanged, Qt::UniqueConnection);

    // Collect children
    std::vector<SceneElementItem*> children;
    for (auto* child : item.childItems())
    {
        auto* childItem = dynamic_cast<SceneElementItem*>(child);
        if (!childItem)
            continue;

        auto* childElement = childItem->GetElement();
        for (auto* comp : childElement->GetComponents())
            QObject::connect(comp, &Component::ComponentChanged, this, &GridLayoutComponent::OnChildChanged, Qt::UniqueConnection);

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
