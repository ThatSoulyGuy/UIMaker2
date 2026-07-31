#include "tools/ScaleTool.hpp"

REGISTER_TOOL(ScaleTool, "scale")

ScaleTool::ScaleTool(QObject* parent) : Tool(parent) { }

QString ScaleTool::GetId() const
{
    return QStringLiteral("scale");
}

QString ScaleTool::GetDisplayName() const
{
    return tr("Scale");
}

QKeySequence ScaleTool::GetShortcut() const
{
    return QKeySequence(Qt::Key_R);
}

QString ScaleTool::GetGizmoId() const
{
    return QStringLiteral("scale");
}
