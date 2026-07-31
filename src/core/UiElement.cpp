#include "core/UiElement.hpp"

#include "core/Component.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUuid>
#include <vector>

UiElement::UiElement(const QString& name, UiElement* parent) : QObject(parent), id(QUuid::createUuid()), name(name.isEmpty() ? QStringLiteral("Element") : name) { }

QUuid UiElement::GetId() const noexcept
{
    return id;
}

void UiElement::SetId(const QUuid& value)
{
    if (!value.isNull())
        id = value;
}

QString UiElement::GetName() const noexcept
{
    return name;
}

void UiElement::SetName(const QString& value)
{
    if (value.isEmpty() || name == value)
        return;

    name = value;

    emit NameChanged(name);
    emit StructureChanged();
}

std::vector<Component*> UiElement::GetComponents() const
{
    std::vector<Component*> out;

    for (QObject* child : children())
    {
        if (auto* comp = qobject_cast<Component*>(child))
            out.push_back(comp);
    }

    return out;
}

bool UiElement::IsSlot() const
{
    for (auto* c : GetComponents())
    {
        if (c->GetTypeName() == QLatin1String("Slot"))
            return true;
    }

    return false;
}

int UiElement::GetSlotIndex() const
{
    for (auto* c : GetComponents())
    {
        if (c->GetTypeName() == QLatin1String("Slot"))
            return c->property("slotIndex").toInt();
    }

    return -1;
}

UiElement* UiElement::AddChild(const QString& childName)
{
    auto* e = new UiElement(childName, this);

    emit StructureChanged();

    return e;
}

bool UiElement::ReparentTo(UiElement* newParent, int insertPos)
{
    if (newParent == nullptr || newParent == this)
        return false;

    for (auto* p = newParent; p != nullptr; p = qobject_cast<UiElement*>(p->parent()))
    {
        if (p == this)
            return false;
    }

    UiElement* oldParent = qobject_cast<UiElement*>(parent());

    QList<UiElement*> ordered;
    int currentIndex = -1;

    for (QObject* c : newParent->children())
    {
        if (auto* e = qobject_cast<UiElement*>(c))
        {
            if (e == this)
                currentIndex = ordered.size();
            else
                ordered.append(e);
        }
    }

    if (insertPos < 0 || insertPos > ordered.size())
        insertPos = ordered.size();

    if (oldParent == newParent && insertPos == currentIndex)
        return true;

    ordered.insert(insertPos, this);

    // Rebuild the sibling order with plain reparenting only: re-adding a
    // child appends it to the end of the children list, so re-adding every
    // element in the desired order leaves newParent's components first and
    // its child elements in that order. (Directly reordering QObject's
    // children list is not a supported operation.)
    for (UiElement* e : ordered)
    {
        e->setParent(nullptr);
        e->setParent(newParent);
    }

    emit StructureChanged();

    // The hierarchy tree model listens only on the ROOT element, so a move
    // must also surface there or undo/redo replays leave the tree stale.
    UiElement* top = newParent;

    while (auto* p = qobject_cast<UiElement*>(top->parent()))
        top = p;

    if (top != this)
        emit top->StructureChanged();

    return true;
}

void UiElement::ToJson(QJsonObject& out) const
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
