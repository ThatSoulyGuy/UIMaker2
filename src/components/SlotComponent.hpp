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

    explicit SlotComponent(QObject* parent = nullptr) : Component(parent), m_slotIndex(0) { }

    QString GetTypeName() const override
    {
        return QStringLiteral("Slot");
    }

    int UpdateOrder() const override
    {
        return 100;
    }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override;

    int GetSlotIndex() const noexcept
    {
        return m_slotIndex;
    }

    void SetSlotIndex(int v)
    {
        if (m_slotIndex == v)
            return;

        m_slotIndex = v;

        NotifyChanged();
    }

    QString GetMasterKind() const noexcept
    {
        return m_masterKind;
    }

    void SetMasterKind(const QString& v)
    {
        if (m_masterKind == v)
            return;

        m_masterKind = v;

        NotifyChanged();
    }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Slot";
        out["slotIndex"] = m_slotIndex;
        out["masterKind"] = m_masterKind;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetSlotIndex(in["slotIndex"].toInt(0));
        SetMasterKind(in["masterKind"].toString());
    }

private:

    int m_slotIndex;
    QString m_masterKind;
};

#endif
