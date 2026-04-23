#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include <QClipboard>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUndoCommand>
#include <QUndoStack>
#include <QGridLayout>
#include <QFileDialog>
#include <QFileInfo>
#include "scene/SceneElementItem.hpp"
#include "scene/SceneExporter.hpp"
#include "app/MainWindow.hpp"
#include "ui/ViewportWidget.hpp"
#include "./ui_mainwindow.h"

MainWindow* MainWindow::s_instance = nullptr;

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
        if (mw)
            mw->ApplySceneJson(after);
    }

private:

    MainWindow* mw;
    QByteArray before, after;
};


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

void MainWindow::ApplySceneJson(const QByteArray& bytes)
{
    if (!document)
        return;

    // Save current selection name before LoadJson destroys elements
    UiElement* current = CurrentElement();
    QString selectedName = current ? current->GetName() : QString();

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

    // Restore selection by name
    UiElement* restored = nullptr;
    if (!selectedName.isEmpty())
    {
        for (auto* e : document->GetRoot()->findChildren<UiElement*>())
        {
            if (e->GetName() == selectedName)
            {
                restored = e;
                break;
            }
        }
    }

    if (restored)
    {
        auto idx = hierarchyModel->GetIndexFromElement(restored);
        hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        propertyPanel->SetTarget(restored);
        document->SetSelected(restored);
    }
    else
    {
        propertyPanel->SetTarget(document->GetRoot());
    }
}

void MainWindow::onTransformCompleted(const QByteArray& before, const QByteArray& after, const QString& actionName)
{
    if (before != after)
    {
        ApplySceneJson(before);
        undoStack->push(new JsonSceneCommand(this, before, after, actionName));
    }
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

    hierarchyModel = new EntityTreeModel(document->GetRoot(), this);
    hierarchySelection = new QItemSelectionModel(hierarchyModel, this);
    hierarchyView->setModel(hierarchyModel);
    hierarchyView->setSelectionModel(hierarchySelection);

    WireHierarchySignals();
    layout->addWidget(hierarchyView);
    contents->setLayout(layout);

    connect(hierarchySelection, &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex&){ UiElement* e = hierarchyModel->GetElementFromIndex(current); document->SetSelected(e); propertyPanel->SetTarget(e); });
    connect(hierarchyModel, &EntityTreeModel::HierarchyChanged, this, [this](){ hierarchyView->expandAll(); });
}

void MainWindow::BuildPropertyDock()
{
    QWidget* contents = ui->PropertyEditorContents;
    auto* layout = new QVBoxLayout(); layout->setContentsMargins(4,4,4,4);

    propertyPanel = new PropertyEditorPanel(contents);

    layout->addWidget(propertyPanel);
    contents->setLayout(layout);

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
            QMessageBox::information(this, "Export", "Scene exported successfully.");
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

        // Resolve relative asset paths against the JSON file's directory
        QString baseDir = QFileInfo(path).absolutePath();
        json = SceneExporter::ResolveJsonPaths(json, baseDir);

        SceneDocument* newDoc = new SceneDocument(this);

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

    connect(hierarchySelection, &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex&)
    {
        UiElement* e = hierarchyModel->GetElementFromIndex(current);

        document->SetSelected(e);
        propertyPanel->SetTarget(e);
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

    sceneSelectionConnection = connect(scene, &QGraphicsScene::selectionChanged, this, [this]()
    {
        auto items = document->GetScene()->selectedItems();

        m_viewport->viewport()->update();

        if (items.isEmpty())
            return;

        if (auto* se = static_cast<SceneElementItem*>(items.first()))
        {
            UiElement* e = se->GetElement();

            auto idx = hierarchyModel->GetIndexFromElement(e);

            hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            propertyPanel->SetTarget(e);
        }
    });

    // Fit view once on initial attach (new document / file load)
    QTimer::singleShot(0, this, [this, scene]()
    {
        if (!m_viewport->viewport()->size().isEmpty())
            m_viewport->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    });
}

void MainWindow::DoCopy()
{
    UiElement* e = CurrentElement();

    if (!e)
        return;

    QJsonObject obj;
    e->ToJson(obj);

    QJsonDocument doc(obj);
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

    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    UiElement* current = CurrentElement();
    UiElement* parent  = current ? qobject_cast<UiElement*>(current->parent()) : document->GetRoot();

    if (!parent)
        parent = document->GetRoot();

    const QByteArray before = document->ExportJson();

    document->CreateElementFromJson(doc.object(), parent);

    const QByteArray after = document->ExportJson();

    ApplySceneJson(before);
    undoStack->push(new JsonSceneCommand(this, before, after, "Paste"));
}

void MainWindow::DoCut()
{
    UiElement* e = CurrentElement();

    if (!e || e == document->GetRoot() || e->IsSlot())
        return;

    DoCopy();

    const QByteArray before = document->ExportJson();
    document->DeleteElement(e);
    const QByteArray after  = document->ExportJson();

    ApplySceneJson(before);

    undoStack->push(new JsonSceneCommand(this, before, after, "Cut"));
}

void MainWindow::DoDuplicate()
{
    UiElement* e = CurrentElement();

    if (!e || e == document->GetRoot() || e->IsSlot())
        return;

    QJsonObject obj; e->ToJson(obj);

    const QByteArray before = document->ExportJson();

    UiElement* parent = qobject_cast<UiElement*>(e->parent());

    if (!parent)
        parent = document->GetRoot();

    UiElement* dup = document->CreateElementFromJson(obj, parent);

    if (dup)
    {
        if (auto* t = dup->GetComponent<TransformComponent>())
            t->SetPosition(t->GetPosition() + QPointF(20.0, 20.0));

        dup->SetName(e->GetName() + " Copy");
    }

    const QByteArray after = document->ExportJson();

    ApplySceneJson(before);
    undoStack->push(new JsonSceneCommand(this, before, after, "Duplicate"));
}

void MainWindow::DoDelete()
{
    UiElement* e = CurrentElement();

    if (!e || e == document->GetRoot() || e->IsSlot())
        return;

    const QByteArray before = document->ExportJson();
    document->DeleteElement(e);
    const QByteArray after  = document->ExportJson();

    ApplySceneJson(before);
    undoStack->push(new JsonSceneCommand(this, before, after, "Delete"));
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
