#ifndef SCENEDOCUMENT_HPP
#define SCENEDOCUMENT_HPP

#include <QObject>
#include <QMap>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QUuid>
#include <QJsonObject>

class QGraphicsScene;
class QGraphicsRectItem;
class UiElement;
class SceneElementItem;

class SceneDocument : public QObject
{
    Q_OBJECT

public:

    explicit SceneDocument(QObject* parent = nullptr);
    ~SceneDocument() override;

    UiElement* GetRoot() const noexcept;

    QGraphicsScene* GetScene() const noexcept;

    // Directory that contains this scene's scene.json. All imagePath/fontPath/
    // iconPath values are relative to it. Setting it mirrors into AssetContext
    // so components can resolve relative paths for preview.
    QString GetBaseDir() const noexcept;

    void SetBaseDir(const QString& dir);

    UiElement* CreateImageElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateTextElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateButtonElement(const QString& name, UiElement* parent = nullptr);

    UiElement* CreateStackLayoutElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateGridLayoutElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateScrollBoxElement(const QString& name, UiElement* parent = nullptr);

    UiElement* CreatePanelElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateProgressBarElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateToggleElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateDropdownElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateTextInputElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateIconElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateSpriteElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateTooltipElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateModalElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateTabContainerElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateRadialMenuElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateMinimapElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateDragSlotElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateListRepeaterElement(const QString& name, UiElement* parent = nullptr);

    UiElement* CreateElementFromJson(const QJsonObject& obj, UiElement* parent, bool preserveIds = false);
    void DeleteElement(UiElement* e);

    SceneElementItem* GetItem(UiElement* e) const;

    QByteArray ExportJson() const;
    bool LoadJson(const QByteArray& data);

    QList<UiElement*> GetSelectedElements() const;
    UiElement* GetPrimarySelection() const;

    // Locate an element anywhere in the tree by its persistent UUID. Used by
    // the delta-based undo/redo commands, which survive across undo cycles
    // because UUIDs are preserved (CreateElementFromJson with preserveIds).
    UiElement* FindById(const QUuid& id) const;

signals:

    void SelectionChanged(const QList<UiElement*>& selected);

public slots:

    void SetSelected(UiElement* e);
    void SetSelectedElements(const QList<UiElement*>& elements);

private slots:

    void OnStructureChanged();
    void OnSceneSelectionChanged();

private:

    SceneElementItem* CreateItemFor(UiElement* e);
    void UpdateZValues(UiElement* parent);

    void WireRootConnections();
    void RemoveElementInternal(UiElement* e);

    void EnsureSlots(UiElement* master, const QString& masterKind, int desiredCount, const QString& nameFormat);
    void WireSlotReconciliation(UiElement* master);

    UiElement* root;
    QGraphicsScene* scene;
    QGraphicsRectItem* rootRect;
    QMap<UiElement*, SceneElementItem*> items;
    QString m_baseDir;
    bool m_syncingSelection = false;
    QMetaObject::Connection m_sceneRectConn;
    QMetaObject::Connection m_rootStructureConn;
};

#endif
