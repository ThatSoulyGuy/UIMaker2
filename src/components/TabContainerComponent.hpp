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

    explicit TabContainerComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QString GetTabNames() const noexcept;
    void SetTabNames(const QString& v);

    int GetActiveTab() const noexcept;
    void SetActiveTab(int v);

    int GetTabHeight() const noexcept;
    void SetTabHeight(int v);

    QColor GetActiveColor() const noexcept;
    void SetActiveColor(const QColor& v);

    QColor GetInactiveColor() const noexcept;
    void SetInactiveColor(const QColor& v);

    QColor GetTextColor() const noexcept;
    void SetTextColor(const QColor& v);

    QColor GetBackgroundColor() const noexcept;
    void SetBackgroundColor(const QColor& v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

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
