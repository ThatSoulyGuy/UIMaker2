#include "tools/TranslateTool.hpp"

REGISTER_TOOL(TranslateTool, "translate")

TranslateTool::TranslateTool(QObject* parent) : Tool(parent) { }

QString TranslateTool::GetId() const
{
    return QStringLiteral("translate");
}

QString TranslateTool::GetDisplayName() const
{
    return tr("Move");
}

QKeySequence TranslateTool::GetShortcut() const
{
    return QKeySequence(Qt::Key_W);
}

QString TranslateTool::GetGizmoId() const
{
    return QStringLiteral("translate");
}
