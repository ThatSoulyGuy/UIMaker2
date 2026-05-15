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
    void SetTargets(const QList<UiElement*>& elements);

signals:

    void PropertyEdited();

private:

    void Rebuild();
    QWidget* EditorForProperty(QObject* object, const QMetaProperty& prop, bool mixed = false);
    void ApplyPropertyChange(QObject* primary, const QByteArray& propName, const QVariant& value);

private slots:

    void OnComponentChanged();

private:

    UiElement* target;
    QList<UiElement*> targets;
    QScrollArea* scrollArea;
    QWidget* container;
    QVBoxLayout* layout;

    bool suppressRebuild = false;
    bool pendingRebuild = false;
};

#endif
