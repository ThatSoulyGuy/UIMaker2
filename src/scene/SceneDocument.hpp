#ifndef SCENEDOCUMENT_HPP
#define SCENEDOCUMENT_HPP

#include <QGraphicsScene>
#include <QMap>
#include "core/UiElement.hpp"
#include "components/TransformComponent.hpp"
#include "components/ImageComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/StackLayoutComponent.hpp"
#include "components/GridLayoutComponent.hpp"
#include "components/ScrollBoxComponent.hpp"
#include "components/PanelComponent.hpp"
#include "components/ProgressBarComponent.hpp"
#include "components/ToggleComponent.hpp"
#include "components/DropdownComponent.hpp"
#include "components/TextInputComponent.hpp"
#include "components/IconComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TooltipComponent.hpp"
#include "components/ModalComponent.hpp"
#include "components/TabContainerComponent.hpp"
#include "components/RadialMenuComponent.hpp"
#include "components/MinimapComponent.hpp"
#include "components/DragSlotComponent.hpp"
#include "components/ListRepeaterComponent.hpp"
#include "components/SlotComponent.hpp"
#include "scene/SceneElementItem.hpp"

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

    void EnsureSlots(UiElement* master, const QString& masterKind, int desiredCount, const QString& nameFormat);
    void WireSlotReconciliation(UiElement* master);

    UiElement* root;
    QGraphicsScene* scene;
    QGraphicsRectItem* rootRect;
    QMap<UiElement*, SceneElementItem*> items;
};

#endif
