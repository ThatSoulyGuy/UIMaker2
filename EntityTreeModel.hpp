#ifndef ENTITYTREEMODEL_HPP
#define ENTITYTREEMODEL_HPP

#include <QAbstractItemModel>
#include <QMimeData>
#include "EntityComponentSystem.hpp"

class EntityTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    explicit EntityTreeModel(UiElement* root, QObject* parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex&) const override { return 1; }

    QVariant data(const QModelIndex& index, int role) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    UiElement* GetElementFromIndex(const QModelIndex& idx) const;
    QModelIndex GetIndexFromElement(UiElement* element) const;

signals:

    void HierarchyChanged();

public slots:

    void OnStructureChanged();

private:

    UiElement* root;
};

#endif
