#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTreeView>
#include <QUndoStack>
#include <QToolBar>
#include <QActionGroup>
#include "scene/SceneDocument.hpp"
#include "ui/EntityTreeModel.hpp"
#include "ui/PropertyEditorPanel.hpp"

class ViewportWidget;

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

    void ApplySceneJson(const QByteArray& bytes);

    static MainWindow* Instance() { return s_instance; }

private:
    static MainWindow* s_instance;

private slots:

    void onTransformCompleted(const QByteArray& before, const QByteArray& after, const QString& actionName);

private:

    void BuildHierarchyDock();
    void BuildPropertyDock();
    void BuildToolbar();
    void ConnectActions();
    void WireHierarchySignals();
    void AttachScene(QGraphicsScene* scene);

    UiElement* CurrentElement() const;

    void DoCopy();
    void DoPaste();
    void DoCut();
    void DoDuplicate();
    void DoDelete();
    void DoUndo();
    void DoRedo();

    Ui::UIMaker2* ui;
    ViewportWidget* m_viewport = nullptr;

    SceneDocument* document;
    EntityTreeModel* hierarchyModel;
    QItemSelectionModel* hierarchySelection;
    QTreeView* hierarchyView;
    PropertyEditorPanel* propertyPanel;

    QToolBar* transformToolbar = nullptr;
    QActionGroup* toolActionGroup = nullptr;

    QMetaObject::Connection sceneRectChangedConnection;
    QMetaObject::Connection sceneSelectionConnection;

    QUndoStack* undoStack = nullptr;

    static constexpr const char* kElementMime = "application/x-uimaker2-element";
};

#endif
