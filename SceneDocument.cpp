#include "SceneDocument.hpp"

SceneDocument::SceneDocument(QObject* parent) : QObject(parent), root(new UiElement("Root")), scene(new QGraphicsScene(this))
{
    scene->setSceneRect(QRectF(0.0, 0.0, 1920.0, 1080.0));
}

SceneElementItem* SceneDocument::CreateItemFor(UiElement* e)
{
    auto* item = new SceneElementItem(e);

    scene->addItem(item);
    items.insert(e, item);

    return item;
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

QByteArray SceneDocument::ExportJson() const
{
    QJsonObject rootObj;

    root->ToJson(rootObj);
    QJsonDocument doc(rootObj);

    return doc.toJson(QJsonDocument::Indented);
}

void SceneDocument::SetSelected(UiElement* e)
{
    for (auto it = items.begin(); it != items.end(); ++it)
        it.value()->setSelected(it.key() == e);

    emit SelectionChanged(e);
}
