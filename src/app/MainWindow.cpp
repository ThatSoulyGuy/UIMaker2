#include <algorithm>
#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUndoCommand>
#include <QUndoStack>
#include <QGridLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QMenu>
#include <QMenuBar>
#include <QInputDialog>
#include <QItemSelection>
#include <QSignalBlocker>
#include "core/GridSnap.hpp"
#include "core/UiElement.hpp"
#include "core/Component.hpp"
#include "components/TransformComponent.hpp"
#include "tools/ToolManager.hpp"
#include "scene/SceneElementItem.hpp"
#include "scene/SceneExporter.hpp"
#include "scene/SceneDocument.hpp"
#include "app/MainWindow.hpp"
#include "ui/EntityTreeModel.hpp"
#include "ui/PropertyEditorPanel.hpp"
#include "ui/ViewportWidget.hpp"
#include "./ui_mainwindow.h"

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
// in one pass. row is the index among the parent's child *elements* at the
// moment the op was recorded (see RowInParent below) - on recreate the element
// is moved to that row via UiElement::ReparentTo which clamps out-of-range
// values to append.
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
// delete - eventually reparent). Stores a list of StructuralOps. redo()
// replays forward, undo() walks the same list and applies the inverse of
// each op.
//
// Forward iteration on undo is deliberate: for Remove ops whose recreation
// uses recorded row indices, the ops are sorted into ascending (parent, row)
// order before the command is built (see SortOpsForReplay) and walking
// forward keeps insertion math consistent. For Add ops the inverse
// (delete-by-id) is order-independent so forward iteration is fine there
// too.
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

// Undo command for a hierarchy drag-drop reparent. Rows are final indices
// among the parent's child elements (the index space UiElement::ReparentTo
// uses), so replay is unaffected by the parents' component sets. A null
// parent id means the document root.
class ReparentCommand : public QUndoCommand
{

public:

    ReparentCommand(SceneDocument* doc, const QUuid& elementId,
                    const QUuid& oldParentId, int oldRow,
                    const QUuid& newParentId, int newRow)
        : QUndoCommand(QStringLiteral("Reparent")), doc(doc), elementId(elementId),
          oldParentId(oldParentId), oldRow(oldRow), newParentId(newParentId), newRow(newRow) { }

    void undo() override
    {
        Move(oldParentId, oldRow);
    }

    void redo() override
    {
        if (firstRedo)
        {
            firstRedo = false;
            return;
        }

        Move(newParentId, newRow);
    }

private:

    void Move(const QUuid& parentId, int row)
    {
        if (!doc)
            return;

        UiElement* element = doc->FindById(elementId);
        UiElement* parent  = parentId.isNull() ? doc->GetRoot() : doc->FindById(parentId);

        if (element && parent)
            element->ReparentTo(parent, row);
    }

    SceneDocument* doc;
    QUuid elementId;
    QUuid oldParentId;
    int oldRow;
    QUuid newParentId;
    int newRow;
    bool firstRedo = true;
};

// Undo command for property-panel edits. Stores one (before, after) pair per
// touched object, resolved by element id + component kind so undo/redo work
// on whichever objects currently carry those ids. Consecutive edits of the
// same property on the same objects merge, so a spinbox drag or per-keystroke
// text commit collapses into one undo step.
class PropertyEditCommand : public QUndoCommand
{

public:

    PropertyEditCommand(SceneDocument* doc, QList<PropertyEditRecord> recordsIn)
        : QUndoCommand(), doc(doc), records(std::move(recordsIn))
    {
        setText(QStringLiteral("Edit %1").arg(QString::fromLatin1(records.value(0).propName)));

        for (const PropertyEditRecord& r : records)
            key += r.elementId.toString() + QLatin1Char('/') + r.componentKind + QLatin1Char('/') + QString::fromLatin1(r.propName) + QLatin1Char(';');
    }

    int id() const override { return kCommandId; }

    bool mergeWith(const QUndoCommand* other) override
    {
        auto* o = static_cast<const PropertyEditCommand*>(other);

        if (o->key != key || o->records.size() != records.size())
            return false;

        for (int i = 0; i < records.size(); ++i)
            records[i].after = o->records[i].after;

        return true;
    }

    void undo() override
    {
        Apply(/*useAfter=*/false);
    }

    void redo() override
    {
        if (firstRedo)
        {
            firstRedo = false;
            return;
        }

        Apply(/*useAfter=*/true);
    }

private:

    static constexpr int kCommandId = 0x504F;

    void Apply(bool useAfter)
    {
        if (!doc)
            return;

        for (const PropertyEditRecord& r : records)
        {
            UiElement* el = doc->FindById(r.elementId);
            if (!el)
                continue;

            const QVariant& v = useAfter ? r.after : r.before;

            if (r.componentKind.isEmpty())
            {
                el->setProperty(r.propName.constData(), v);
                continue;
            }

            for (Component* c : el->GetComponents())
            {
                if (c->GetTypeName() == r.componentKind)
                {
                    c->setProperty(r.propName.constData(), v);
                    break;
                }
            }
        }
    }

    SceneDocument* doc;
    QList<PropertyEditRecord> records;
    QString key;
    bool firstRedo = true;
};

// Undo replay walks Remove ops forward and re-inserts each subtree at its
// recorded row, which is only correct in ascending (parent, row) order. The
// capture order is the selection order - arbitrary click order - so sort
// before building the command.
static void SortOpsForReplay(QList<StructuralOp>& ops)
{
    std::sort(ops.begin(), ops.end(), [](const StructuralOp& a, const StructuralOp& b)
    {
        if (a.parentId != b.parentId)
            return a.parentId < b.parentId;

        return a.row < b.row;
    });
}

// Position of this element among its parent's child *elements* (components
// are not counted). This matches the index space UiElement::ReparentTo takes
// for insertPos, and unlike a raw QObject children index it stays stable
// across delete+recreate even if the parent's component set changes between
// the two.
static int RowInParent(UiElement* e)
{
    if (!e)
        return -1;

    auto* parent = qobject_cast<UiElement*>(e->parent());
    if (!parent)
        return -1;

    int row = 0;

    for (QObject* c : parent->children())
    {
        if (auto* el = qobject_cast<UiElement*>(c))
        {
            if (el == e)
                return row;
            ++row;
        }
    }

    return -1;
}


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::UIMaker2)
{
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
    BuildViewMenu();
    ConnectActions();

    // Reopen the scene the user last had open; fall back to seeding demo
    // content on a fresh install or if that file is missing/unreadable.
    QSettings settings;
    const QString lastFile = settings.value(QStringLiteral("io/lastFile")).toString();

    if (lastFile.isEmpty() || !QFileInfo::exists(lastFile) || !OpenSceneFile(lastFile))
    {
        UiElement* title = document->CreateTextElement("Title");
        title->GetComponent<TransformComponent>()->SetPosition(QPointF(100.0, 80.0));

        UiElement* startButton = document->CreateButtonElement("StartButton");
        startButton->GetComponent<TransformComponent>()->SetPosition(QPointF(100.0, 160.0));
    }
}

bool MainWindow::OpenSceneFile(const QString& path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Load Failed", QString("Could not open file:\n%1").arg(file.errorString()));
        return false;
    }

    const QByteArray json = file.readAll();
    file.close();

    SceneDocument* newDoc = new SceneDocument(this);

    // The JSON file's directory is the project root; asset paths inside it
    // are relative to here and resolved on demand (no JSON rewriting).
    newDoc->SetBaseDir(QFileInfo(path).absolutePath());

    if (!newDoc->LoadJson(json))
    {
        delete newDoc;
        QMessageBox::warning(this, "Load Failed", "Invalid or corrupt scene JSON.");
        return false;
    }

    // Undo commands hold raw pointers into the old document; drop them before
    // it goes away or Ctrl+Z would dereference freed memory.
    undoStack->clear();

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

    return true;
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

    // The panel has already applied the change; the command's first redo() is
    // a no-op and consecutive edits of the same property merge (see
    // PropertyEditCommand).
    connect(propertyPanel, &PropertyEditorPanel::PropertyChangeApplied, this, [this](const QList<PropertyEditRecord>& records)
    {
        undoStack->push(new PropertyEditCommand(document, records));
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

void MainWindow::FinishAddElement(UiElement* e, const QString& name)
{
    if (!e)
        return;

    // The element already exists in the document; record an Add op so undo can
    // delete it by id and redo can recreate it in place (same pattern as
    // paste/duplicate - the command's first redo() is a no-op).
    StructuralOp op;
    op.kind     = StructuralOp::Add;
    op.id       = e->GetId();
    auto* p     = qobject_cast<UiElement*>(e->parent());
    op.parentId = p ? p->GetId() : QUuid();
    op.row      = RowInParent(e);
    e->ToJson(op.json);

    undoStack->push(new StructuralCommand(document, { op }, "Add " + name));

    hierarchyModel->OnStructureChanged();

    auto idx = hierarchyModel->GetIndexFromElement(e);
    hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    propertyPanel->SetTarget(e);
}

void MainWindow::BuildViewMenu()
{
    // Restore persisted snapping state into the global GridSnap before building
    // the menu, so the checkmarks reflect it.
    QSettings settings;
    GridSnap::SetDivisions(settings.value(QStringLiteral("snap/divX"), 16).toInt(),
                           settings.value(QStringLiteral("snap/divY"), 16).toInt());
    GridSnap::SetEnabled(settings.value(QStringLiteral("snap/enabled"), false).toBool());

    QMenu* viewMenu = menuBar()->addMenu("View");
    QMenu* snapMenu = viewMenu->addMenu("Snapping");

    m_snapGroup = new QActionGroup(this);
    m_snapGroup->setExclusive(true);

    // Each preset carries its per-axis division count in data() (0,0 = Off).
    auto addPreset = [this, snapMenu](const QString& label, int dx, int dy)
    {
        QAction* a = snapMenu->addAction(label);
        a->setCheckable(true);
        a->setData(QPoint(dx, dy));
        m_snapGroup->addAction(a);
        m_snapPresetActions.append(a);

        connect(a, &QAction::triggered, this, [this, dx, dy]()
        {
            if (dx <= 0 || dy <= 0)
                GridSnap::SetEnabled(false);
            else
            {
                GridSnap::SetDivisions(dx, dy);
                GridSnap::SetEnabled(true);
            }

            SaveSnapSettings();
            SyncSnapChecks();

            if (m_viewport)
                m_viewport->viewport()->update();
        });
    };

    addPreset("Off", 0, 0);
    snapMenu->addSeparator();
    addPreset("4 x 4", 4, 4);
    addPreset("8 x 8", 8, 8);
    addPreset("16 x 16", 16, 16);
    addPreset("32 x 32", 32, 32);
    addPreset("64 x 64", 64, 64);
    // Minecraft's GUI is laid out on a 320x240 base resolution (the minimum it
    // scales its screen down to); snapping to that grid matches its UI pixels.
    addPreset("Minecraft (320 x 240)", 320, 240);
    snapMenu->addSeparator();

    m_snapCustomAction = snapMenu->addAction("Custom...");
    m_snapCustomAction->setCheckable(true);
    m_snapGroup->addAction(m_snapCustomAction);

    connect(m_snapCustomAction, &QAction::triggered, this, [this]()
    {
        bool ok = false;
        const int n = QInputDialog::getInt(this, "Custom Grid",
            "Number of divisions per axis:", qMax(1, GridSnap::DivisionsX()), 1, 4096, 1, &ok);

        if (!ok)
        {
            // Cancelled: restore the checkmark to the real current state.
            SyncSnapChecks();
            return;
        }

        GridSnap::SetDivisions(n, n);
        GridSnap::SetEnabled(true);
        SaveSnapSettings();
        SyncSnapChecks();

        if (m_viewport)
            m_viewport->viewport()->update();
    });

    SyncSnapChecks();
}

void MainWindow::SaveSnapSettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("snap/enabled"), GridSnap::Enabled());
    settings.setValue(QStringLiteral("snap/divX"), GridSnap::DivisionsX());
    settings.setValue(QStringLiteral("snap/divY"), GridSnap::DivisionsY());
}

void MainWindow::SyncSnapChecks()
{
    const bool enabled = GridSnap::Enabled();
    const QPoint current(GridSnap::DivisionsX(), GridSnap::DivisionsY());

    QAction* match = nullptr;

    for (QAction* a : m_snapPresetActions)
    {
        const QPoint d = a->data().toPoint();

        if (d.x() <= 0 || d.y() <= 0)   // the Off entry
        {
            if (!enabled)
                match = a;
        }
        else if (enabled && d == current)
        {
            match = a;
        }
    }

    // Snapping on with no preset match -> it's a custom division count.
    if (!match && enabled && m_snapCustomAction)
    {
        m_snapCustomAction->setText(QString("Custom (%1 x %2)...").arg(current.x()).arg(current.y()));
        match = m_snapCustomAction;
    }
    else if (m_snapCustomAction)
    {
        m_snapCustomAction->setText("Custom...");
    }

    if (match)
        match->setChecked(true);
}

void MainWindow::ConnectActions()
{
    auto connectAddAction = [this](QAction* action, const QString& name, auto createFn)
    {
        connect(action, &QAction::triggered, this, [this, name, createFn]()
        {
            FinishAddElement((document->*createFn)(name, nullptr), name);
        });
    };

    connectAddAction(ui->ActionAddText, "Text", &SceneDocument::CreateTextElement);
    connectAddAction(ui->ActionAddImage, "Image", &SceneDocument::CreateImageElement);
    connectAddAction(ui->ActionAddButton, "Button", &SceneDocument::CreateButtonElement);
    connectAddAction(ui->ActionAddStackLayout, "StackLayout", &SceneDocument::CreateStackLayoutElement);
    connectAddAction(ui->ActionAddGridLayout, "GridLayout", &SceneDocument::CreateGridLayoutElement);
    connectAddAction(ui->ActionAddScrollBox, "ScrollBox", &SceneDocument::CreateScrollBoxElement);
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
        // Undo commands hold raw pointers into the old document; drop them
        // before it goes away or Ctrl+Z would dereference freed memory.
        undoStack->clear();

        delete document;

        document = new SceneDocument(this);
        // Fresh scene has no project root yet; clear any stale resolution base.
        document->SetBaseDir(QString());

        m_viewport->SetDocument(document);
        AttachScene(document->GetScene());

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
        QSettings settings;
        QString folder = QFileDialog::getExistingDirectory(this, "Export to Folder",
            settings.value(QStringLiteral("io/lastDir")).toString());

        if (folder.isEmpty())
            return;

        if (SceneExporter::ExportToFolder(document, folder))
        {
            // The exported folder is now the project root: assets live there
            // and subsequent relative-path edits resolve against it.
            document->SetBaseDir(folder);
            settings.setValue(QStringLiteral("io/lastDir"), folder);
            settings.setValue(QStringLiteral("io/lastFile"), QDir(folder).filePath("scene.json"));
            QMessageBox::information(this, "Export", "Scene exported successfully.");
        }
        else
            QMessageBox::warning(this, "Export Failed", "Could not export scene to folder.");
    });

    connect(ui->ActionBake, &QAction::triggered, this, [this]()
    {
        QSettings settings;
        QString path = QFileDialog::getSaveFileName(this, "Bake Scene",
            settings.value(QStringLiteral("io/lastDir")).toString(), "UI Binary (*.uibin)");

        if (path.isEmpty())
            return;

        if (SceneExporter::BakeToUiBin(document, path))
        {
            settings.setValue(QStringLiteral("io/lastDir"), QFileInfo(path).absolutePath());
            QMessageBox::information(this, "Bake", "Scene baked to .uibin successfully.");
        }
        else
            QMessageBox::warning(this, "Bake Failed", "Could not bake scene.");
    });

    connect(ui->ActionLoad, &QAction::triggered, this, [this]()
    {
        QSettings settings;
        QString path = QFileDialog::getOpenFileName(this, "Load Scene JSON",
            settings.value(QStringLiteral("io/lastDir")).toString(), "JSON (*.json)");

        if (path.isEmpty())
            return;

        if (OpenSceneFile(path))
        {
            settings.setValue(QStringLiteral("io/lastDir"), QFileInfo(path).absolutePath());
            settings.setValue(QStringLiteral("io/lastFile"), path);
        }
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

    // The model has already performed the reparent; the command's first
    // redo() is a no-op.
    connect(hierarchyModel, &EntityTreeModel::ElementReparented, this,
        [this](const QUuid& elementId, const QUuid& oldParentId, int oldRow, const QUuid& newParentId, int newRow)
    {
        undoStack->push(new ReparentCommand(document, elementId, oldParentId, oldRow, newParentId, newRow));
    });
}


void MainWindow::AttachScene(QGraphicsScene* scene)
{
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

    // Fit the design canvas once on initial attach (new document / file load).
    // Fitting the canvas rather than the large pasteboard scene rect keeps the
    // 1920x1080 canvas framed instead of shrinking it to a speck.
    QTimer::singleShot(0, this, [this]()
    {
        if (document && !m_viewport->viewport()->size().isEmpty())
            m_viewport->fitInView(document->GetCanvasRect(), Qt::KeepAspectRatio);
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
    {
        SortOpsForReplay(ops);
        undoStack->push(new StructuralCommand(document, ops, "Cut"));
    }
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

    // Snapshot subtrees first, then perform the deletions. The ops are sorted
    // into (parent, row) order before the command is built - selection order
    // is arbitrary and replay depends on ascending rows.
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
    {
        SortOpsForReplay(ops);
        undoStack->push(new StructuralCommand(document, ops, "Delete"));
    }
}

void MainWindow::DoUndo()
{
    if (!undoStack)
        return;

    undoStack->undo();

    // Property writes land synchronously but notify through queued
    // connections, and the panel defers rebuilds while one of its editors
    // has focus - which would leave stale text a later keystroke re-applies.
    // Refresh unconditionally instead.
    if (propertyPanel)
        propertyPanel->RefreshTargets();
}

void MainWindow::DoRedo()
{
    if (!undoStack)
        return;

    undoStack->redo();

    if (propertyPanel)
        propertyPanel->RefreshTargets();
}
