#include "tools/RotateTool.hpp"

REGISTER_TOOL(RotateTool, "rotate")

RotateTool::RotateTool(QObject* parent) : Tool(parent) { }

QString RotateTool::GetId() const
{
    return QStringLiteral("rotate");
}

QString RotateTool::GetDisplayName() const
{
    return tr("Rotate");
}

QKeySequence RotateTool::GetShortcut() const
{
    return QKeySequence(Qt::Key_E);
}

QString RotateTool::GetGizmoId() const
{
    return QStringLiteral("rotate");
}
