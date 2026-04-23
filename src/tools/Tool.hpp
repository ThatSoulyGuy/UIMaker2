#ifndef TOOL_HPP
#define TOOL_HPP

#include <QObject>
#include <QString>
#include <QIcon>
#include <QKeySequence>
#include <QHash>
#include <functional>

class InputHandler;

class Tool : public QObject
{
    Q_OBJECT

public:

    explicit Tool(QObject* parent = nullptr) : QObject(parent) { }

    virtual ~Tool() = default;

    virtual QString GetId() const = 0;
    virtual QString GetDisplayName() const = 0;

    virtual QIcon GetIcon() const
    {
        return QIcon();
    }

    virtual QKeySequence GetShortcut() const
    {
        return QKeySequence();
    }

    virtual void Activate()
    {
        m_active = true;

        emit Activated();
    }

    virtual void Deactivate()
    {
        m_active = false;

        emit Deactivated();
    }

    bool IsActive() const noexcept
    {
        return m_active;
    }

    virtual QString GetGizmoId() const = 0;

    virtual InputHandler* CreateInputHandler() = 0;

    using ToolFactory = std::function<Tool*(QObject*)>;

    static QHash<QString, ToolFactory>& Registry()
    {
        static QHash<QString, ToolFactory> registry;

        return registry;
    }

    static void Register(const QString& id, ToolFactory factory)
    {
        Registry().insert(id, factory);
    }

    static Tool* Create(const QString& id, QObject* parent)
    {
        auto it = Registry().find(id);

        return it != Registry().end() ? it.value()(parent) : nullptr;
    }

    static QStringList GetRegisteredIds()
    {
        return Registry().keys();
    }

signals:

    void Activated();
    void Deactivated();

protected:

    bool m_active = false;

};

#define REGISTER_TOOL(ClassName, ToolId) \
    static const bool ClassName##_tool_registered = [](){ \
        Tool::Register(ToolId, [](QObject* p){ return new ClassName(p); }); \
        return true; \
    }();

#endif
