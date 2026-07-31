#include "tools/ToolManager.hpp"

#include <QAction>
#include <QActionGroup>

#include "tools/Tool.hpp"
#include "gizmos/GizmoManager.hpp"
#include "input/PanZoomHandler.hpp"
#include "input/TransformInputHandler.hpp"

ToolManager::ToolManager(QObject* parent)
    : QObject(parent)
    , m_gizmoManager(new GizmoManager(this))
    , m_panZoomHandler(new PanZoomHandler(this))
    , m_transformHandler(new TransformInputHandler(m_gizmoManager, this))
{
    // Create default tools from registry
    for (const QString& id : Tool::GetRegisteredIds())
    {
        if (Tool* tool = Tool::Create(id, this))
        {
            RegisterTool(tool);
        }
    }

    // Connect transform handler signals
    connect(m_transformHandler, &TransformInputHandler::TransformStarted, this, &ToolManager::onTransformStarted);
    connect(m_transformHandler, &TransformInputHandler::TransformEnded, this, &ToolManager::onTransformEnded);
}

void ToolManager::RegisterTool(Tool* tool)
{
    if (!tool)
        return;

    tool->setParent(this);
    m_tools.insert(tool->GetId(), tool);
}

void ToolManager::SetActiveTool(const QString& toolId)
{
    Tool* newTool = m_tools.value(toolId, nullptr);

    if (!newTool || newTool == m_activeTool)
        return;

    if (m_activeTool)
    {
        m_activeTool->Deactivate();
    }

    m_activeTool = newTool;
    m_activeTool->Activate();

    // Update gizmo manager to show the corresponding gizmo
    m_gizmoManager->SetActiveGizmoId(m_activeTool->GetGizmoId());

    emit ActiveToolChanged(m_activeTool);
}

Tool* ToolManager::GetActiveTool() const noexcept
{
    return m_activeTool;
}

Tool* ToolManager::GetTool(const QString& id) const
{
    return m_tools.value(id, nullptr);
}

QList<Tool*> ToolManager::GetAllTools() const
{
    return m_tools.values();
}

GizmoManager* ToolManager::GetGizmoManager() const noexcept
{
    return m_gizmoManager;
}

InputResult ToolManager::HandleInput(const MousePressEvent& event, EditorContext& ctx)
{
    // Pan/zoom has highest priority
    InputResult result = m_panZoomHandler->HandlePress(event, ctx);

    if (result.consumed)
        return result;

    // Transform handling
    return m_transformHandler->HandlePress(event, ctx);
}

InputResult ToolManager::HandleInput(const MouseMoveEvent& event, EditorContext& ctx)
{
    // Check pan/zoom first
    InputResult result = m_panZoomHandler->HandleMove(event, ctx);

    if (result.consumed)
        return result;

    // Transform handling
    return m_transformHandler->HandleMove(event, ctx);
}

InputResult ToolManager::HandleInput(const MouseReleaseEvent& event, EditorContext& ctx)
{
    // Check pan/zoom first
    InputResult result = m_panZoomHandler->HandleRelease(event, ctx);

    if (result.consumed)
        return result;

    // Transform handling
    return m_transformHandler->HandleRelease(event, ctx);
}

InputResult ToolManager::HandleInput(const WheelEvent& event, EditorContext& ctx)
{
    return m_panZoomHandler->HandleWheel(event, ctx);
}

bool ToolManager::IsTransforming() const
{
    return m_transformHandler->IsTransforming();
}

bool ToolManager::IsPanning() const
{
    return m_panZoomHandler->IsPanning();
}

QActionGroup* ToolManager::CreateToolActions(QObject* parent)
{
    auto* group = new QActionGroup(parent);
    group->setExclusive(true);

    for (Tool* tool : m_tools.values())
    {
        QAction* action = new QAction(tool->GetDisplayName(), group);
        action->setCheckable(true);
        action->setShortcut(tool->GetShortcut());
        action->setIcon(tool->GetIcon());
        action->setData(tool->GetId());

        if (tool == m_activeTool)
            action->setChecked(true);

        connect(action, &QAction::triggered, this, [this, tool]() {
            SetActiveTool(tool->GetId());
        });

        group->addAction(action);
    }

    return group;
}

void ToolManager::onTransformStarted()
{
    emit TransformStarted();
}

void ToolManager::onTransformEnded(const QList<TransformDelta>& deltas, const QString& actionName)
{
    emit TransformEnded(deltas, actionName);
}
