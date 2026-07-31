#include "components/TabContainerComponent.hpp"

#include <algorithm>

#include <QGraphicsItem>
#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QString>
#include <QStringList>

#include "core/UiElement.hpp"
#include "scene/SceneElementItem.hpp"

REGISTER_COMPONENT(TabContainerComponent, "TabContainer")

TabContainerComponent::TabContainerComponent(QObject* parent)
    : Component(parent)
    , m_tabNames("Tab 1,Tab 2,Tab 3")
    , m_activeTab(0)
    , m_tabHeight(32)
    , m_activeColor(QColor(60, 60, 68))
    , m_inactiveColor(QColor(40, 40, 45))
    , m_textColor(Qt::white)
    , m_backgroundColor(QColor(50, 50, 55)) { }

QString TabContainerComponent::GetTypeName() const { return QStringLiteral("TabContainer"); }

void TabContainerComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(rect);
    Q_UNUSED(parentRect);

    for (QGraphicsItem* child : item.childItems())
    {
        auto* sei = dynamic_cast<SceneElementItem*>(child);

        if (!sei)
            continue;

        UiElement* elem = sei->GetElement();

        if (elem && elem->IsSlot())
        {
            sei->setVisible(elem->GetSlotIndex() == m_activeTab);
            sei->RefreshFromComponents();
        }
    }
}

bool TabContainerComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QStringList tabs = m_tabNames.split(',', Qt::SkipEmptyParts);
    int tabCount = std::max(1, static_cast<int>(tabs.size()));
    double tabW = rect.width() / tabCount;

    // Draw tab bar
    QFont font;
    font.setPixelSize(14);
    painter->setFont(font);

    for (int i = 0; i < tabCount; ++i)
    {
        QRectF tabRect(rect.x() + i * tabW, rect.y(), tabW, m_tabHeight);
        bool active = (i == m_activeTab);

        painter->setPen(Qt::NoPen);
        painter->setBrush(active ? m_activeColor : m_inactiveColor);
        painter->drawRect(tabRect);

        // Bottom border for inactive tabs
        if (!active)
        {
            QPen sep(QColor(70, 70, 80), 1);
            sep.setCosmetic(true);
            painter->setPen(sep);
            painter->drawLine(tabRect.bottomLeft(), tabRect.bottomRight());
        }

        painter->setPen(m_textColor);
        QString label = (i < tabs.size()) ? tabs[i].trimmed() : QString("Tab %1").arg(i + 1);
        painter->drawText(tabRect, Qt::AlignCenter, label);
    }

    // Draw content area
    QRectF contentRect(rect.x(), rect.y() + m_tabHeight, rect.width(), rect.height() - m_tabHeight);
    QPen borderPen(QColor(70, 70, 80), 1);
    borderPen.setCosmetic(true);
    painter->setPen(borderPen);
    painter->setBrush(m_backgroundColor);
    painter->drawRect(contentRect);

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

QString TabContainerComponent::GetTabNames() const noexcept { return m_tabNames; }
void TabContainerComponent::SetTabNames(const QString& v) { if (m_tabNames == v) return; m_tabNames = v; NotifyChanged(); }

int TabContainerComponent::GetActiveTab() const noexcept { return m_activeTab; }
void TabContainerComponent::SetActiveTab(int v) { if (m_activeTab == v) return; m_activeTab = v; NotifyChanged(); }

int TabContainerComponent::GetTabHeight() const noexcept { return m_tabHeight; }
void TabContainerComponent::SetTabHeight(int v) { if (m_tabHeight == v) return; m_tabHeight = v; NotifyChanged(); }

QColor TabContainerComponent::GetActiveColor() const noexcept { return m_activeColor; }
void TabContainerComponent::SetActiveColor(const QColor& v) { if (m_activeColor == v) return; m_activeColor = v; NotifyChanged(); }

QColor TabContainerComponent::GetInactiveColor() const noexcept { return m_inactiveColor; }
void TabContainerComponent::SetInactiveColor(const QColor& v) { if (m_inactiveColor == v) return; m_inactiveColor = v; NotifyChanged(); }

QColor TabContainerComponent::GetTextColor() const noexcept { return m_textColor; }
void TabContainerComponent::SetTextColor(const QColor& v) { if (m_textColor == v) return; m_textColor = v; NotifyChanged(); }

QColor TabContainerComponent::GetBackgroundColor() const noexcept { return m_backgroundColor; }
void TabContainerComponent::SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

void TabContainerComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "TabContainer";
    out["tabNames"] = m_tabNames;
    out["activeTab"] = m_activeTab;
    out["tabHeight"] = m_tabHeight;
    out["activeColor"] = m_activeColor.name(QColor::HexArgb);
    out["inactiveColor"] = m_inactiveColor.name(QColor::HexArgb);
    out["textColor"] = m_textColor.name(QColor::HexArgb);
    out["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);
}

void TabContainerComponent::FromJson(const QJsonObject& in)
{
    SetTabNames(in["tabNames"].toString("Tab 1,Tab 2,Tab 3"));
    SetActiveTab(in["activeTab"].toInt(0));
    SetTabHeight(in["tabHeight"].toInt(32));
    SetActiveColor(QColor(in["activeColor"].toString("#FF3C3C44")));
    SetInactiveColor(QColor(in["inactiveColor"].toString("#FF28282D")));
    SetTextColor(QColor(in["textColor"].toString("#FFFFFFFF")));
    SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF323237")));
}
