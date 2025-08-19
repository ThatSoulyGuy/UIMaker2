#include "EntityTreeModel.hpp"
#include <QByteArray>

static UiElement* ElementAtRow(UiElement* parent, int row)
{
    int i = -1;

    for (QObject* c : parent->children())
    {
        if (auto* e = qobject_cast<UiElement*>(c))
        {
            ++i;

            if (i == row)
                return e;
        }
    }

    return nullptr;
}

static int RowOfElement(UiElement* element)
{
    if (!element || !element->parent())
        return 0;

    auto* p = qobject_cast<UiElement*>(element->parent());

    if (!p)
        return 0;

    int i = -1;

    for (QObject* c : p->children())
    {
        if (auto* e = qobject_cast<UiElement*>(c))
        {
            ++i;

            if (e == element)
                return i;
        }
    }

    return 0;
}

EntityTreeModel::EntityTreeModel(UiElement* root, QObject* parent) : QAbstractItemModel(parent), root(root)
{
    QObject::connect(root, &UiElement::StructureChanged, this, &EntityTreeModel::OnStructureChanged);
}

QModelIndex EntityTreeModel::index(int row, int column, const QModelIndex& parentIndex) const
{
    if (!hasIndex(row, column, parentIndex))
        return {};

    UiElement* parentElement = parentIndex.isValid() ? static_cast<UiElement*>(parentIndex.internalPointer()) : root;
    UiElement* child = ElementAtRow(parentElement, row);

    if (!child)
        return {};

    return createIndex(row, column, child);
}

QModelIndex EntityTreeModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return {};

    UiElement* element = static_cast<UiElement*>(child.internalPointer());
    UiElement* p = qobject_cast<UiElement*>(element->parent());

    if (!p || p == root)
        return {};

    return createIndex(RowOfElement(p), 0, p);
}

int EntityTreeModel::rowCount(const QModelIndex& parentIndex) const
{
    UiElement* parentElement = parentIndex.isValid() ? static_cast<UiElement*>(parentIndex.internalPointer()) : root;

    int count = 0;

    for (QObject* c : parentElement->children())
    {
        if (qobject_cast<UiElement*>(c))
            ++count;
    }

    return count;
}

QVariant EntityTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    UiElement* e = static_cast<UiElement*>(index.internalPointer());

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return e->GetName();

    return {};
}

bool EntityTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    UiElement* e = static_cast<UiElement*>(index.internalPointer());

    e->SetName(value.toString());

    emit dataChanged(index, index);

    return true;
}

Qt::ItemFlags EntityTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;

    if (!index.isValid())
        f = Qt::ItemIsDropEnabled;

    return f;
}

QStringList EntityTreeModel::mimeTypes() const
{
    return { "application/x-uielement-id" };
}

QMimeData* EntityTreeModel::mimeData(const QModelIndexList& indexes) const
{
    auto* mime = new QMimeData();

    if (indexes.isEmpty())
        return mime;

    UiElement* e = static_cast<UiElement*>(indexes.first().internalPointer());

    mime->setData("application/x-uielement-id", e->GetId().toString(QUuid::WithoutBraces).toUtf8());

    return mime;
}

bool EntityTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int, const QModelIndex& parentIndex)
{
    if (action != Qt::MoveAction)
        return false;

    if (!data->hasFormat("application/x-uielement-id"))
        return false;

    QString idString = QString::fromUtf8(data->data("application/x-uielement-id"));
    QUuid id = QUuid::fromString("{" + idString + "}");

    UiElement* parentElement = parentIndex.isValid() ? static_cast<UiElement*>(parentIndex.internalPointer()) : root;

    std::function<UiElement*(UiElement*)> find = [&](UiElement* n) -> UiElement*
    {
        if (n->GetId() == id)
            return n;

        for (QObject* c : n->children())
        {
            if (auto* e = qobject_cast<UiElement*>(c))
            {
                if (auto* fnd = find(e))
                    return fnd;
            }
        }

        return nullptr;
    };

    UiElement* moving = find(root);

    if (!moving || moving == parentElement)
        return false;

    int insertRow = row < 0 ? rowCount(parentIndex) : row;

    beginResetModel();
    moving->ReparentTo(parentElement, insertRow);
    endResetModel();

    emit HierarchyChanged();

    return true;
}

UiElement* EntityTreeModel::GetElementFromIndex(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return root;

    return static_cast<UiElement*>(idx.internalPointer());
}

QModelIndex EntityTreeModel::GetIndexFromElement(UiElement* element) const
{
    if (element == nullptr || element == root)
        return {};

    UiElement* parentElement = qobject_cast<UiElement*>(element->parent());
    QModelIndex parentIdx = {};

    if (parentElement && parentElement != root)
        parentIdx = GetIndexFromElement(parentElement);

    return index(RowOfElement(element), 0, parentIdx);
}

void EntityTreeModel::OnStructureChanged()
{
    beginResetModel();
    endResetModel();

    emit HierarchyChanged();
}
