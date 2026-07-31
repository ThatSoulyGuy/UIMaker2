#ifndef TOOLMANAGER_HPP
#define TOOLMANAGER_HPP

#include <QObject>
#include <QHash>
#include <QActionGroup>
#include <QGraphicsView>

#include "input/InputHandler.hpp"
#include "input/EditorContext.hpp"

class Tool;
class GizmoManager;
class PanZoomHandler;
class TransformInputHandler;

class ToolManager : public QObject
{
    Q_OBJECT

public:

    explicit ToolManager(QObject* parent = nullptr);

    ~ToolManager() = default;

    void RegisterTool(Tool* tool);

    void SetActiveTool(const QString& toolId);

    Tool* GetActiveTool() const noexcept;

    Tool* GetTool(const QString& id) const;

    QList<Tool*> GetAllTools() const;

    GizmoManager* GetGizmoManager() const noexcept;

    InputResult HandleInput(const MousePressEvent& event, EditorContext& ctx);

    InputResult HandleInput(const MouseMoveEvent& event, EditorContext& ctx);

    InputResult HandleInput(const MouseReleaseEvent& event, EditorContext& ctx);

    InputResult HandleInput(const WheelEvent& event, EditorContext& ctx);

    bool IsTransforming() const;

    bool IsPanning() const;

    QActionGroup* CreateToolActions(QObject* parent);

signals:

    void ActiveToolChanged(Tool* tool);
    void TransformStarted();
    void TransformEnded(const QList<TransformDelta>& deltas, const QString& actionName);
    void CursorChanged(Qt::CursorShape cursor);

private slots:

    void onTransformStarted();

    void onTransformEnded(const QList<TransformDelta>& deltas, const QString& actionName);

private:

    QHash<QString, Tool*> m_tools;
    Tool* m_activeTool = nullptr;

    GizmoManager* m_gizmoManager = nullptr;
    PanZoomHandler* m_panZoomHandler = nullptr;
    TransformInputHandler* m_transformHandler = nullptr;

};

#endif
