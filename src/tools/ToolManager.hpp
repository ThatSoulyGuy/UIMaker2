#ifndef TOOLMANAGER_HPP
#define TOOLMANAGER_HPP

#include <QObject>
#include <QHash>
#include <QAction>
#include <QActionGroup>
#include <QGraphicsView>

#include "tools/Tool.hpp"
#include "gizmos/GizmoManager.hpp"
#include "input/InputHandler.hpp"
#include "input/PanZoomHandler.hpp"
#include "input/TransformInputHandler.hpp"
#include "input/EditorContext.hpp"

class ToolManager : public QObject
{
    Q_OBJECT

public:

    explicit ToolManager(QObject* parent = nullptr)
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

    ~ToolManager() = default;

    void RegisterTool(Tool* tool)
    {
        if (!tool)
            return;

        tool->setParent(this);
        m_tools.insert(tool->GetId(), tool);
    }

    void SetActiveTool(const QString& toolId)
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

    Tool* GetActiveTool() const noexcept
    {
        return m_activeTool;
    }

    Tool* GetTool(const QString& id) const
    {
        return m_tools.value(id, nullptr);
    }

    QList<Tool*> GetAllTools() const
    {
        return m_tools.values();
    }

    GizmoManager* GetGizmoManager() const noexcept
    {
        return m_gizmoManager;
    }

    InputResult HandleInput(const MousePressEvent& event, EditorContext& ctx)
    {
        // Pan/zoom has highest priority
        InputResult result = m_panZoomHandler->HandlePress(event, ctx);

        if (result.consumed)
            return result;

        // Transform handling
        return m_transformHandler->HandlePress(event, ctx);
    }

    InputResult HandleInput(const MouseMoveEvent& event, EditorContext& ctx)
    {
        // Check pan/zoom first
        InputResult result = m_panZoomHandler->HandleMove(event, ctx);

        if (result.consumed)
            return result;

        // Transform handling
        return m_transformHandler->HandleMove(event, ctx);
    }

    InputResult HandleInput(const MouseReleaseEvent& event, EditorContext& ctx)
    {
        // Check pan/zoom first
        InputResult result = m_panZoomHandler->HandleRelease(event, ctx);

        if (result.consumed)
            return result;

        // Transform handling
        return m_transformHandler->HandleRelease(event, ctx);
    }

    InputResult HandleInput(const WheelEvent& event, EditorContext& ctx)
    {
        return m_panZoomHandler->HandleWheel(event, ctx);
    }

    bool IsTransforming() const
    {
        return m_transformHandler->IsTransforming();
    }

    bool IsPanning() const
    {
        return m_panZoomHandler->IsPanning();
    }

    QActionGroup* CreateToolActions(QObject* parent)
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

signals:

    void ActiveToolChanged(Tool* tool);
    void TransformStarted();
    void TransformEnded(const QByteArray& beforeState, const QByteArray& afterState, const QString& actionName);
    void CursorChanged(Qt::CursorShape cursor);

private slots:

    void onTransformStarted()
    {
        emit TransformStarted();
    }

    void onTransformEnded(const QByteArray& before, const QByteArray& after, const QString& actionName)
    {
        emit TransformEnded(before, after, actionName);
    }

private:

    QHash<QString, Tool*> m_tools;
    Tool* m_activeTool = nullptr;

    GizmoManager* m_gizmoManager = nullptr;
    PanZoomHandler* m_panZoomHandler = nullptr;
    TransformInputHandler* m_transformHandler = nullptr;

};

#endif
