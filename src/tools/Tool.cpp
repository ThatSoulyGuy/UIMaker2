#include "tools/Tool.hpp"

#include <QIcon>
#include <QKeySequence>
#include <QHash>

Tool::Tool(QObject* parent) : QObject(parent) { }

QIcon Tool::GetIcon() const
{
    return QIcon();
}

QKeySequence Tool::GetShortcut() const
{
    return QKeySequence();
}

void Tool::Activate()
{
    m_active = true;

    emit Activated();
}

void Tool::Deactivate()
{
    m_active = false;

    emit Deactivated();
}

bool Tool::IsActive() const noexcept
{
    return m_active;
}

QHash<QString, Tool::ToolFactory>& Tool::Registry()
{
    static QHash<QString, ToolFactory> registry;

    return registry;
}

void Tool::Register(const QString& id, ToolFactory factory)
{
    Registry().insert(id, factory);
}

Tool* Tool::Create(const QString& id, QObject* parent)
{
    auto it = Registry().find(id);

    return it != Registry().end() ? it.value()(parent) : nullptr;
}

QStringList Tool::GetRegisteredIds()
{
    return Registry().keys();
}
