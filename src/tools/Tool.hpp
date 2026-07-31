#ifndef TOOL_HPP
#define TOOL_HPP

#include <QObject>
#include <QString>
#include <QIcon>
#include <QKeySequence>
#include <QHash>
#include <functional>

class Tool : public QObject
{
    Q_OBJECT

public:

    explicit Tool(QObject* parent = nullptr);

    virtual ~Tool() = default;

    virtual QString GetId() const = 0;
    virtual QString GetDisplayName() const = 0;

    virtual QIcon GetIcon() const;

    virtual QKeySequence GetShortcut() const;

    virtual void Activate();

    virtual void Deactivate();

    bool IsActive() const noexcept;

    virtual QString GetGizmoId() const = 0;

    using ToolFactory = std::function<Tool*(QObject*)>;

    static QHash<QString, ToolFactory>& Registry();

    static void Register(const QString& id, ToolFactory factory);

    static Tool* Create(const QString& id, QObject* parent);

    static QStringList GetRegisteredIds();

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
