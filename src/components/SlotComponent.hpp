#ifndef COMPONENTS_SLOTCOMPONENT_HPP
#define COMPONENTS_SLOTCOMPONENT_HPP

#include <QString>

#include "core/Component.hpp"

class SlotComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(int slotIndex READ GetSlotIndex WRITE SetSlotIndex NOTIFY ComponentChanged)
    Q_PROPERTY(QString masterKind READ GetMasterKind WRITE SetMasterKind NOTIFY ComponentChanged)

public:

    explicit SlotComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    int UpdateOrder() const override;

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    int GetSlotIndex() const noexcept;

    void SetSlotIndex(int v);

    QString GetMasterKind() const noexcept;

    void SetMasterKind(const QString& v);

    void ToJson(QJsonObject& out) const override;

    void FromJson(const QJsonObject& in) override;

private:

    int m_slotIndex;
    QString m_masterKind;
};

#endif
