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

    explicit ListRepeaterComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_itemCount(5)
        , m_itemHeight(40)
        , m_spacing(2.0)
        , m_direction(Vertical)
        , m_labels("Item 1,Item 2,Item 3,Item 4,Item 5")
        , m_itemColor(QColor(45, 45, 52))
        , m_alternateColor(QColor(50, 50, 58))
        , m_borderColor(QColor(70, 70, 80)) { }

    QString GetTypeName() const override { return QStringLiteral("ListRepeater"); }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        if (m_direction == Vertical)
        {
            double totalH = m_itemCount * m_itemHeight + std::max(0, m_itemCount - 1) * m_spacing;
            rect = QRectF(0, 0, 200.0, totalH);
        }
        else
        {
            double totalW = m_itemCount * m_itemHeight + std::max(0, m_itemCount - 1) * m_spacing;
            rect = QRectF(0, 0, totalW, 60.0);
        }
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QFont font;
        font.setPixelSize(12);
        painter->setFont(font);

        QPen borderPen(m_borderColor, 1);
        borderPen.setCosmetic(true);

        QStringList labels = m_labels.split(',', Qt::KeepEmptyParts);

        for (int i = 0; i < m_itemCount; ++i)
        {
            QRectF itemRect;

            if (m_direction == Vertical)
            {
                double y = rect.y() + i * (m_itemHeight + m_spacing);
                itemRect = QRectF(rect.x(), y, rect.width(), m_itemHeight);
            }
            else
            {
                double x = rect.x() + i * (m_itemHeight + m_spacing);
                itemRect = QRectF(x, rect.y(), m_itemHeight, rect.height());
            }

            QColor bg = (i % 2 == 0) ? m_itemColor : m_alternateColor;
            painter->setPen(borderPen);
            painter->setBrush(bg);
            painter->drawRect(itemRect);

            QString label = (i < labels.size()) ? labels[i].trimmed() : QString();

            if (label.isEmpty())
                label = QString("Item %1").arg(i + 1);

            painter->setPen(QColor(160, 160, 170));
            painter->drawText(itemRect, Qt::AlignCenter, label);
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

    int GetItemCount() const noexcept { return m_itemCount; }
    void SetItemCount(int v) { v = std::max(1, v); if (m_itemCount == v) return; m_itemCount = v; NotifyChanged(); }

    int GetItemHeight() const noexcept { return m_itemHeight; }
    void SetItemHeight(int v) { v = std::max(1, v); if (m_itemHeight == v) return; m_itemHeight = v; NotifyChanged(); }

    double GetSpacing() const noexcept { return m_spacing; }
    void SetSpacing(double v) { if (m_spacing == v) return; m_spacing = v; NotifyChanged(); }

    int GetDirectionInt() const noexcept { return m_direction; }
    void SetDirectionInt(int v) { SetDirection(static_cast<Direction>(v)); }

    Direction GetDirection() const noexcept { return m_direction; }
    void SetDirection(Direction v) { if (m_direction == v) return; m_direction = v; NotifyChanged(); }

    QString GetLabels() const noexcept { return m_labels; }
    void SetLabels(const QString& v) { if (m_labels == v) return; m_labels = v; NotifyChanged(); }

    QColor GetItemColor() const noexcept { return m_itemColor; }
    void SetItemColor(const QColor& v) { if (m_itemColor == v) return; m_itemColor = v; NotifyChanged(); }

    QColor GetAlternateColor() const noexcept { return m_alternateColor; }
    void SetAlternateColor(const QColor& v) { if (m_alternateColor == v) return; m_alternateColor = v; NotifyChanged(); }

    QColor GetBorderColor() const noexcept { return m_borderColor; }
    void SetBorderColor(const QColor& v) { if (m_borderColor == v) return; m_borderColor = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "ListRepeater";
        out["itemCount"] = m_itemCount;
        out["itemHeight"] = m_itemHeight;
        out["spacing"] = m_spacing;
        out["direction"] = static_cast<int>(m_direction);
        out["labels"] = m_labels;
        out["itemColor"] = m_itemColor.name(QColor::HexArgb);
        out["alternateColor"] = m_alternateColor.name(QColor::HexArgb);
        out["borderColor"] = m_borderColor.name(QColor::HexArgb);
    }

    void FromJson(const QJsonObject& in) override
    {
        SetItemCount(in["itemCount"].toInt(5));
        SetItemHeight(in["itemHeight"].toInt(40));
        SetSpacing(in["spacing"].toDouble(2.0));
        SetDirection(static_cast<Direction>(in["direction"].toInt(0)));
        SetLabels(in["labels"].toString("Item 1,Item 2,Item 3,Item 4,Item 5"));
        SetItemColor(QColor(in["itemColor"].toString("#FF2D2D34")));
        SetAlternateColor(QColor(in["alternateColor"].toString("#FF32323A")));
        SetBorderColor(QColor(in["borderColor"].toString("#FF464650")));
    }

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
