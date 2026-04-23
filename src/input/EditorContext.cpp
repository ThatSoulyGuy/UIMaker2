#include "input/EditorContext.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneElementItem.hpp"

QList<SceneElementItem*> EditorContext::GetSelectedItems() const
{
    QList<SceneElementItem*> result;

    if (!document)
        return result;

    auto* scene = document->GetScene();

    if (!scene)
        return result;

    for (auto* item : scene->selectedItems())
    {
        if (auto* se = dynamic_cast<SceneElementItem*>(item))
            result.append(se);
    }

    return result;
}

UiElement* EditorContext::GetSelectedElement() const
{
    auto items = GetSelectedItems();

    if (items.isEmpty())
        return nullptr;

    return items.first()->GetElement();
}
