#ifndef COMPONENTS_TABCONTAINERCOMPONENT_HPP
#define COMPONENTS_TABCONTAINERCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"

class TabContainerComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString tabNames READ GetTabNames WRITE SetTabNames NOTIFY ComponentChanged)
    Q_PROPERTY(int activeTab READ GetActiveTab WRITE SetActiveTab NOTIFY ComponentChanged)
    Q_PROPERTY(int tabHeight READ GetTabHeight WRITE SetTabHeight NOTIFY ComponentChanged)
    Q_PROPERTY(QColor activeColor READ GetActiveColor WRITE SetActiveColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor inactiveColor READ GetInactiveColor WRITE SetInactiveColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)

public:

    explicit TabContainerComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_tabNames("Tab 1,Tab 2,Tab 3")
        , m_activeTab(0)
        , m_tabHeight(32)
        , m_activeColor(QColor(60, 60, 68))
        , m_inactiveColor(QColor(40, 40, 45))
        , m_textColor(Qt::white)
        , m_backgroundColor(QColor(50, 50, 55)) { }

    QString GetTypeName() const override { return QStringLiteral("TabContainer"); }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
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

    QString GetTabNames() const noexcept { return m_tabNames; }
    void SetTabNames(const QString& v) { if (m_tabNames == v) return; m_tabNames = v; NotifyChanged(); }

    int GetActiveTab() const noexcept { return m_activeTab; }
    void SetActiveTab(int v) { if (m_activeTab == v) return; m_activeTab = v; NotifyChanged(); }

    int GetTabHeight() const noexcept { return m_tabHeight; }
    void SetTabHeight(int v) { if (m_tabHeight == v) return; m_tabHeight = v; NotifyChanged(); }

    QColor GetActiveColor() const noexcept { return m_activeColor; }
    void SetActiveColor(const QColor& v) { if (m_activeColor == v) return; m_activeColor = v; NotifyChanged(); }

    QColor GetInactiveColor() const noexcept { return m_inactiveColor; }
    void SetInactiveColor(const QColor& v) { if (m_inactiveColor == v) return; m_inactiveColor = v; NotifyChanged(); }

    QColor GetTextColor() const noexcept { return m_textColor; }
    void SetTextColor(const QColor& v) { if (m_textColor == v) return; m_textColor = v; NotifyChanged(); }

    QColor GetBackgroundColor() const noexcept { return m_backgroundColor; }
    void SetBackgroundColor(const QColor& v) { if (m_backgroundColor == v) return; m_backgroundColor = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
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

    void FromJson(const QJsonObject& in) override
    {
        SetTabNames(in["tabNames"].toString("Tab 1,Tab 2,Tab 3"));
        SetActiveTab(in["activeTab"].toInt(0));
        SetTabHeight(in["tabHeight"].toInt(32));
        SetActiveColor(QColor(in["activeColor"].toString("#FF3C3C44")));
        SetInactiveColor(QColor(in["inactiveColor"].toString("#FF28282D")));
        SetTextColor(QColor(in["textColor"].toString("#FFFFFFFF")));
        SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF323237")));
    }

private:

    QString m_tabNames;
    int m_activeTab;
    int m_tabHeight;
    QColor m_activeColor;
    QColor m_inactiveColor;
    QColor m_textColor;
    QColor m_backgroundColor;
};

#endif
