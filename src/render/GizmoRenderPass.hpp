#ifndef GIZMORENDERPASS_HPP
#define GIZMORENDERPASS_HPP

#include "render/RenderPass.hpp"
#include "gizmos/GizmoManager.hpp"
#include "tools/ToolManager.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneElementItem.hpp"
#include "components/TransformComponent.hpp"
#include "input/TransformInputHandler.hpp"

class GizmoRenderPass : public RenderPass
{
    Q_OBJECT

public:

    explicit GizmoRenderPass(QObject* parent = nullptr) : RenderPass(parent) { }

    QString GetId() const override
    {
        return QStringLiteral("gizmos");
    }

    RenderLayer GetLayer() const override
    {
        return RenderLayer::Gizmos;
    }

    void SetToolManager(ToolManager* toolManager)
    {
        m_toolManager = toolManager;
    }

    void Render(QPainter& painter, const EditorContext& ctx) override
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
            if (auto* xform = selectedTop.first()->GetElement()->GetComponent<TransformComponent>())
            {
                rotation = xform->GetRotationDegrees();
                scale = xform->GetScale();
            }
        }

        gizmoManager->Draw(painter, sceneBounds, ctx.view, rotation, scale);
    }

private:

    ToolManager* m_toolManager = nullptr;

};

#endif
