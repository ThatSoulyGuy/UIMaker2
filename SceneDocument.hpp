#ifndef SCENEDOCUMENT_HPP
#define SCENEDOCUMENT_HPP

#include <QGraphicsScene>
#include <QMap>
#include "EntityComponentSystem.hpp"
#include "SceneElementItem.hpp"

class QGraphicsRectItem;

class SceneDocument : public QObject
{
    Q_OBJECT

public:

    explicit SceneDocument(QObject* parent = nullptr);

    UiElement* GetRoot() const noexcept
    {
        return root;
    }

    QGraphicsScene* GetScene() const noexcept
    {
        return scene;
    }

    UiElement* CreateImageElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateTextElement(const QString& name, UiElement* parent = nullptr);
    UiElement* CreateButtonElement(const QString& name, UiElement* parent = nullptr);

    UiElement* CreateElementFromJson(const QJsonObject& obj, UiElement* parent);
    void DeleteElement(UiElement* e);

    SceneElementItem* GetItem(UiElement* e) const
    {
        return items.value(e, nullptr);
    }

    QByteArray ExportJson() const;
    bool LoadJson(const QByteArray& data);

signals:

    void SelectionChanged(UiElement*);

public slots:

    void SetSelected(UiElement* e);

private slots:

    void OnStructureChanged();

private:

    SceneElementItem* CreateItemFor(UiElement* e);
    void UpdateZValues(UiElement* parent);

    UiElement* root;
    QGraphicsScene* scene;
    QGraphicsRectItem* rootRect;
    QMap<UiElement*, SceneElementItem*> items;
};

#endif
