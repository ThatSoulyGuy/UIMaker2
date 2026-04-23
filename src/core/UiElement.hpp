#ifndef CORE_UIELEMENT_HPP
#define CORE_UIELEMENT_HPP

#include <QObject>
#include <QString>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

#include "core/Component.hpp"

class UiElement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ GetName WRITE SetName NOTIFY NameChanged)

public:

    explicit UiElement(const QString& name, UiElement* parent = nullptr) : QObject(parent), id(QUuid::createUuid()), name(name.isEmpty() ? QStringLiteral("Element") : name) { }

    QUuid GetId() const noexcept
    {
        return id;
    }

    QString GetName() const noexcept
    {
        return name;
    }

    void SetName(const QString& value)
    {
        if (value.isEmpty() || name == value)
            return;

        name = value;

        emit NameChanged(name);
        emit StructureChanged();
    }

    std::vector<Component*> GetComponents() const
    {
        std::vector<Component*> out;

        for (QObject* child : children())
        {
            if (auto* comp = qobject_cast<Component*>(child))
                out.push_back(comp);
        }

        return out;
    }

    template <typename T> T* GetComponent() const
    {
        for (QObject* child : children())
        {
            if (auto* comp = qobject_cast<T*>(child))
                return comp;
        }

        return nullptr;
    }

    bool IsSlot() const
    {
        for (auto* c : GetComponents())
        {
            if (c->GetTypeName() == QLatin1String("Slot"))
                return true;
        }

        return false;
    }

    int GetSlotIndex() const
    {
        for (auto* c : GetComponents())
        {
            if (c->GetTypeName() == QLatin1String("Slot"))
                return c->property("slotIndex").toInt();
        }

        return -1;
    }

    template <typename T, typename... Args> T* AddComponent(Args&&... args)
    {
        if (GetComponent<T>() != nullptr)
            return GetComponent<T>();

        auto* c = new T(this, std::forward<Args>(args)...);

        emit ComponentListChanged(this);

        return c;
    }

    UiElement* AddChild(const QString& childName)
    {
        auto* e = new UiElement(childName, this);

        emit StructureChanged();

        return e;
    }

    void ReparentTo(UiElement* newParent, int insertPos = -1)
    {
        if (newParent == nullptr || newParent == this)
            return;

        for (auto* p = qobject_cast<UiElement*>(newParent); p != nullptr; p = qobject_cast<UiElement*>(p->parent()))
        {
            if (p == this)
                return;
        }

        UiElement* oldParent = qobject_cast<UiElement*>(parent());

        if (oldParent == newParent)
        {
            QObjectList& siblings = const_cast<QObjectList&>(newParent->children());
            int currentIndex = siblings.indexOf(this);

            if (insertPos < 0)
                insertPos = siblings.size() - 1;

            if (insertPos > currentIndex)
                --insertPos;

            if (insertPos < 0)
                insertPos = 0;

            if (insertPos >= siblings.size())
                insertPos = siblings.size() - 1;

            if (currentIndex != insertPos)
            {
                siblings.move(currentIndex, insertPos);
                emit StructureChanged();
            }

            return;
        }

        setParent(newParent);

        if (insertPos >= 0)
        {
            QObjectList& siblings = const_cast<QObjectList&>(newParent->children());
            int currentIndex = siblings.indexOf(this);

            if (insertPos >= siblings.size())
                insertPos = siblings.size() - 1;

            if (currentIndex != insertPos)
                siblings.move(currentIndex, insertPos);
        }

        emit StructureChanged();
    }

    void ToJson(QJsonObject& out) const
    {
        out["id"] = id.toString(QUuid::WithoutBraces);
        out["name"] = name;

        QJsonArray comps;

        for (auto* comp : GetComponents())
        {
            QJsonObject c;

            comp->ToJson(c);
            comps.push_back(c);
        }

        out["components"] = comps;

        QJsonArray kids;

        for (QObject* c : children())
        {
            if (auto* e = qobject_cast<UiElement*>(c))
            {
                QJsonObject child;

                e->ToJson(child);

                kids.push_back(child);
            }
        }

        out["children"] = kids;
    }

signals:

    void NameChanged(const QString& newName);
    void StructureChanged();
    void ComponentListChanged(UiElement*);

private:

    QUuid id;
    QString name;
};

#endif
