#include "scene/SceneDocument.hpp"

#include <QBrush>
#include <QColor>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QSet>
#include <QSignalBlocker>
#include <QStringList>
#include <QUuid>
#include <functional>

#include "core/Anchor.hpp"
#include "core/AssetContext.hpp"
#include "core/Component.hpp"
#include "core/UiElement.hpp"
#include "scene/SceneElementItem.hpp"

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

SceneDocument::SceneDocument(QObject* parent) : QObject(parent), root(new UiElement("Root")), scene(new QGraphicsScene(this)), rootRect(nullptr)
{
    scene->setSceneRect(QRectF(0.0, 0.0, 1920.0, 1080.0));

    QObject::connect(scene, &QGraphicsScene::selectionChanged, this, &SceneDocument::OnSceneSelectionChanged);

    {
        const int tileSize = 32;
        const int dotSpacing = 16;
        const qreal dotRadius = 1.2;

        QPixmap tile(tileSize, tileSize);
        tile.fill(QColor(35, 35, 38));

        QPainter painter(&tile);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(110, 110, 115));

        for (int y = 0; y < tileSize; y += dotSpacing)
        {
            for (int x = 0; x < tileSize; x += dotSpacing)
            {
                painter.drawEllipse(QPointF(x + 0.5, y + 0.5), dotRadius, dotRadius);
            }
        }
        painter.end();

        scene->setBackgroundBrush(QBrush(tile));
    }

    {
        QPen borderPen(QColor(220, 220, 220));
        borderPen.setCosmetic(true);
        borderPen.setWidth(1);

        rootRect = scene->addRect(scene->sceneRect(), borderPen, Qt::NoBrush);
        rootRect->setZValue(-1);
    }

    WireRootConnections();
}

SceneDocument::~SceneDocument()
{
    delete root;
}

// Root-scoped connections are re-established on every load (the root is
// replaced), so disconnect the previous ones first to avoid stacking.
void SceneDocument::WireRootConnections()
{
    QObject::disconnect(m_sceneRectConn);
    QObject::disconnect(m_rootStructureConn);

    m_sceneRectConn = QObject::connect(scene, &QGraphicsScene::sceneRectChanged, this, [this](const QRectF& r)
    {
        if (rootRect)
            rootRect->setRect(r);
    });

    m_rootStructureConn = QObject::connect(root, &UiElement::StructureChanged, this, &SceneDocument::OnStructureChanged);
}

SceneElementItem* SceneDocument::CreateItemFor(UiElement* e)
{
    auto* item = new SceneElementItem(e);

    SceneElementItem* parentItem = nullptr;
    if (auto* parentElement = qobject_cast<UiElement*>(e->parent()))
    {
        parentItem = items.value(parentElement, nullptr);
        if (parentItem)
            item->setParentItem(parentItem);
    }

    // setParentItem() on a parent that is already in the scene implicitly adds
    // this item to that scene. Only add it explicitly when it has no such
    // parent, otherwise QGraphicsScene warns "item has already been added".
    if (item->scene() != scene)
        scene->addItem(item);

    items.insert(e, item);

    // Re-run anchor/stretch math now that the item has a real parent / scene attachment.
    // The first refresh ran inside the SEI constructor with no parent and no scene, so any
    // formula touching parentRect.width/height/topLeft saw a zero rect.
    item->RefreshFromComponents();

    QObject::connect(e, &UiElement::StructureChanged, this, &SceneDocument::OnStructureChanged);

    // Refresh parent layout so the new child is positioned immediately
    if (parentItem)
    {
        auto* parentElement = qobject_cast<UiElement*>(e->parent());
        for (auto* comp : parentElement->GetComponents())
        {
            if (comp->IsLayout())
            {
                parentItem->RefreshFromComponents();
                parentItem->update();
                break;
            }
        }
    }

    return item;
}

void SceneDocument::UpdateZValues(UiElement* parent)
{
    int z = 0;

    for (QObject* c : parent->children())
    {
        if (auto* e = qobject_cast<UiElement*>(c))
        {
            if (auto* item = items.value(e, nullptr))
                item->setZValue(z++);

            UpdateZValues(e);
        }
    }
}

UiElement* SceneDocument::CreateImageElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<ImageComponent>();

    auto* item = CreateItemFor(e);

    Q_UNUSED(item);

    return e;
}

UiElement* SceneDocument::CreateTextElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();

    auto* t = e->AddComponent<TextComponent>();
    t->SetText("Text");
    auto* item = CreateItemFor(e);

    Q_UNUSED(item);

    return e;
}

UiElement* SceneDocument::CreateButtonElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();

    auto* b = e->AddComponent<ButtonComponent>();
    b->SetText("Button");

    auto* item = CreateItemFor(e);
    Q_UNUSED(item);

    return e;
}

UiElement* SceneDocument::CreateStackLayoutElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<StackLayoutComponent>();

    CreateItemFor(e);

    return e;
}

UiElement* SceneDocument::CreateGridLayoutElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<GridLayoutComponent>();

    CreateItemFor(e);

    return e;
}

UiElement* SceneDocument::CreateScrollBoxElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    auto* t = e->AddComponent<TransformComponent>();
    t->SetScale(QPointF(200.0, 300.0));
    e->AddComponent<ScrollBoxComponent>();

    CreateItemFor(e);

    return e;
}

UiElement* SceneDocument::CreatePanelElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    auto* t = e->AddComponent<TransformComponent>();
    t->SetScale(QPointF(200.0, 150.0));
    e->AddComponent<PanelComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateProgressBarElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    auto* t = e->AddComponent<TransformComponent>();
    t->SetScale(QPointF(200.0, 24.0));
    e->AddComponent<ProgressBarComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateToggleElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<ToggleComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateDropdownElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<DropdownComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateTextInputElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<TextInputComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateIconElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<IconComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateSpriteElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<SpriteComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateTooltipElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<TooltipComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateModalElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    auto* t = e->AddComponent<TransformComponent>();
    t->SetScale(QPointF(400.0, 300.0));
    e->AddComponent<ModalComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateTabContainerElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    auto* t = e->AddComponent<TransformComponent>();
    t->SetScale(QPointF(300.0, 200.0));

    auto* tc = e->AddComponent<TabContainerComponent>();

    CreateItemFor(e);

    QStringList tabs = tc->GetTabNames().split(',', Qt::SkipEmptyParts);
    EnsureSlots(e, QStringLiteral("TabContainer"), tabs.size(), QStringLiteral("Tab %1"));
    WireSlotReconciliation(e);

    return e;
}

UiElement* SceneDocument::CreateRadialMenuElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    auto* rm = e->AddComponent<RadialMenuComponent>();

    CreateItemFor(e);

    EnsureSlots(e, QStringLiteral("RadialMenu"), rm->GetSliceCount(), QStringLiteral("Slot %1"));
    WireSlotReconciliation(e);

    return e;
}

UiElement* SceneDocument::CreateMinimapElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    auto* t = e->AddComponent<TransformComponent>();
    t->SetScale(QPointF(180.0, 180.0));
    e->AddComponent<MinimapComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateDragSlotElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<DragSlotComponent>();

    CreateItemFor(e);
    return e;
}

UiElement* SceneDocument::CreateListRepeaterElement(const QString& name, UiElement* parent)
{
    UiElement* p = parent ? parent : root;
    UiElement* e = p->AddChild(name);

    e->AddComponent<TransformComponent>();
    e->AddComponent<ListRepeaterComponent>();

    CreateItemFor(e);
    return e;
}

void SceneDocument::OnStructureChanged()
{
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        UiElement* element = it.key();
        SceneElementItem* item = it.value();

        UiElement* parentElement = qobject_cast<UiElement*>(element->parent());
        SceneElementItem* parentItem = parentElement ? items.value(parentElement, nullptr) : nullptr;

        if (item->parentItem() != parentItem)
        {
            item->setParentItem(parentItem);

            if (!parentItem && item->scene() != scene)
                scene->addItem(item);
        }
    }

    UpdateZValues(root);

    // Refresh any layout parents so children are repositioned
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        for (auto* comp : it.key()->GetComponents())
        {
            if (comp->IsLayout())
            {
                it.value()->RefreshFromComponents();
                it.value()->update();
                break;
            }
        }
    }
}

QByteArray SceneDocument::ExportJson() const
{
    QJsonObject rootObj;

    root->ToJson(rootObj);
    QJsonDocument doc(rootObj);

    return doc.toJson(QJsonDocument::Indented);
}

bool SceneDocument::LoadJson(const QByteArray& data)
{
    QJsonParseError err;

    QJsonDocument doc = QJsonDocument::fromJson(data, &err);

    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    scene->clear();
    items.clear();

    QPen borderPen(QColor(220, 220, 220));

    borderPen.setCosmetic(true);
    borderPen.setWidth(1);
    rootRect = scene->addRect(scene->sceneRect(), borderPen, Qt::NoBrush);
    rootRect->setZValue(-1);

    delete root;
    root = new UiElement("Root");

    QJsonObject rootObj = doc.object();
    root->SetName(rootObj["name"].toString("Root"));
    root->SetId(QUuid::fromString(rootObj["id"].toString()));

    for (const QJsonValue& v : rootObj["components"].toArray())
    {
        const QJsonObject c = v.toObject();

        if (auto* comp = Component::Create(c["kind"].toString(), root))
            comp->FromJson(c);
    }

    for (const QJsonValue& v : rootObj["children"].toArray())
        CreateElementFromJson(v.toObject(), root, /*preserveIds=*/true);

    WireRootConnections();
    OnStructureChanged();

    return true;
}

// Destroys an element, its subtree, and their SceneElementItems without any
// guards or notifications. Callers emit the structure-change signals.
void SceneDocument::RemoveElementInternal(UiElement* e)
{
    std::function<void(UiElement*)> removeRec = [&](UiElement* n)
    {
        const QObjectList kids = n->children();

        for (QObject* c : kids)
        {
            if (auto* ce = qobject_cast<UiElement*>(c))
                removeRec(ce);
        }

        if (auto* it = items.take(n))
        {
            scene->removeItem(it);
            delete it;
        }
    };

    removeRec(e);

    e->setParent(nullptr);
    delete e;
}

void SceneDocument::DeleteElement(UiElement* e)
{
    if (!e || e == root || e->IsSlot())
        return;

    SetSelected(nullptr);

    UiElement* parent = qobject_cast<UiElement*>(e->parent());
    RemoveElementInternal(e);

    if (parent)
        emit parent->StructureChanged();

    emit root->StructureChanged();
}


void SceneDocument::SetSelected(UiElement* e)
{
    SetSelectedElements(e ? QList<UiElement*>{ e } : QList<UiElement*>{});
}

void SceneDocument::SetSelectedElements(const QList<UiElement*>& elements)
{
    if (m_syncingSelection)
        return;

    m_syncingSelection = true;

    const QSet<UiElement*> selectedSet(elements.begin(), elements.end());

    // Block scene::selectionChanged emission so the intermediate state during the
    // setSelected loop doesn't trigger downstream UI sync until the final state lands.
    {
        QSignalBlocker blocker(scene);

        for (auto it = items.begin(); it != items.end(); ++it)
        {
            const bool shouldSelect = selectedSet.contains(it.key());
            if (it.value()->isSelected() != shouldSelect)
                it.value()->setSelected(shouldSelect);
        }
    }

    m_syncingSelection = false;

    emit SelectionChanged(GetSelectedElements());
}

QList<UiElement*> SceneDocument::GetSelectedElements() const
{
    QList<UiElement*> result;

    for (auto* qitem : scene->selectedItems())
    {
        if (auto* sei = dynamic_cast<SceneElementItem*>(qitem))
            result.append(sei->GetElement());
    }

    return result;
}

UiElement* SceneDocument::GetPrimarySelection() const
{
    const auto sel = GetSelectedElements();
    return sel.isEmpty() ? nullptr : sel.last();
}

UiElement* SceneDocument::FindById(const QUuid& id) const
{
    if (!root || id.isNull())
        return nullptr;

    if (root->GetId() == id)
        return root;

    // findChildren is recursive across all QObject descendants.
    for (UiElement* e : root->findChildren<UiElement*>())
    {
        if (e && e->GetId() == id)
            return e;
    }

    return nullptr;
}

void SceneDocument::OnSceneSelectionChanged()
{
    if (m_syncingSelection)
        return;

    emit SelectionChanged(GetSelectedElements());
}

UiElement* SceneDocument::CreateElementFromJson(const QJsonObject& obj, UiElement* parent, bool preserveIds)
{
    QString name = obj["name"].toString("Element");
    UiElement* e = parent->AddChild(name);

    // Preserve the id only on load/undo (so selection survives a JSON round-trip).
    // Paste/Duplicate intentionally pass preserveIds=false so copies get fresh ids.
    if (preserveIds)
        e->SetId(QUuid::fromString(obj["id"].toString()));

    QJsonArray comps = obj["components"].toArray();

    for (const QJsonValue& v : comps)
    {
        QJsonObject c = v.toObject();
        QString kind = c["kind"].toString();
        if (auto* comp = Component::Create(kind, e))
            comp->FromJson(c);
    }

    CreateItemFor(e);

    QJsonArray children = obj["children"].toArray();

    for (const QJsonValue& v : children)
    {
        QJsonObject childObj = v.toObject();
        CreateElementFromJson(childObj, e, preserveIds);
    }

    if (auto* tc = e->GetComponent<TabContainerComponent>())
    {
        QStringList tabs = tc->GetTabNames().split(',', Qt::SkipEmptyParts);
        EnsureSlots(e, QStringLiteral("TabContainer"), tabs.size(), QStringLiteral("Tab %1"));
        WireSlotReconciliation(e);
    }

    if (auto* rm = e->GetComponent<RadialMenuComponent>())
    {
        EnsureSlots(e, QStringLiteral("RadialMenu"), rm->GetSliceCount(), QStringLiteral("Slot %1"));
        WireSlotReconciliation(e);
    }

    return e;
}

void SceneDocument::EnsureSlots(UiElement* master, const QString& masterKind, int desiredCount, const QString& nameFormat)
{
    if (!master)
        return;

    // Existing slots by index. Growth is index-based so a missing middle index
    // is recreated even when the total count happens to match, and a surviving
    // surplus slot is reused (same element, same id) if the count grows back.
    QSet<int> present;

    for (QObject* c : master->children())
    {
        if (auto* ce = qobject_cast<UiElement*>(c))
        {
            if (!ce->IsSlot())
                continue;

            auto* s = ce->GetComponent<SlotComponent>();

            if (s && s->GetMasterKind() == masterKind)
                present.insert(s->GetSlotIndex());
        }
    }

    bool added = false;

    for (int i = 0; i < desiredCount; ++i)
    {
        if (present.contains(i))
            continue;

        UiElement* slave = master->AddChild(nameFormat.arg(i + 1));

        auto* t = slave->AddComponent<TransformComponent>();
        t->SetStretch(AnchorFlags(Anchor::LEFT | Anchor::RIGHT | Anchor::TOP | Anchor::BOTTOM));

        auto* s = slave->AddComponent<SlotComponent>();
        s->SetSlotIndex(i);
        s->SetMasterKind(masterKind);

        CreateItemFor(slave);

        added = true;
    }

    // Shrink: surplus slots are unreachable through user deletion (DeleteElement's
    // IsSlot() guard), so prune them here - but ONLY when empty. A surplus slot
    // still holding user content is kept (and reused if the count grows back), so
    // reconciliation can never destroy content: not during per-keystroke tabNames
    // edits, not on undo of a count change, and not while loading legacy files
    // that serialised orphan slots with content. Collect first, then delete, so
    // nothing iterates the child list while it mutates.
    QList<UiElement*> doomed;

    for (QObject* c : master->children())
    {
        auto* ce = qobject_cast<UiElement*>(c);

        if (!ce || !ce->IsSlot())
            continue;

        auto* s = ce->GetComponent<SlotComponent>();

        if (!s || s->GetMasterKind() != masterKind || s->GetSlotIndex() < desiredCount)
            continue;

        bool hasContent = false;

        for (QObject* cc : ce->children())
        {
            if (qobject_cast<UiElement*>(cc))
            {
                hasContent = true;
                break;
            }
        }

        if (!hasContent)
            doomed.append(ce);
    }

    for (UiElement* ce : doomed)
        RemoveElementInternal(ce);

    if (added || !doomed.isEmpty())
    {
        if (auto* masterItem = items.value(master, nullptr))
            masterItem->RefreshFromComponents();

        // Root's StructureChanged is what the hierarchy panel listens to; emit
        // for growth as well as shrink so new slots appear immediately.
        emit master->StructureChanged();
        emit root->StructureChanged();
    }
}

void SceneDocument::WireSlotReconciliation(UiElement* master)
{
    if (!master)
        return;

    if (auto* tc = master->GetComponent<TabContainerComponent>())
    {
        QObject::connect(tc, &Component::ComponentChanged, this, [this, master, tc]()
        {
            QStringList tabs = tc->GetTabNames().split(',', Qt::SkipEmptyParts);
            EnsureSlots(master, QStringLiteral("TabContainer"), tabs.size(), QStringLiteral("Tab %1"));
        });
    }

    if (auto* rm = master->GetComponent<RadialMenuComponent>())
    {
        QObject::connect(rm, &Component::ComponentChanged, this, [this, master, rm]()
        {
            EnsureSlots(master, QStringLiteral("RadialMenu"), rm->GetSliceCount(), QStringLiteral("Slot %1"));
        });
    }
}

UiElement* SceneDocument::GetRoot() const noexcept
{
    return root;
}

QGraphicsScene* SceneDocument::GetScene() const noexcept
{
    return scene;
}

QString SceneDocument::GetBaseDir() const noexcept
{
    return m_baseDir;
}

void SceneDocument::SetBaseDir(const QString& dir)
{
    m_baseDir = dir;
    AssetContext::SetBaseDir(dir);
}

SceneElementItem* SceneDocument::GetItem(UiElement* e) const
{
    return items.value(e, nullptr);
}
