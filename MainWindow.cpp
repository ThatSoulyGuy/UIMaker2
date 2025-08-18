#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include "MainWindow.hpp"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::UIMaker2)
{
    ui->setupUi(this);

    document = new SceneDocument(this);

    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
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
        delete document; document = new SceneDocument(this);

        ui->graphicsView->setScene(document->GetScene());

        delete hierarchyModel;
        hierarchyModel = new EntityTreeModel(document->GetRoot(), this);

        delete hierarchySelection;
        hierarchySelection = new QItemSelectionModel(hierarchyModel, this);

        hierarchyView->setModel(hierarchyModel); hierarchyView->setSelectionModel(hierarchySelection);
        propertyPanel->SetTarget(document->GetRoot());
    });

    connect(ui->ActionLoad, &QAction::triggered, this, [this]()
    {
        auto json = document->ExportJson();

        QGuiApplication::clipboard()->setText(QString::fromUtf8(json));

        QMessageBox::information(this, "Export", "Scene JSON copied to clipboard.");
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
        QByteArray json = document->ExportJson();
        QGuiApplication::clipboard()->setText(QString::fromUtf8(json));
        QMessageBox::information(this, "Export", "Scene JSON copied to clipboard.");
    });
}

void MainWindow::AttachScene(QGraphicsScene* scene)
{
    QObject::disconnect(sceneRectChangedConnection);
    if (scene == nullptr)
        return;

    sceneRectChangedConnection = connect(scene, &QGraphicsScene::sceneRectChanged, this, [this, scene](const QRectF&)
    {
        if (!ui->graphicsView->viewport()->size().isEmpty()) { ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio); }
    });

    QTimer::singleShot(0, this, [this, scene]()
    {
        if (!ui->graphicsView->viewport()->size().isEmpty()) { ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio); }
    });
}
