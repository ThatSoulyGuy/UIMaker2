#ifndef COMPONENTS_LISTREPEATERCOMPONENT_HPP
#define COMPONENTS_LISTREPEATERCOMPONENT_HPP

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <algorithm>

#include "core/Component.hpp"

class ListRepeaterComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(int itemCount READ GetItemCount WRITE SetItemCount NOTIFY ComponentChanged)
    Q_PROPERTY(int itemHeight READ GetItemHeight WRITE SetItemHeight NOTIFY ComponentChanged)
    Q_PROPERTY(double spacing READ GetSpacing WRITE SetSpacing NOTIFY ComponentChanged)
    Q_PROPERTY(int direction READ GetDirectionInt WRITE SetDirectionInt NOTIFY ComponentChanged)
    Q_PROPERTY(QString labels READ GetLabels WRITE SetLabels NOTIFY ComponentChanged)
    Q_PROPERTY(QColor itemColor READ GetItemColor WRITE SetItemColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor alternateColor READ GetAlternateColor WRITE SetAlternateColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)

public:

    enum Direction { Vertical = 0, Horizontal = 1 };

    explicit ListRepeaterComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    int GetItemCount() const noexcept;
    void SetItemCount(int v);

    int GetItemHeight() const noexcept;
    void SetItemHeight(int v);

    double GetSpacing() const noexcept;
    void SetSpacing(double v);

    int GetDirectionInt() const noexcept;
    void SetDirectionInt(int v);

    Direction GetDirection() const noexcept;
    void SetDirection(Direction v);

    QString GetLabels() const noexcept;
    void SetLabels(const QString& v);

    QColor GetItemColor() const noexcept;
    void SetItemColor(const QColor& v);

    QColor GetAlternateColor() const noexcept;
    void SetAlternateColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    int m_itemCount;
    int m_itemHeight;
    double m_spacing;
    Direction m_direction;
    QString m_labels;
    QColor m_itemColor;
    QColor m_alternateColor;
    QColor m_borderColor;
};

#endif
