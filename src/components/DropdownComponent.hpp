#ifndef COMPONENTS_DROPDOWNCOMPONENT_HPP
#define COMPONENTS_DROPDOWNCOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>

#include "core/Component.hpp"

class DropdownComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString options READ GetOptions WRITE SetOptions NOTIFY ComponentChanged)
    Q_PROPERTY(int selectedIndex READ GetSelectedIndex WRITE SetSelectedIndex NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)

public:

    explicit DropdownComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QString GetOptions() const noexcept;
    void SetOptions(const QString& v);

    int GetSelectedIndex() const noexcept;
    void SetSelectedIndex(int v);

    QColor GetBackgroundColor() const noexcept;
    void SetBackgroundColor(const QColor& v);

    QColor GetTextColor() const noexcept;
    void SetTextColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    QString GetFontFamily() const noexcept;
    void SetFontFamily(const QString& v);

    int GetPixelSize() const noexcept;
    void SetPixelSize(int v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    QString m_options;
    int m_selectedIndex;
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_borderColor;
    QString m_fontFamily;
    int m_pixelSize;
};

#endif
