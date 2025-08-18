/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_UIMaker2
{
public:
    QAction *ActionNew;
    QAction *ActionLoad;
    QAction *actionProperties;
    QAction *ActionMove;
    QAction *ActionScale;
    QAction *ActionRotate;
    QAction *ActionUndo;
    QAction *ActionRedo;
    QAction *ActionAddImage;
    QAction *ActionAddButton;
    QAction *ActionAddText;
    QAction *ActionExport;
    QWidget *Grid;
    QGridLayout *gridLayout;
    QGraphicsView *graphicsView;
    QMenuBar *MenuBar;
    QMenu *MenuFile;
    QMenu *MenuEdit;
    QMenu *MenuNew;
    QDockWidget *ObjectHierarchy;
    QWidget *ObjectHierarchyContents;
    QDockWidget *PropertyEditor;
    QWidget *PropertyEditorContents;

    void setupUi(QMainWindow *UIMaker2)
    {
        if (UIMaker2->objectName().isEmpty())
            UIMaker2->setObjectName("UIMaker2");
        UIMaker2->resize(1096, 628);
        QIcon icon;
        icon.addFile(QString::fromUtf8("C:/Users/bkmcm/Documents/Models/UIMakerIcon.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        UIMaker2->setWindowIcon(icon);
        ActionNew = new QAction(UIMaker2);
        ActionNew->setObjectName("ActionNew");
        ActionLoad = new QAction(UIMaker2);
        ActionLoad->setObjectName("ActionLoad");
        actionProperties = new QAction(UIMaker2);
        actionProperties->setObjectName("actionProperties");
        ActionMove = new QAction(UIMaker2);
        ActionMove->setObjectName("ActionMove");
        ActionScale = new QAction(UIMaker2);
        ActionScale->setObjectName("ActionScale");
        ActionRotate = new QAction(UIMaker2);
        ActionRotate->setObjectName("ActionRotate");
        ActionUndo = new QAction(UIMaker2);
        ActionUndo->setObjectName("ActionUndo");
        ActionRedo = new QAction(UIMaker2);
        ActionRedo->setObjectName("ActionRedo");
        ActionAddImage = new QAction(UIMaker2);
        ActionAddImage->setObjectName("ActionAddImage");
        ActionAddButton = new QAction(UIMaker2);
        ActionAddButton->setObjectName("ActionAddButton");
        ActionAddText = new QAction(UIMaker2);
        ActionAddText->setObjectName("ActionAddText");
        ActionExport = new QAction(UIMaker2);
        ActionExport->setObjectName("ActionExport");
        Grid = new QWidget(UIMaker2);
        Grid->setObjectName("Grid");
        gridLayout = new QGridLayout(Grid);
        gridLayout->setObjectName("gridLayout");
        graphicsView = new QGraphicsView(Grid);
        graphicsView->setObjectName("graphicsView");

        gridLayout->addWidget(graphicsView, 0, 0, 1, 1);

        UIMaker2->setCentralWidget(Grid);
        MenuBar = new QMenuBar(UIMaker2);
        MenuBar->setObjectName("MenuBar");
        MenuBar->setGeometry(QRect(0, 0, 1096, 20));
        MenuFile = new QMenu(MenuBar);
        MenuFile->setObjectName("MenuFile");
        MenuEdit = new QMenu(MenuBar);
        MenuEdit->setObjectName("MenuEdit");
        MenuNew = new QMenu(MenuBar);
        MenuNew->setObjectName("MenuNew");
        UIMaker2->setMenuBar(MenuBar);
        ObjectHierarchy = new QDockWidget(UIMaker2);
        ObjectHierarchy->setObjectName("ObjectHierarchy");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ObjectHierarchy->sizePolicy().hasHeightForWidth());
        ObjectHierarchy->setSizePolicy(sizePolicy);
        ObjectHierarchyContents = new QWidget();
        ObjectHierarchyContents->setObjectName("ObjectHierarchyContents");
        ObjectHierarchy->setWidget(ObjectHierarchyContents);
        UIMaker2->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, ObjectHierarchy);
        PropertyEditor = new QDockWidget(UIMaker2);
        PropertyEditor->setObjectName("PropertyEditor");
        sizePolicy.setHeightForWidth(PropertyEditor->sizePolicy().hasHeightForWidth());
        PropertyEditor->setSizePolicy(sizePolicy);
        PropertyEditorContents = new QWidget();
        PropertyEditorContents->setObjectName("PropertyEditorContents");
        PropertyEditor->setWidget(PropertyEditorContents);
        UIMaker2->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, PropertyEditor);

        MenuBar->addAction(MenuFile->menuAction());
        MenuBar->addAction(MenuEdit->menuAction());
        MenuBar->addAction(MenuNew->menuAction());
        MenuFile->addAction(ActionNew);
        MenuFile->addAction(ActionLoad);
        MenuFile->addAction(ActionExport);
        MenuEdit->addAction(ActionMove);
        MenuEdit->addAction(ActionScale);
        MenuEdit->addAction(ActionRotate);
        MenuEdit->addSeparator();
        MenuEdit->addAction(ActionUndo);
        MenuEdit->addAction(ActionRedo);
        MenuNew->addAction(ActionAddImage);
        MenuNew->addAction(ActionAddButton);
        MenuNew->addAction(ActionAddText);

        retranslateUi(UIMaker2);

        QMetaObject::connectSlotsByName(UIMaker2);
    } // setupUi

    void retranslateUi(QMainWindow *UIMaker2)
    {
        UIMaker2->setWindowTitle(QCoreApplication::translate("UIMaker2", "UIMaker2", nullptr));
        ActionNew->setText(QCoreApplication::translate("UIMaker2", "New...", nullptr));
        ActionLoad->setText(QCoreApplication::translate("UIMaker2", "Load...", nullptr));
        actionProperties->setText(QCoreApplication::translate("UIMaker2", "Properties", nullptr));
        ActionMove->setText(QCoreApplication::translate("UIMaker2", "Move", nullptr));
        ActionScale->setText(QCoreApplication::translate("UIMaker2", "Scale...", nullptr));
        ActionRotate->setText(QCoreApplication::translate("UIMaker2", "Rotate", nullptr));
        ActionUndo->setText(QCoreApplication::translate("UIMaker2", "Undo", nullptr));
        ActionRedo->setText(QCoreApplication::translate("UIMaker2", "Redo", nullptr));
        ActionAddImage->setText(QCoreApplication::translate("UIMaker2", "Add Image...", nullptr));
        ActionAddButton->setText(QCoreApplication::translate("UIMaker2", "Add Button...", nullptr));
        ActionAddText->setText(QCoreApplication::translate("UIMaker2", "Add Text", nullptr));
        ActionExport->setText(QCoreApplication::translate("UIMaker2", "Export", nullptr));
        MenuFile->setTitle(QCoreApplication::translate("UIMaker2", "File", nullptr));
        MenuEdit->setTitle(QCoreApplication::translate("UIMaker2", "Edit", nullptr));
        MenuNew->setTitle(QCoreApplication::translate("UIMaker2", "New", nullptr));
        ObjectHierarchy->setWindowTitle(QCoreApplication::translate("UIMaker2", "Object Hierarchy", nullptr));
        PropertyEditor->setWindowTitle(QCoreApplication::translate("UIMaker2", "Property Editor", nullptr));
    } // retranslateUi

};

namespace Ui {
    class UIMaker2: public Ui_UIMaker2 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
