#ifndef GIZMORENDERPASS_HPP
#define GIZMORENDERPASS_HPP

#include "render/RenderPass.hpp"

class ToolManager;

class GizmoRenderPass : public RenderPass
{
    Q_OBJECT

public:

    explicit GizmoRenderPass(QObject* parent = nullptr);

    QString GetId() const override;

    RenderLayer GetLayer() const override;

    void SetToolManager(ToolManager* toolManager);

    void Render(QPainter& painter, const EditorContext& ctx) override;

private:

    ToolManager* m_toolManager = nullptr;

};

#endif
