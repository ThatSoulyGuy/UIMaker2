#include "render/GizmoRenderPass.hpp"

#include "tools/ToolManager.hpp"
#include "gizmos/GizmoManager.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneElementItem.hpp"
#include "components/TransformComponent.hpp"
#include "input/TransformInputHandler.hpp"
#include "input/EditorContext.hpp"
#include "core/UiElement.hpp"

GizmoRenderPass::GizmoRenderPass(QObject* parent) : RenderPass(parent) { }

QString GizmoRenderPass::GetId() const
{
    return QStringLiteral("gizmos");
}

RenderLayer GizmoRenderPass::GetLayer() const
{
    return RenderLayer::Gizmos;
}

void GizmoRenderPass::SetToolManager(ToolManager* toolManager)
{
    m_toolManager = toolManager;
}

void GizmoRenderPass::Render(QPainter& painter, const EditorContext& ctx)
{
    if (!m_toolManager || !ctx.document || !ctx.view)
        return;

    GizmoManager* gizmoManager = m_toolManager->GetGizmoManager();

    if (!gizmoManager)
        return;

    auto* scene = ctx.document->GetScene();

    if (!scene)
        return;

    EditorContext mctx = ctx;
    QList<SceneElementItem*> selectedTop = TransformInputHandler::GetTopLevelSelectedItems(mctx);

    if (selectedTop.isEmpty())
        return;

    const bool isGroup = selectedTop.size() > 1;
    const QRectF sceneBounds = isGroup
        ? TransformInputHandler::ComputeUnionSceneBounds(selectedTop)
        : selectedTop.first()->sceneBoundingRect();

    double rotation = 0.0;
    QPointF scale(1.0, 1.0);

    if (!isGroup)
    {
        if (auto* element = selectedTop.first()->GetElement())
        {
            if (auto* xform = element->GetComponent<TransformComponent>())
            {
                rotation = xform->GetRotationDegrees();
                scale = xform->GetScale();
            }
        }
    }

    gizmoManager->Draw(painter, sceneBounds, ctx.view, rotation, scale);
}
