#ifndef UI_ENTITYTREEMODEL_HPP
#define UI_ENTITYTREEMODEL_HPP

#include <QAbstractItemModel>
#include <QMimeData>
#include <QUuid>

class UiElement;

class EntityTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    explicit EntityTreeModel(UiElement* root, QObject* parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex&) const override;

    QVariant data(const QModelIndex& index, int role) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    Qt::DropActions supportedDropActions() const override;

    UiElement* GetElementFromIndex(const QModelIndex& idx) const;
    QModelIndex GetIndexFromElement(UiElement* element) const;

signals:

    void HierarchyChanged();

    // Emitted after a successful drag-drop reparent so MainWindow can record
    // an undo command. Rows are FINAL indices among the parent's child
    // elements (the index space UiElement::ReparentTo uses); a null parent id
    // means the tree root.
    void ElementReparented(const QUuid& elementId,
                           const QUuid& oldParentId, int oldRow,
                           const QUuid& newParentId, int newRow);

public slots:

    void OnStructureChanged();

private:

    void ConnectNameSignals(UiElement* node);

    UiElement* root;
};

#endif
