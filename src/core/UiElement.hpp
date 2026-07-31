#ifndef CORE_UIELEMENT_HPP
#define CORE_UIELEMENT_HPP

#include <QObject>
#include <QString>
#include <QUuid>
#include <QJsonObject>
#include <vector>
#include <utility>

class Component;

class UiElement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ GetName WRITE SetName NOTIFY NameChanged)

public:

    explicit UiElement(const QString& name, UiElement* parent = nullptr);

    QUuid GetId() const noexcept;

    void SetId(const QUuid& value);

    QString GetName() const noexcept;

    void SetName(const QString& value);

    std::vector<Component*> GetComponents() const;

    template <typename T> T* GetComponent() const
    {
        for (QObject* child : children())
        {
            if (auto* comp = qobject_cast<T*>(child))
                return comp;
        }

        return nullptr;
    }

    bool IsSlot() const;

    int GetSlotIndex() const;

    template <typename T, typename... Args> T* AddComponent(Args&&... args)
    {
        if (GetComponent<T>() != nullptr)
            return GetComponent<T>();

        auto* c = new T(this, std::forward<Args>(args)...);

        emit ComponentListChanged(this);

        return c;
    }

    UiElement* AddChild(const QString& childName);

    // Move this element under newParent. insertPos is the desired FINAL index
    // among newParent's child *elements* (components are not counted); -1 or
    // out-of-range appends. Returns false if the move is rejected (null
    // target, self, or a descendant of self).
    bool ReparentTo(UiElement* newParent, int insertPos = -1);

    void ToJson(QJsonObject& out) const;

signals:

    void NameChanged(const QString& newName);
    void StructureChanged();
    void ComponentListChanged(UiElement*);

private:

    QUuid id;
    QString name;
};

#endif
