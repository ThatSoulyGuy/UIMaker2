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

    // Suspend rebuilds for the duration of a viewport gesture. The transform
    // handler writes TransformComponent once per mouse-move, and each write posts
    // a queued ComponentChanged; rebuilding the whole panel at input rate cost
    // ~1.8 ms per frame and made the inspector flicker under the cursor. Call
    // SetLive(false) on TransformStarted and SetLive(true) + RefreshTargets() on
    // TransformEnded.
    void SetLive(bool on);

    // The single mutation entry point: writes the property, broadcasts it to
    // every other selected element's same-kind component, and emits the
    // (before, after) records MainWindow turns into an undo command. Public so
    // the hierarchy tree can route renames through the same path.
    void ApplyPropertyChange(QObject* primary, const QByteArray& propName, const QVariant& value);

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

    // Coalesces the queued Rebuild. ComponentChanged is delivered queued, one per
    // property write, so a gizmo drag used to post one singleShot(0, Rebuild) per
    // mouse-move - each one tearing down and re-creating every widget in the panel.
    bool rebuildQueued = false;

    // False while a viewport transform is in flight: the panel holds still and
    // refreshes once on mouse-up instead of rebuilding per frame.
    bool live = true;
};

#endif
