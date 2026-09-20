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

    // ---- The elements-only row index space --------------------------------
    //
    // QObject::children() interleaves this element's Components with its child
    // UiElements. Everywhere the editor talks about a "row" it means a position
    // among the child ELEMENTS only, components not counted. That single
    // definition is what EntityTreeModel's rows, MainWindow's StructuralOps and
    // ReparentTo's insertPos all refer to, and it used to be hand-copied as
    // three separate walks in three translation units that merely happened to
    // agree. These are the one definition; do not reintroduce a local copy.
    //
    // Computed rather than cached: ReparentTo churns the child list hard
    // (setParent(nullptr)/setParent(p) on every sibling), and a cache that can
    // silently disagree with the QObject tree is worse than a short walk.

    QList<UiElement*> ChildElements() const;

    int ChildElementCount() const;

    // Null when row is out of range.
    UiElement* ChildElementAt(int row) const;

    // This element's position among its parent's child elements, or -1 when it
    // has no UiElement parent (the document root, or a detached element).
    int RowInParent() const;

    // This element or the descendant carrying this id, else null. The one
    // definition; SceneDocument::FindById and the tree's drop handler both
    // used to hand-roll their own recursive search.
    UiElement* FindById(const QUuid& id);

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
