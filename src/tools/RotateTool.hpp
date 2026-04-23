#ifndef ROTATETOOL_HPP
#define ROTATETOOL_HPP

#include "tools/Tool.hpp"
#include "input/InputHandler.hpp"

class RotateTool : public Tool
{
    Q_OBJECT

public:

    explicit RotateTool(QObject* parent = nullptr) : Tool(parent) { }

    QString GetId() const override
    {
        return QStringLiteral("rotate");
    }

    QString GetDisplayName() const override
    {
        return tr("Rotate");
    }

    QKeySequence GetShortcut() const override
    {
        return QKeySequence(Qt::Key_E);
    }

    QString GetGizmoId() const override
    {
        return QStringLiteral("rotate");
    }

    InputHandler* CreateInputHandler() override
    {
        // Input handling is centralized in ToolManager
        return nullptr;
    }

private:

    static const bool registered;

};

#endif
