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

protected:

    // Offers to save when the undo stack is dirty. Until this existed, closing
    // the window threw away every unsaved edit with no prompt at all.
    void closeEvent(QCloseEvent* event) override;

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

    // Rendering-model menu: set the checkmarks and unit label from the current
    // PixelModel (a document property, so it is not persisted to QSettings).
    void SyncRenderModelChecks();

    // Finish a "calibrate to resolution" pick: derive the pixel unit from the
    // clicked square image element's scene size / texture resolution.
    void OnCalibrationPick(UiElement* element);
    void WireHierarchySignals();
    void AttachScene(QGraphicsScene* scene);
    void FinishAddElement(UiElement* e, const QString& name);

    // Load a scene .json, swapping it in as the active document and rewiring
    // the tree/property/viewport. Returns false (and shows a warning) on a
    // read or parse failure. Shared by File>Load and the reopen-on-startup path.
    bool OpenSceneFile(const QString& path);

    // Write scene.json (plus assets) back to the document's own project root,
    // with no dialog and no confirmation popup. Falls back to the Export flow
    // when there is no root yet. Marks the undo stack clean on success.
    bool SaveScene();

    // Window title tracks the current file and the modified state via the
    // "[*]" placeholder that QWidget::setWindowModified drives.
    void UpdateTitle();

    // Save / Discard / Cancel prompt for anything that is about to destroy the
    // current document (New, Load). Returns false when the caller must abort.
    bool ConfirmDiscardChanges();

    // Where a newly added or pasted element should go: the selected element if
    // it can hold children, otherwise the document root. Never returns a slot
    // (slots are managed by their owning TabContainer/RadialMenu) and never
    // returns null.
    UiElement* ContainerForNewElement() const;

    // "Panel", then "Panel 2", "Panel 3", ... among that parent's child
    // elements. UiElement::AddChild takes the name verbatim, so without this
    // three added panels are all literally named "Panel".
    QString UniqueChildName(UiElement* parent, const QString& base) const;

    // Centre of what the user is currently looking at, expressed in the given
    // parent's local coordinates. New elements land there instead of at scene
    // (0,0), which is off-screen whenever the canvas is panned.
    QPointF ViewCentreInParent(UiElement* parent) const;

    // Offset an element that would sit exactly on top of a sibling, so added
    // and pasted elements are visibly distinct. Leaves an intentional hand-made
    // stack alone, and does nothing under a layout parent.
    void NudgeOffSiblings(UiElement* e) const;

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

    QActionGroup* m_renderGroup = nullptr;
    QAction* m_renderContinuousAction = nullptr;
    QAction* m_renderPixelAction = nullptr;
    QAction* m_pixelUnitAction = nullptr;

    QMetaObject::Connection sceneSelectionConnection;

    // Guards the document -> tree half of the selection round trip. A
    // QSignalBlocker cannot be used here: QTreeView repaints its selection and
    // autoscrolls in response to the selection model's own signals, so blocking
    // them updates the internal state and never schedules the repaint.
    bool syncingTreeSelection = false;

    // Absolute path of the scene.json backing this document, empty for a
    // document that has never been saved or loaded. Drives both the window
    // title and whether Save needs to ask for a location.
    QString currentPath;

    QUndoStack* undoStack = nullptr;

    static constexpr const char* kElementMime = "application/x-uimaker2-element";
};

#endif
