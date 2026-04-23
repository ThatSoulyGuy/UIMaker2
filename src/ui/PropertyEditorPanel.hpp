#ifndef UI_PROPERTYEDITORPANEL_HPP
#define UI_PROPERTYEDITORPANEL_HPP

#include <QWidget>
#include <QTreeWidget>
#include <QFormLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QFontComboBox>
#include <QFileDialog>
#include "core/UiElement.hpp"
#include "core/Component.hpp"
#include "core/Anchor.hpp"

class PropertyEditorPanel : public QWidget
{
    Q_OBJECT

public:

    explicit PropertyEditorPanel(QWidget* parent = nullptr);

    void SetTarget(UiElement* element);

signals:

    void PropertyEdited();

private:

    void Rebuild();
    QWidget* EditorForProperty(QObject* object, const QMetaProperty& prop);

private slots:

    void OnComponentChanged();

private:

    UiElement* target;
    QScrollArea* scrollArea;
    QWidget* container;
    QVBoxLayout* layout;

    bool suppressRebuild = false;
    bool pendingRebuild = false;
};

#endif
