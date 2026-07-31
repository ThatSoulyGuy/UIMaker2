#ifndef COMPONENTS_TOGGLECOMPONENT_HPP
#define COMPONENTS_TOGGLECOMPONENT_HPP

#include <QColor>
#include <QString>

#include "core/Component.hpp"

class ToggleComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(bool checked READ IsChecked WRITE SetChecked NOTIFY ComponentChanged)
    Q_PROPERTY(QColor onColor READ GetOnColor WRITE SetOnColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor offColor READ GetOffColor WRITE SetOffColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor knobColor READ GetKnobColor WRITE SetKnobColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString label READ GetLabel WRITE SetLabel NOTIFY ComponentChanged)

public:

    explicit ToggleComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override;

    bool IsChecked() const noexcept;
    void SetChecked(bool v);

    QColor GetOnColor() const noexcept;
    void SetOnColor(const QColor& v);

    QColor GetOffColor() const noexcept;
    void SetOffColor(const QColor& v);

    QColor GetKnobColor() const noexcept;
    void SetKnobColor(const QColor& v);

    QString GetLabel() const noexcept;
    void SetLabel(const QString& v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    bool m_checked;
    QColor m_onColor;
    QColor m_offColor;
    QColor m_knobColor;
    QString m_label;
};

#endif
