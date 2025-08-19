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

private slots:

    bool eventFilter(QObject*, QEvent*);

private:

    void BuildHierarchyDock();
    void BuildPropertyDock();
    void ConnectActions();
    void WireHierarchySignals();
    void AttachScene(QGraphicsScene* scene);

    void ZoomAt(const QPoint&, double);

    void FitItem(QGraphicsItem*);

    bool isPanning = false;
    QPoint lastPanPoint;
    double minZoom = 0.05;
    double maxZoom = 20.0;

    Ui::UIMaker2* ui;

    SceneDocument* document;
    EntityTreeModel* hierarchyModel;
    QItemSelectionModel* hierarchySelection;
    QTreeView* hierarchyView;
    PropertyEditorPanel* propertyPanel;

    QMetaObject::Connection sceneRectChangedConnection;
    QMetaObject::Connection sceneSelectionConnection;
};

#endif
