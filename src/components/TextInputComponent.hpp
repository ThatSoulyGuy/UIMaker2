#ifndef COMPONENTS_TEXTINPUTCOMPONENT_HPP
#define COMPONENTS_TEXTINPUTCOMPONENT_HPP

#include <QColor>
#include <QString>

#include "core/Component.hpp"

class TextInputComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString placeholder READ GetPlaceholder WRITE SetPlaceholder NOTIFY ComponentChanged)
    Q_PROPERTY(QString text READ GetText WRITE SetText NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor placeholderColor READ GetPlaceholderColor WRITE SetPlaceholderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)

public:

    explicit TextInputComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    QString GetPlaceholder() const noexcept;
    void SetPlaceholder(const QString& v);

    QString GetText() const noexcept;
    void SetText(const QString& v);

    QColor GetBackgroundColor() const noexcept;
    void SetBackgroundColor(const QColor& v);

    QColor GetTextColor() const noexcept;
    void SetTextColor(const QColor& v);

    QColor GetPlaceholderColor() const noexcept;
    void SetPlaceholderColor(const QColor& v);

    QColor GetBorderColor() const noexcept;
    void SetBorderColor(const QColor& v);

    QString GetFontFamily() const noexcept;
    void SetFontFamily(const QString& v);

    int GetPixelSize() const noexcept;
    void SetPixelSize(int v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    QString m_placeholder;
    QString m_text;
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_placeholderColor;
    QColor m_borderColor;
    QString m_fontFamily;
    int m_pixelSize;
};

#endif
