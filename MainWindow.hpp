#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTreeView>
#include <QGraphicsView>
#include "SceneDocument.hpp"
#include "EntityTreeModel.hpp"
#include "PropertyEditorPanel.hpp"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class UIMaker2;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget* = nullptr);
    ~MainWindow();

private:

    void BuildHierarchyDock();
    void BuildPropertyDock();
    void ConnectActions();
    void AttachScene(QGraphicsScene* scene);

    Ui::UIMaker2* ui;

    SceneDocument* document;
    EntityTreeModel* hierarchyModel;
    QItemSelectionModel* hierarchySelection;
    QTreeView* hierarchyView;
    PropertyEditorPanel* propertyPanel;

    QMetaObject::Connection sceneRectChangedConnection;
};

#endif
