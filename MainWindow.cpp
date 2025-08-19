#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include <QWheelEvent>
#include <QScrollBar>
#include <QClipboard>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUndoCommand>
#include <QUndoStack>
#include "SceneElementItem.hpp"
#include "MainWindow.hpp"
#include "./ui_mainwindow.h"

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
    ui->setupUi(this);
    undoStack = new QUndoStack(this);

    document = new SceneDocument(this);

    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    ui->graphicsView->setMouseTracking(true);
    ui->graphicsView->installEventFilter(this);

    ui->graphicsView->setScene(document->GetScene());
    AttachScene(document->GetScene());

    BuildHierarchyDock();
    BuildPropertyDock();
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

    if (!document->LoadJson(bytes))
        return;

    ui->graphicsView->setScene(document->GetScene());
    AttachScene(document->GetScene());

    delete hierarchyModel;
    hierarchyModel = new EntityTreeModel(document->GetRoot(), this);

    delete hierarchySelection;
    hierarchySelection = new QItemSelectionModel(hierarchyModel, this);

    hierarchyView->setModel(hierarchyModel);
    hierarchyView->setSelectionModel(hierarchySelection);
    WireHierarchySignals();

    propertyPanel->SetTarget(document->GetRoot());
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

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->graphicsView->viewport() || watched == ui->graphicsView)
    {
        if (event->type() == QEvent::Wheel)
        {
            auto* wheel = static_cast<QWheelEvent*>(event);
            const int delta = wheel->angleDelta().y();

            if (delta != 0)
            {
                const double factor = std::pow(1.0015, static_cast<double>(delta));
                ZoomAt(wheel->position().toPoint(), factor);
                event->accept();
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonPress)
        {
            auto* me = static_cast<QMouseEvent*>(event);

            const bool spaceHeld = (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier) == 0 ? (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ShiftModifier) == 0 ? (QApplication::keyboardModifiers() & Qt::KeyboardModifier::AltModifier) == 0 ? (QApplication::keyboardModifiers() & Qt::KeyboardModifier::MetaModifier) == 0 ? (QApplication::keyboardModifiers() & Qt::KeyboardModifier::KeyboardModifierMask) == Qt::KeyboardModifier::NoModifier ? false : (QApplication::keyboardModifiers() & Qt::KeyboardModifier::KeyboardModifierMask) == Qt::KeyboardModifier::NoModifier : false : false : false : false; // keep single-line per your rule

            if (me->button() == Qt::MiddleButton || (me->button() == Qt::LeftButton && (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ShiftModifier)))
            {
                isPanning = true;
                lastPanPoint = me->pos();

                ui->graphicsView->setCursor(Qt::ClosedHandCursor);
                ui->graphicsView->setDragMode(QGraphicsView::NoDrag);

                event->accept();

                return true;
            }
        }

        if (event->type() == QEvent::MouseMove && isPanning)
        {
            auto* me = static_cast<QMouseEvent*>(event);
            const QPoint delta = me->pos() - lastPanPoint;
            lastPanPoint = me->pos();

            ui->graphicsView->horizontalScrollBar()->setValue(ui->graphicsView->horizontalScrollBar()->value() - delta.x());
            ui->graphicsView->verticalScrollBar()->setValue(ui->graphicsView->verticalScrollBar()->value() - delta.y());

            event->accept();

            return true;
        }

        if (event->type() == QEvent::MouseButtonRelease && isPanning)
        {
            auto* me = static_cast<QMouseEvent*>(event);

            if (me->button() == Qt::MiddleButton || me->button() == Qt::LeftButton)
            {
                isPanning = false;

                ui->graphicsView->setCursor(Qt::ArrowCursor);
                ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag);

                event->accept();

                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonDblClick)
        {
            auto* me = static_cast<QMouseEvent*>(event);

            if (me->button() == Qt::LeftButton)
            {
                const QPointF scenePos = ui->graphicsView->mapToScene(me->pos());

                if (auto* s = document->GetScene())
                {
                    QGraphicsItem* item = nullptr;
                    const QList<QGraphicsItem*> hit = s->items(scenePos);

                    for (QGraphicsItem* it : hit)
                    {
                        if (it->flags().testFlag(QGraphicsItem::ItemIsSelectable))
                        {
                            item = it;
                            break;
                        }
                    }

                    if (item != nullptr)
                    {
                        FitItem(item);

                        event->accept();

                        return true;
                    }
                }
            }
        }

        if (event->type() == QEvent::KeyPress)
        {
            auto* ke = static_cast<QKeyEvent*>(event);

            if (ke->key() == Qt::Key_F)
            {
                if (auto* s = document->GetScene())
                {
                    const auto selected = s->selectedItems();
                    if (!selected.isEmpty()) { FitItem(selected.first()); event->accept(); return true; }
                }
            }

            if ((ke->modifiers() & Qt::ControlModifier) && ke->key() == Qt::Key_0)
            {
                ui->graphicsView->fitInView(document->GetScene()->sceneRect(), Qt::KeepAspectRatio);
                event->accept();
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
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

void MainWindow::ConnectActions()
{
    connect(ui->ActionAddText, &QAction::triggered, this, [this]()
    {
        UiElement* parent = hierarchyModel->GetElementFromIndex(hierarchySelection->currentIndex());
        UiElement* e = document->CreateTextElement("Text", parent);

        hierarchyModel->OnStructureChanged();

        auto idx = hierarchyModel->GetIndexFromElement(e); hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        propertyPanel->SetTarget(e);
    });

    connect(ui->ActionAddImage, &QAction::triggered, this, [this]()
    {
        UiElement* parent = hierarchyModel->GetElementFromIndex(hierarchySelection->currentIndex());
        UiElement* e = document->CreateImageElement("Image", parent);

        hierarchyModel->OnStructureChanged();

        auto idx = hierarchyModel->GetIndexFromElement(e); hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        propertyPanel->SetTarget(e);
    });

    connect(ui->ActionAddButton, &QAction::triggered, this, [this]()
    {
        UiElement* parent = hierarchyModel->GetElementFromIndex(hierarchySelection->currentIndex());
        UiElement* e = document->CreateButtonElement("Button", parent);

        hierarchyModel->OnStructureChanged();

        auto idx = hierarchyModel->GetIndexFromElement(e); hierarchySelection->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        propertyPanel->SetTarget(e);
    });

    connect(ui->ActionNew, &QAction::triggered, this, [this]()
    {
        delete document;

        document = new SceneDocument(this);

        ui->graphicsView->setScene(document->GetScene());

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
        QString path = QFileDialog::getSaveFileName(this, "Export Scene JSON", QString(), "JSON (*.json)");

        if (path.isEmpty())
            return;

        QFile file(path);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            QMessageBox::warning(this, "Export Failed", QString("Could not open file:\n%1").arg(file.errorString()));
            return;
        }

        QByteArray json = document->ExportJson();
        qint64 bytesWritten = file.write(json);

        file.close();

        if (bytesWritten != json.size())
        {
            QMessageBox::warning(this, "Export Incomplete", "Not all bytes were written.");
            return;
        }

        QMessageBox::information(this, "Export", "Scene exported successfully.");
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

        if (!newDoc->LoadJson(json))
        {
            delete newDoc;
            QMessageBox::warning(this, "Load Failed", "Invalid or corrupt scene JSON.");
            return;
        }

        delete document;
        document = newDoc;

        ui->graphicsView->setScene(document->GetScene());
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

    sceneRectChangedConnection = connect(scene, &QGraphicsScene::sceneRectChanged, this, [this, scene](const QRectF&)
    {
        if (!ui->graphicsView->viewport()->size().isEmpty())
            ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    });

    sceneSelectionConnection = connect(scene, &QGraphicsScene::selectionChanged, this, [this]()
    {
        auto items = document->GetScene()->selectedItems();

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

    QTimer::singleShot(0, this, [this, scene]()
    {
        if (!ui->graphicsView->viewport()->size().isEmpty())
            ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    });
}

void MainWindow::ZoomAt(const QPoint& viewPos, double factor)
{
    QTransform t = ui->graphicsView->transform();

    const double current = t.m11();
    double clampedFactor = factor;

    if (current * factor < minZoom)
        clampedFactor = minZoom / current;
    if (current * factor > maxZoom)
        clampedFactor = maxZoom / current;

    if (std::abs(clampedFactor - 1.0) < 1e-9) { return; }

    const QPointF scenePosBefore = ui->graphicsView->mapToScene(viewPos);

    ui->graphicsView->scale(clampedFactor, clampedFactor);

    const QPointF scenePosAfter = ui->graphicsView->mapToScene(viewPos);

    const QPointF delta = scenePosAfter - scenePosBefore;

    ui->graphicsView->translate(delta.x(), delta.y());
}

void MainWindow::FitItem(QGraphicsItem* item)
{
    if (!item)
        return;

    const QRectF r = item->sceneBoundingRect().adjusted(-20.0, -20.0, 20.0, 20.0);

    ui->graphicsView->fitInView(r, Qt::KeepAspectRatio);
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

    if (!e || e == document->GetRoot())
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

    if (!e || e == document->GetRoot())
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

    if (!e || e == document->GetRoot())
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
