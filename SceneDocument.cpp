#include "SceneDocument.hpp"
#include <QBrush>

SceneDocument::SceneDocument(QObject* parent) : QObject(parent), root(new UiElement("Root")), scene(new QGraphicsScene(this)), rootRect(nullptr)
{
    scene->setSceneRect(QRectF(0.0, 0.0, 1920.0, 1080.0));

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

    QObject::connect(scene, &QGraphicsScene::sceneRectChanged, this, [this](const QRectF& r)
    {
        if (rootRect)
            rootRect->setRect(r);
    });
}

SceneElementItem* SceneDocument::CreateItemFor(UiElement* e)
{
    auto* item = new SceneElementItem(e);

    if (auto* parentElement = qobject_cast<UiElement*>(e->parent()))
    {
        if (items.contains(parentElement))
            item->setParentItem(items.value(parentElement));
    }

    scene->addItem(item);
    items.insert(e, item);

    QObject::connect(e, &UiElement::StructureChanged, this, &SceneDocument::OnStructureChanged);

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

    rootRect = scene->addRect(scene->sceneRect(), Qt::NoPen, QBrush(Qt::white));
    rootRect->setZValue(-1);

    delete root;
    root = new UiElement("Root");

    QJsonObject rootObj = doc.object();
    root->SetName(rootObj["name"].toString("Root"));

    QJsonArray rootComps = rootObj["components"].toArray();
    for (const QJsonValue& v : rootComps)
    {
        QJsonObject c = v.toObject();
        QString kind = c["kind"].toString();
        Component* comp = nullptr;
        if (kind == "Transform")
            comp = root->AddComponent<TransformComponent>();
        else if (kind == "Image")
            comp = root->AddComponent<ImageComponent>();
        else if (kind == "Text")
            comp = root->AddComponent<TextComponent>();
        else if (kind == "Button")
            comp = root->AddComponent<ButtonComponent>();

        if (comp)
            comp->FromJson(c);
    }

    QJsonArray children = rootObj["children"].toArray();
    for (const QJsonValue& v : children)
    {
        QJsonObject childObj = v.toObject();
        CreateElementFromJson(childObj, root);
    }

    QObject::connect(root, &UiElement::StructureChanged, this, &SceneDocument::OnStructureChanged);
    OnStructureChanged();

    return true;
}

void SceneDocument::SetSelected(UiElement* e)
{
    for (auto it = items.begin(); it != items.end(); ++it)
        it.value()->setSelected(it.key() == e);

    emit SelectionChanged(e);
}

UiElement* SceneDocument::CreateElementFromJson(const QJsonObject& obj, UiElement* parent)
{
    QString name = obj["name"].toString("Element");
    UiElement* e = parent->AddChild(name);

    QJsonArray comps = obj["components"].toArray();

    for (const QJsonValue& v : comps)
    {
        QJsonObject c = v.toObject();
        QString kind = c["kind"].toString();

        Component* comp = nullptr;

        if (kind == "Transform")
            comp = e->AddComponent<TransformComponent>();
        else if (kind == "Image")
            comp = e->AddComponent<ImageComponent>();
        else if (kind == "Text")
            comp = e->AddComponent<TextComponent>();
        else if (kind == "Button")
            comp = e->AddComponent<ButtonComponent>();

        if (comp)
            comp->FromJson(c);
    }

    CreateItemFor(e);

    QJsonArray children = obj["children"].toArray();

    for (const QJsonValue& v : children)
    {
        QJsonObject childObj = v.toObject();
        CreateElementFromJson(childObj, e);
    }

    return e;
}
