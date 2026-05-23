#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include <QClipboard>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUndoCommand>
#include <QUndoStack>
#include <QGridLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QItemSelection>
#include <QSignalBlocker>
#include "scene/SceneElementItem.hpp"
#include "scene/SceneExporter.hpp"
#include "app/MainWindow.hpp"
#include "ui/ViewportWidget.hpp"
#include "./ui_mainwindow.h"

MainWindow* MainWindow::s_instance = nullptr;

// Whole-scene snapshot command. Retained as a fallback (e.g. for the menu
// "Load" path or other future blanket replacements); the interactive editor
// flows have moved to the delta-based commands below, which avoid the full
// scene rebuild that made big hierarchies freeze on mouseup.
class JsonSceneCommand : public QUndoCommand
{

public:

    JsonSceneCommand(MainWindow* mw, QByteArray before, QByteArray after, QString text) : QUndoCommand(std::move(text)), mw(mw), before(std::move(before)), after(std::move(after)) { }

    void undo() override
    {
        if (mw)
            mw->ApplySceneJson(before);
    }

    void redo() override
    {
        if (firstRedo)
        {
            firstRedo = false;
            return;
        }

        if (mw)
            mw->ApplySceneJson(after);
    }

private:

    MainWindow* mw;
    QByteArray before, after;
    bool firstRedo = true;
};

// Apply a TransformDelta to whichever element currently carries its id. Used
// by both undo() (applies the "before" pose) and redo() (the "after" pose).
// Touches only the affected elements - no scene rebuild - which is the whole
// point of the delta model.
static void ApplyTransformDelta(SceneDocument* doc, const TransformDelta& d, bool useAfter)
{
    if (!doc)
        return;

    UiElement* el = doc->FindById(d.id);
    if (!el)
        return;

    auto* xform = el->GetComponent<TransformComponent>();
    if (!xform)
        return;

    if (useAfter)
    {
        xform->SetPosition(d.afterPos);
        xform->SetRotationDegrees(d.afterRotation);
        xform->SetScale(d.afterScale);
    }
    else
    {
        xform->SetPosition(d.beforePos);
        xform->SetRotationDegrees(d.beforeRotation);
        xform->SetScale(d.beforeScale);
    }
}

// Delta-based undo command for gizmo transforms (translate/rotate/scale).
// Stores only the affected elements' (before, after) pose pairs keyed by
// UUID. undo/redo do O(touched elements) work and never rebuild the scene.
// The first redo() is a no-op because the drag handler already applied the
// "after" pose live on the document by the time the command is pushed.
class TransformDeltaCommand : public QUndoCommand
{

public:

    TransformDeltaCommand(SceneDocument* doc, QList<TransformDelta> deltas, QString text)
        : QUndoCommand(std::move(text)), doc(doc), deltas(std::move(deltas)) { }

    void undo() override
    {
        for (const TransformDelta& d : deltas)
            ApplyTransformDelta(doc, d, /*useAfter=*/false);
    }

    void redo() override
    {
        if (firstRedo)
        {
            firstRedo = false;
            return;
        }

        for (const TransformDelta& d : deltas)
            ApplyTransformDelta(doc, d, /*useAfter=*/true);
    }

private:

    SceneDocument* doc;
    QList<TransformDelta> deltas;
    bool firstRedo = true;
};

// One add-or-remove inside a StructuralCommand.
//
//   kind=Add    : on redo, recreate the subtree at (parentId, row) from json,
//                 preserving the recorded id; on undo, delete it by id.
//   kind=Remove : on redo, delete the element with this id; on undo, recreate
//                 the subtree at (parentId, row) from json, preserving the id.
//
// json is the FULL subtree serialisation, so recreate restores all descendants
// in one pass. row is the index in the parent's child list at the moment the
// op was recorded - on recreate the element is moved to that row via
// UiElement::ReparentTo which clamps to current child count.
struct StructuralOp
{
    enum Kind { Add, Remove };
    Kind        kind   = Add;
    QUuid       id;
    QUuid       parentId;
    int         row    = 0;
    QJsonObject json;
};

// Helper: recreate a subtree from json at (parent, row) preserving the id.
// Used by both Add::redo() and Remove::undo() - the operations are symmetric.
static UiElement* RecreateSubtree(SceneDocument* doc, const QJsonObject& json,
                                  const QUuid& parentId, int row)
{
    if (!doc)
        return nullptr;

    UiElement* parent = parentId.isNull() ? doc->GetRoot() : doc->FindById(parentId);
    if (!parent)
        parent = doc->GetRoot();
    if (!parent)
        return nullptr;

    UiElement* created = doc->CreateElementFromJson(json, parent, /*preserveIds=*/true);
    if (created && row >= 0)
        created->ReparentTo(parent, row);

    return created;
}

// Helper: delete the element currently carrying this id.
static void RemoveById(SceneDocument* doc, const QUuid& id)
{
    if (!doc)
        return;
    if (UiElement* el = doc->FindById(id))
        doc->DeleteElement(el);
}

// Delta-based undo command for structural changes (paste, cut, duplicate,
// delete - eventually reparent). Stores a list of StructuralOps recorded in
// the order they were performed. redo() replays forward, undo() walks the
// same list and applies the inverse of each op.
//
// Forward iteration on undo is deliberate: for Remove ops whose recreation
// uses recorded row indices, the rows are captured in ascending order at
// record time and walking forward keeps insertion math consistent. For Add
// ops the inverse (delete-by-id) is order-independent so forward iteration
// is fine there too.
class StructuralCommand : public QUndoCommand
{

public:

    StructuralCommand(SceneDocument* doc, QList<StructuralOp> ops, QString text)
        : QUndoCommand(std::move(text)), doc(doc), ops(std::move(ops)) { }

    void undo() override
    {
        for (const StructuralOp& op : ops)
        {
            if (op.kind == StructuralOp::Add)
                RemoveById(doc, op.id);
            else
                RecreateSubtree(doc, op.json, op.parentId, op.row);
        }
    }

    void redo() override
    {
        if (firstRedo)
        {
            firstRedo = false;
            return;
        }

        for (const StructuralOp& op : ops)
        {
            if (op.kind == StructuralOp::Add)
                RecreateSubtree(doc, op.json, op.parentId, op.row);
            else
                RemoveById(doc, op.id);
        }
    }

private:

    SceneDocument* doc;
    QList<StructuralOp> ops;
    bool firstRedo = true;
};

// Raw position of this element in its parent's QObject children list. This
// matches the index space that UiElement::ReparentTo expects for insertPos -
// it includes the parent's own Components, which sit before any child
// UiElements. Capture and replay use the same index space, so the value is
// stable across delete+recreate as long as the parent's component set is
// unchanged between the two (which is the steady-state in this editor).
static int RowInParent(UiElement* e)
{
    if (!e)
        return -1;
    auto* parent = qobject_cast<UiElement*>(e->parent());
    if (!parent)
        return -1;
    return parent->children().indexOf(e);
}


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::UIMaker2)
{
    s_instance = this;
    ui->setupUi(this);
    undoStack = new QUndoStack(this);

    document = new SceneDocument(this);

    // Replace the designer QGraphicsView with our custom ViewportWidget
    auto* oldView = ui->graphicsView;
    auto* layout = qobject_cast<QGridLayout*>(ui->Grid->layout());

    m_viewport = new ViewportWidget(ui->Grid);
    m_viewport->SetDocument(document);

    if (layout)
    {
        layout->replaceWidget(oldView, m_viewport);
        delete oldView;
    }

    // Connect transform signals to undo stack
    connect(m_viewport, &ViewportWidget::TransformCompleted, this, &MainWindow::onTransformCompleted);

    AttachScene(document->GetScene());

    BuildHierarchyDock();
    BuildPropertyDock();
    BuildToolbar();
    ConnectActions();

    UiElement* title = document->CreateTextElement("Title");
    title->GetComponent<TransformComponent>()->SetPosition(QPointF(100.0, 80.0));

    UiElement* startButton = document->CreateButtonElement("StartButton");
    startButton->GetComponent<TransformComponent>()->SetPosition(QPointF(100.0, 160.0));
}

MainWindow::~MainWindow()
{
    delete ui;
}

UiElement* MainWindow::CurrentElement() const
{
    if (!hierarchyModel || !hierarchySelection) return nullptr;
    return hierarchyModel->GetElementFromIndex(hierarchySelection->currentIndex());
}

QList<UiElement*> MainWindow::SelectedElements() const
{
    QList<UiElement*> result;

    if (!hierarchyModel || !hierarchySelection)
        return result;

    for (const QModelIndex& idx : hierarchySelection->selectedIndexes())
    {
        if (auto* e = hierarchyModel->GetElementFromIndex(idx))
        {
            if (!result.contains(e))
                result.append(e);
        }
    }

    return result;
}

void MainWindow::ApplySceneJson(const QByteArray& bytes)
{
    if (!document)
        return;

    // Save the selected element ids before LoadJson destroys the elements. Ids are
    // preserved across the JSON round-trip (see SceneDocument::CreateElementFromJson),
    // so the entire multiselection can be restored, not just one element by name.
    QList<QUuid> selectedIds;
    for (UiElement* e : document->GetSelectedElements())
    {
        if (e)
            selectedIds.append(e->GetId());
    }

    if (!document->LoadJson(bytes))
        return;

    // Don't call SetDocument or AttachScene - scene pointer hasn't changed,
    // and AttachScene triggers fitInView which recenters the viewport.

    delete hierarchyModel;
    hierarchyModel = new EntityTreeModel(document->GetRoot(), this);

    delete hierarchySelection;
    hierarchySelection = new QItemSelectionModel(hierarchyModel, this);

    hierarchyView->setModel(hierarchyModel);
    hierarchyView->setSelectionModel(hierarchySelection);
    WireHierarchySignals();

    // Restore selection by id.
    QList<UiElement*> restored;
    if (!selectedIds.isEmpty())
    {
        const QList<UiElement*> all = document->GetRoot()->findChildren<UiElement*>();
        for (const QUuid& id : selectedIds)
        {
            for (UiElement* e : all)
            {
                if (e->GetId() == id)
                {
                    restored.append(e);
                    break;
                }
            }
        }
    }

    if (!restored.isEmpty())
    {
        // SceneDocument::SelectionChanged listener mirrors this back to the tree view
        // and the property panel, so no need to touch them directly here.
        document->SetSelectedElements(restored);
    }
    else
    {
        propertyPanel->SetTarget(document->GetRoot());
    }
}

void MainWindow::onTransformCompleted(const QList<TransformDelta>& deltas, const QString& actionName)
{
    if (deltas.isEmpty())
        return;

    // The drag handler already applied the "after" pose live on every affected
    // element. The command stores per-element (before, after) pose pairs keyed
    // by UUID, so undo/redo touch only those elements and never rebuild the
    // scene. First redo() is a no-op (the change is already live).
    undoStack->push(new TransformDeltaCommand(document, deltas, actionName));
}

void MainWindow::BuildHierarchyDock()
{
    QWidget* contents = ui->ObjectHierarchyContents;
    auto* layout = new QVBoxLayout();

    layout->setContentsMargins(4,4,4,4);

    hierarchyView = new QTreeView(contents);
    hierarchyView->setHeaderHidden(true);
    hierarchyView->setExpandsOnDoubleClick(true);
    hierarchyView->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
    hierarchyView->setDragDropMode(QAbstractItemView::InternalMove);
    hierarchyView->setDefaultDropAction(Qt::MoveAction);
    hierarchyView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    hierarchyModel = new EntityTreeModel(document->GetRoot(), this);
    hierarchySelection = new QItemSelectionModel(hierarchyModel, this);
    hierarchyView->setModel(hierarchyModel);
    hierarchyView->setSelectionModel(hierarchySelection);

    WireHierarchySignals();
    layout->addWidget(hierarchyView);
    contents->setLayout(layout);
}

void MainWindow::BuildPropertyDock()
{
    QWidget* contents = ui->PropertyEditorContents;
    auto* layout = new QVBoxLayout(); layout->setContentsMargins(4,4,4,4);

    propertyPanel = new PropertyEditorPanel(contents);

    layout->addWidget(propertyPanel);
    contents->setLayout(layout);

    // Keep the active document's project root in sync with the root the user
    // establishes through the asset browse flow.
    connect(propertyPanel, &PropertyEditorPanel::ProjectRootChanged, this, [this](const QString& dir)
    {
        if (document)
            document->SetBaseDir(dir);
    });

    propertyPanel->SetTarget(document->GetRoot());
}

void MainWindow::BuildToolbar()
{
    transformToolbar = new QToolBar("Transform Tools", this);
    transformToolbar->setIconSize(QSize(24, 24));
    addToolBar(Qt::TopToolBarArea, transformToolbar);

    toolActionGroup = new QActionGroup(this);
    toolActionGroup->setExclusive(true);

    ui->ActionMove->setCheckable(true);
    ui->ActionMove->setChecked(true);
    ui->ActionMove->setShortcut(QKeySequence(Qt::Key_W));
    toolActionGroup->addAction(ui->ActionMove);
    transformToolbar->addAction(ui->ActionMove);

    ui->ActionRotate->setCheckable(true);
    ui->ActionRotate->setShortcut(QKeySequence(Qt::Key_E));
    toolActionGroup->addAction(ui->ActionRotate);
    transformToolbar->addAction(ui->ActionRotate);

    ui->ActionScale->setCheckable(true);
    ui->ActionScale->setShortcut(QKeySequence(Qt::Key_R));
    toolActionGroup->addAction(ui->ActionScale);
    transformToolbar->addAction(ui->ActionScale);

    // Connect to ToolManager instead of local SetToolMode
    connect(ui->ActionMove, &QAction::triggered, this, [this]() {
        m_viewport->GetToolManager()->SetActiveTool("translate");
        m_viewport->setDragMode(QGraphicsView::RubberBandDrag);
        m_viewport->viewport()->update();
    });

    connect(ui->ActionRotate, &QAction::triggered, this, [this]() {
        m_viewport->GetToolManager()->SetActiveTool("rotate");
        m_viewport->setDragMode(QGraphicsView::NoDrag);
        m_viewport->viewport()->update();
    });

    connect(ui->ActionScale, &QAction::triggered, this, [this]() {
        m_viewport->GetToolManager()->SetActiveTool("scale");
        m_viewport->setDragMode(QGraphicsView::NoDrag);
        m_viewport->viewport()->update();
    });
}

void MainWindow::ConnectActions()
{
    connect(ui->ActionAddText, &QAction::triggered, this, [this]()
    {
        UiElement* e = document->CreateTextElement("Text", nullptr);

        hierarchyModel->OnStructureChanged();

        auto idx = hierarchyModel->GetIndexFromElement(e); hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        propertyPanel->SetTarget(e);
    });

    connect(ui->ActionAddImage, &QAction::triggered, this, [this]()
    {
        UiElement* e = document->CreateImageElement("Image", nullptr);

        hierarchyModel->OnStructureChanged();

        auto idx = hierarchyModel->GetIndexFromElement(e); hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        propertyPanel->SetTarget(e);
    });

    connect(ui->ActionAddButton, &QAction::triggered, this, [this]()
    {
        UiElement* e = document->CreateButtonElement("Button", nullptr);

        hierarchyModel->OnStructureChanged();

        auto idx = hierarchyModel->GetIndexFromElement(e); hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        propertyPanel->SetTarget(e);
    });

    connect(ui->ActionAddStackLayout, &QAction::triggered, this, [this]()
    {
        UiElement* e = document->CreateStackLayoutElement("StackLayout", nullptr);

        hierarchyModel->OnStructureChanged();

        auto idx = hierarchyModel->GetIndexFromElement(e);
        hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        propertyPanel->SetTarget(e);
    });

    connect(ui->ActionAddGridLayout, &QAction::triggered, this, [this]()
    {
        UiElement* e = document->CreateGridLayoutElement("GridLayout", nullptr);

        hierarchyModel->OnStructureChanged();

        auto idx = hierarchyModel->GetIndexFromElement(e);
        hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        propertyPanel->SetTarget(e);
    });

    connect(ui->ActionAddScrollBox, &QAction::triggered, this, [this]()
    {
        UiElement* e = document->CreateScrollBoxElement("ScrollBox", nullptr);

        hierarchyModel->OnStructureChanged();

        auto idx = hierarchyModel->GetIndexFromElement(e);
        hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        propertyPanel->SetTarget(e);
    });

    auto connectAddAction = [this](QAction* action, const QString& name, auto createFn)
    {
        connect(action, &QAction::triggered, this, [this, name, createFn]()
        {
            UiElement* e = (document->*createFn)(name, nullptr);

            hierarchyModel->OnStructureChanged();

            auto idx = hierarchyModel->GetIndexFromElement(e);
            hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

            propertyPanel->SetTarget(e);
        });
    };

    connectAddAction(ui->ActionAddPanel, "Panel", &SceneDocument::CreatePanelElement);
    connectAddAction(ui->ActionAddProgressBar, "ProgressBar", &SceneDocument::CreateProgressBarElement);
    connectAddAction(ui->ActionAddToggle, "Toggle", &SceneDocument::CreateToggleElement);
    connectAddAction(ui->ActionAddDropdown, "Dropdown", &SceneDocument::CreateDropdownElement);
    connectAddAction(ui->ActionAddTextInput, "TextInput", &SceneDocument::CreateTextInputElement);
    connectAddAction(ui->ActionAddIcon, "Icon", &SceneDocument::CreateIconElement);
    connectAddAction(ui->ActionAddSprite, "Sprite", &SceneDocument::CreateSpriteElement);
    connectAddAction(ui->ActionAddTooltip, "Tooltip", &SceneDocument::CreateTooltipElement);
    connectAddAction(ui->ActionAddModal, "Modal", &SceneDocument::CreateModalElement);
    connectAddAction(ui->ActionAddTabContainer, "TabContainer", &SceneDocument::CreateTabContainerElement);
    connectAddAction(ui->ActionAddRadialMenu, "RadialMenu", &SceneDocument::CreateRadialMenuElement);
    connectAddAction(ui->ActionAddMinimap, "Minimap", &SceneDocument::CreateMinimapElement);
    connectAddAction(ui->ActionAddDragSlot, "DragSlot", &SceneDocument::CreateDragSlotElement);
    connectAddAction(ui->ActionAddListRepeater, "ListRepeater", &SceneDocument::CreateListRepeaterElement);

    connect(ui->ActionNew, &QAction::triggered, this, [this]()
    {
        delete document;

        document = new SceneDocument(this);
        // Fresh scene has no project root yet; clear any stale resolution base.
        document->SetBaseDir(QString());

        m_viewport->SetDocument(document);

        delete hierarchyModel;
        hierarchyModel = new EntityTreeModel(document->GetRoot(), this);

        delete hierarchySelection;
        hierarchySelection = new QItemSelectionModel(hierarchyModel, this);

        hierarchyView->setModel(hierarchyModel); hierarchyView->setSelectionModel(hierarchySelection);
        WireHierarchySignals();
        propertyPanel->SetTarget(document->GetRoot());
    });

    connect(ui->ActionExport, &QAction::triggered, this, [this]()
    {
        QString folder = QFileDialog::getExistingDirectory(this, "Export to Folder");

        if (folder.isEmpty())
            return;

        if (SceneExporter::ExportToFolder(document, folder))
        {
            // The exported folder is now the project root: assets live there
            // and subsequent relative-path edits resolve against it.
            document->SetBaseDir(folder);
            QMessageBox::information(this, "Export", "Scene exported successfully.");
        }
        else
            QMessageBox::warning(this, "Export Failed", "Could not export scene to folder.");
    });

    connect(ui->ActionBake, &QAction::triggered, this, [this]()
    {
        QString path = QFileDialog::getSaveFileName(this, "Bake Scene", QString(), "UI Binary (*.uibin)");

        if (path.isEmpty())
            return;

        if (SceneExporter::BakeToUiBin(document, path))
            QMessageBox::information(this, "Bake", "Scene baked to .uibin successfully.");
        else
            QMessageBox::warning(this, "Bake Failed", "Could not bake scene.");
    });

    connect(ui->ActionLoad, &QAction::triggered, this, [this]()
    {
        QString path = QFileDialog::getOpenFileName(this, "Load Scene JSON", QString(), "JSON (*.json)");

        if (path.isEmpty())
            return;

        QFile file(path);

        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(this, "Load Failed", QString("Could not open file:\n%1").arg(file.errorString()));
            return;
        }

        QByteArray json = file.readAll();
        file.close();

        SceneDocument* newDoc = new SceneDocument(this);

        // The JSON file's directory is the project root; asset paths inside it
        // are relative to here and resolved on demand (no JSON rewriting).
        newDoc->SetBaseDir(QFileInfo(path).absolutePath());

        if (!newDoc->LoadJson(json))
        {
            delete newDoc;
            QMessageBox::warning(this, "Load Failed", "Invalid or corrupt scene JSON.");
            return;
        }

        delete document;
        document = newDoc;

        m_viewport->SetDocument(document);
        AttachScene(document->GetScene());

        delete hierarchyModel;
        hierarchyModel = new EntityTreeModel(document->GetRoot(), this);

        delete hierarchySelection;
        hierarchySelection = new QItemSelectionModel(hierarchyModel, this);

        hierarchyView->setModel(hierarchyModel);
        hierarchyView->setSelectionModel(hierarchySelection);
        WireHierarchySignals();
        propertyPanel->SetTarget(document->GetRoot());
    });

    ui->ActionCopy->setShortcut(QKeySequence::Copy);
    ui->ActionPaste->setShortcut(QKeySequence::Paste);
    ui->ActionCut->setShortcut(QKeySequence::Cut);

    connect(ui->ActionCopy, &QAction::triggered, this, &MainWindow::DoCopy);
    connect(ui->ActionPaste, &QAction::triggered, this, &MainWindow::DoPaste);
    connect(ui->ActionCut, &QAction::triggered, this, &MainWindow::DoCut);

    ui->ActionDuplicate->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    connect(ui->ActionDuplicate, &QAction::triggered, this, &MainWindow::DoDuplicate);

    ui->ActionDelete->setShortcut(QKeySequence::Delete);
    connect(ui->ActionDelete, &QAction::triggered, this, &MainWindow::DoDelete);

    ui->ActionUndo_2->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    ui->ActionRedo_2->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));

    connect(ui->ActionUndo_2, &QAction::triggered, this, &MainWindow::DoUndo);
    connect(ui->ActionRedo_2, &QAction::triggered, this, &MainWindow::DoRedo);
}

void MainWindow::WireHierarchySignals()
{
    disconnect(hierarchySelection, nullptr, this, nullptr);
    disconnect(hierarchyModel, nullptr, this, nullptr);

    connect(hierarchySelection, &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection&, const QItemSelection&)
    {
        // Push the tree's selection into the document; the SceneDocument::SelectionChanged
        // listener takes care of mirroring back to the property panel and viewport.
        document->SetSelectedElements(SelectedElements());
    });

    connect(hierarchyModel, &EntityTreeModel::HierarchyChanged, this, [this]()
    {
        hierarchyView->expandAll();
    });
}


void MainWindow::AttachScene(QGraphicsScene* scene)
{
    QObject::disconnect(sceneRectChangedConnection);
    QObject::disconnect(sceneSelectionConnection);

    if (!scene) return;

    sceneSelectionConnection = connect(document, &SceneDocument::SelectionChanged, this,
        [this](const QList<UiElement*>& selected)
    {
        m_viewport->viewport()->update();

        // Mirror the document's selection into the tree view without re-triggering the
        // hierarchy -> document handler.
        QSignalBlocker blocker(hierarchySelection);
        QItemSelection itemSel;
        for (UiElement* e : selected)
        {
            const QModelIndex idx = hierarchyModel->GetIndexFromElement(e);
            if (idx.isValid())
                itemSel.select(idx, idx);
        }
        hierarchySelection->select(itemSel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        if (!selected.isEmpty())
            hierarchySelection->setCurrentIndex(hierarchyModel->GetIndexFromElement(selected.last()),
                                                QItemSelectionModel::Current | QItemSelectionModel::Rows);

        propertyPanel->SetTargets(selected);
    });

    // Fit view once on initial attach (new document / file load)
    QTimer::singleShot(0, this, [this, scene]()
    {
        if (!m_viewport->viewport()->size().isEmpty())
            m_viewport->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    });
}

static QList<UiElement*> FilterDeletable(const QList<UiElement*>& src, UiElement* root)
{
    // Remove root, slots, and any element that is a descendant of another selected
    // element (parent will recursively delete it).
    QList<UiElement*> out;

    for (UiElement* e : src)
    {
        if (!e || e == root || e->IsSlot())
            continue;

        bool hasSelectedAncestor = false;
        for (UiElement* other : src)
        {
            if (other == e || !other)
                continue;

            for (auto* p = qobject_cast<UiElement*>(e->parent()); p != nullptr; p = qobject_cast<UiElement*>(p->parent()))
            {
                if (p == other)
                {
                    hasSelectedAncestor = true;
                    break;
                }
            }

            if (hasSelectedAncestor)
                break;
        }

        if (!hasSelectedAncestor)
            out.append(e);
    }

    return out;
}

void MainWindow::DoCopy()
{
    const QList<UiElement*> selected = SelectedElements();

    if (selected.isEmpty())
        return;

    QJsonArray arr;
    for (UiElement* e : selected)
    {
        if (!e)
            continue;
        QJsonObject obj;
        e->ToJson(obj);
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);

    auto* mime = new QMimeData();
    mime->setData(kElementMime, bytes);
    mime->setText(QString::fromUtf8(bytes));

    QGuiApplication::clipboard()->setMimeData(mime);
}

void MainWindow::DoPaste()
{
    const QMimeData* mime = QGuiApplication::clipboard()->mimeData();

    if (!mime)
        return;

    QByteArray bytes;

    if (mime->hasFormat(kElementMime))
        bytes = mime->data(kElementMime);
    else if (mime->hasText())
        bytes = mime->text().toUtf8();
    else
        return;

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);

    if (err.error != QJsonParseError::NoError)
        return;

    UiElement* current = CurrentElement();
    UiElement* parent  = current ? qobject_cast<UiElement*>(current->parent()) : document->GetRoot();

    if (!parent)
        parent = document->GetRoot();

    QJsonArray arr;
    if (doc.isArray())
        arr = doc.array();
    else if (doc.isObject())
        arr.append(doc.object());
    else
        return;

    if (arr.isEmpty())
        return;

    // Create each element, then re-serialise it to capture the canonical JSON
    // including its freshly-assigned id. We record an Add op per created
    // top-level element so undo can delete-by-id and redo can recreate at the
    // same (parent, row) with the same id.
    QList<StructuralOp> ops;
    for (const QJsonValue& v : arr)
    {
        if (!v.isObject())
            continue;

        UiElement* created = document->CreateElementFromJson(v.toObject(), parent);
        if (!created)
            continue;

        StructuralOp op;
        op.kind     = StructuralOp::Add;
        op.id       = created->GetId();
        op.parentId = parent ? parent->GetId() : QUuid();
        op.row      = RowInParent(created);
        created->ToJson(op.json);
        ops.append(op);
    }

    if (!ops.isEmpty())
        undoStack->push(new StructuralCommand(document, ops, "Paste"));
}

void MainWindow::DoCut()
{
    const QList<UiElement*> targets = FilterDeletable(SelectedElements(), document->GetRoot());

    if (targets.isEmpty())
        return;

    DoCopy();

    // Snapshot each subtree (json + parent + row) BEFORE deletion so undo can
    // recreate them at their original locations.
    QList<StructuralOp> ops;
    for (UiElement* e : targets)
    {
        if (!e)
            continue;

        StructuralOp op;
        op.kind     = StructuralOp::Remove;
        op.id       = e->GetId();
        auto* p     = qobject_cast<UiElement*>(e->parent());
        op.parentId = p ? p->GetId() : QUuid();
        op.row      = RowInParent(e);
        e->ToJson(op.json);
        ops.append(op);
    }

    for (UiElement* e : targets)
        document->DeleteElement(e);

    if (!ops.isEmpty())
        undoStack->push(new StructuralCommand(document, ops, "Cut"));
}

void MainWindow::DoDuplicate()
{
    const QList<UiElement*> targets = FilterDeletable(SelectedElements(), document->GetRoot());

    if (targets.isEmpty())
        return;

    QList<StructuralOp> ops;
    for (UiElement* e : targets)
    {
        QJsonObject obj;
        e->ToJson(obj);

        UiElement* parent = qobject_cast<UiElement*>(e->parent());
        if (!parent)
            parent = document->GetRoot();

        UiElement* dup = document->CreateElementFromJson(obj, parent);
        if (!dup)
            continue;

        if (auto* t = dup->GetComponent<TransformComponent>())
            t->SetPosition(t->GetPosition() + QPointF(20.0, 20.0));

        dup->SetName(e->GetName() + " Copy");

        StructuralOp op;
        op.kind     = StructuralOp::Add;
        op.id       = dup->GetId();
        op.parentId = parent->GetId();
        op.row      = RowInParent(dup);
        dup->ToJson(op.json);
        ops.append(op);
    }

    if (!ops.isEmpty())
        undoStack->push(new StructuralCommand(document, ops, "Duplicate"));
}

void MainWindow::DoDelete()
{
    const QList<UiElement*> targets = FilterDeletable(SelectedElements(), document->GetRoot());

    if (targets.isEmpty())
        return;

    // Snapshot subtrees first (ascending row order is preserved by walking
    // SelectedElements in the order Qt returned them; we rely on that for
    // correct re-insertion on undo). Then perform the deletions.
    QList<StructuralOp> ops;
    for (UiElement* e : targets)
    {
        if (!e)
            continue;

        StructuralOp op;
        op.kind     = StructuralOp::Remove;
        op.id       = e->GetId();
        auto* p     = qobject_cast<UiElement*>(e->parent());
        op.parentId = p ? p->GetId() : QUuid();
        op.row      = RowInParent(e);
        e->ToJson(op.json);
        ops.append(op);
    }

    for (UiElement* e : targets)
        document->DeleteElement(e);

    if (!ops.isEmpty())
        undoStack->push(new StructuralCommand(document, ops, "Delete"));
}

void MainWindow::DoUndo()
{
    if (undoStack)
        undoStack->undo();
}

void MainWindow::DoRedo()
{
    if (undoStack)
        undoStack->redo();
}
