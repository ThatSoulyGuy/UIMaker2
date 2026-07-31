#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTreeView>
#include <QUndoStack>
#include <QToolBar>
#include <QActionGroup>
#include <QList>
#include "scene/TransformDelta.hpp"

class ViewportWidget;
class UiElement;
class SceneDocument;
class EntityTreeModel;
class PropertyEditorPanel;
class QAction;

QT_BEGIN_NAMESPACE
class QGraphicsScene;
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

    void onTransformCompleted(const QList<TransformDelta>& deltas, const QString& actionName);

private:

    void BuildHierarchyDock();
    void BuildPropertyDock();
    void BuildToolbar();
    void BuildViewMenu();
    void ConnectActions();

    // Grid-snapping menu helpers: persist the current GridSnap state to
    // QSettings, and set the checkmark on whichever menu item matches it.
    void SaveSnapSettings();
    void SyncSnapChecks();
    void WireHierarchySignals();
    void AttachScene(QGraphicsScene* scene);
    void FinishAddElement(UiElement* e, const QString& name);

    // Load a scene .json, swapping it in as the active document and rewiring
    // the tree/property/viewport. Returns false (and shows a warning) on a
    // read or parse failure. Shared by File>Load and the reopen-on-startup path.
    bool OpenSceneFile(const QString& path);

    UiElement* CurrentElement() const;
    QList<UiElement*> SelectedElements() const;

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

    // View>Snapping actions. Preset/off actions carry their divisions in
    // QAction::data() as a QPoint (0,0 = Off); the custom action is tracked
    // separately so SyncSnapChecks can relabel it.
    QActionGroup* m_snapGroup = nullptr;
    QList<QAction*> m_snapPresetActions;
    QAction* m_snapCustomAction = nullptr;

    QMetaObject::Connection sceneSelectionConnection;

    QUndoStack* undoStack = nullptr;

    static constexpr const char* kElementMime = "application/x-uimaker2-element";
};

#endif
