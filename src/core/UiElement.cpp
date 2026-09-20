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
    // A null id means the source (a hand-edited scene.json, a corrupt .uibin)
    // had no usable "id". Keeping the freshly generated one is the safe
    // fallback, but it silently orphans every undo record and selection restore
    // keyed to the original - so at least say so.
    if (value.isNull())
    {
        qWarning("UiElement::SetId: ignoring null UUID for element \"%s\"; keeping generated id %s",
                 qUtf8Printable(name), qUtf8Printable(id.toString(QUuid::WithoutBraces)));
        return;
    }

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

    // NameChanged only. A rename changes no parentage, no ordering and no
    // geometry: EntityTreeModel turns NameChanged into a precise dataChanged,
    // and nothing in the scene layer reads GetName(). Emitting StructureChanged
    // here used to reset the whole tree model (clearing the tree selection,
    // which silently broke Delete/Copy/Duplicate right after a rename), rewalk
    // every name connection, walk the items map twice, reassign every z-value
    // and relayout every layout owner.
    emit NameChanged(name);
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

QList<UiElement*> UiElement::ChildElements() const
{
    QList<UiElement*> out;

    for (QObject* c : children())
    {
        if (auto* e = qobject_cast<UiElement*>(c))
            out.append(e);
    }

    return out;
}

int UiElement::ChildElementCount() const
{
    int n = 0;

    for (QObject* c : children())
    {
        if (qobject_cast<UiElement*>(c))
            ++n;
    }

    return n;
}

UiElement* UiElement::ChildElementAt(int row) const
{
    if (row < 0)
        return nullptr;

    int n = 0;

    for (QObject* c : children())
    {
        if (auto* e = qobject_cast<UiElement*>(c))
        {
            if (n == row)
                return e;

            ++n;
        }
    }

    return nullptr;
}

int UiElement::RowInParent() const
{
    auto* p = qobject_cast<UiElement*>(parent());

    if (!p)
        return -1;

    int n = 0;

    for (QObject* c : p->children())
    {
        if (auto* e = qobject_cast<UiElement*>(c))
        {
            if (e == this)
                return n;

            ++n;
        }
    }

    return -1;
}

UiElement* UiElement::FindById(const QUuid& target)
{
    if (target.isNull())
        return nullptr;

    if (id == target)
        return this;

    // findChildren is recursive over all QObject descendants.
    for (UiElement* e : findChildren<UiElement*>())
    {
        if (e && e->GetId() == target)
            return e;
    }

    return nullptr;
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

    // Same elements-only index space as ChildElementAt/RowInParent, minus self:
    // insertPos is the FINAL row this element should occupy.
    QList<UiElement*> ordered = newParent->ChildElements();
    const int currentIndex = ordered.indexOf(const_cast<UiElement*>(this));

    if (currentIndex >= 0)
        ordered.removeAt(currentIndex);

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
