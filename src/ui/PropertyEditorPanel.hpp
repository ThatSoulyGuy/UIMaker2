#ifndef UI_PROPERTYEDITORPANEL_HPP
#define UI_PROPERTYEDITORPANEL_HPP

#include <QWidget>
#include <QUuid>
#include <QVariant>
#include <QByteArray>
#include <QString>
#include <QList>

class UiElement;
class QScrollArea;
class QVBoxLayout;
class QMetaProperty;

// One object's (before, after) values for a single property edit made through
// the panel. MainWindow turns the list emitted by PropertyChangeApplied into
// an undo command; elements are re-resolved by id so the command survives
// scene rebuilds. An empty componentKind means the property lives on the
// element itself rather than on one of its components.
struct PropertyEditRecord
{
    QUuid elementId;
    QString componentKind;
    QByteArray propName;
    QVariant before;
    QVariant after;
};

class PropertyEditorPanel : public QWidget
{
    Q_OBJECT

public:

    explicit PropertyEditorPanel(QWidget* parent = nullptr);

    void SetTarget(UiElement* element);
    void SetTargets(const QList<UiElement*>& elements);

    // Rebuild immediately, bypassing the focus-deferral path. MainWindow calls
    // this after undo/redo so editors never keep showing stale values while
    // keyboard focus sits inside the panel.
    void RefreshTargets();

signals:

    void PropertyEdited();

    // Emitted after ApplyPropertyChange lands a real change (before != after)
    // on at least one object, with one record per changed object.
    void PropertyChangeApplied(const QList<PropertyEditRecord>& records);

    // Emitted when the user establishes the project root (the folder that will
    // contain scene.json) via the asset browse flow. MainWindow keeps the
    // active SceneDocument's base directory in sync with this.
    void ProjectRootChanged(const QString& dir);

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
