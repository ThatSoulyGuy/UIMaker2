#ifndef TRANSLATETOOL_HPP
#define TRANSLATETOOL_HPP

#include "tools/Tool.hpp"
#include "input/InputHandler.hpp"

class TranslateTool : public Tool
{
    Q_OBJECT

public:

    explicit TranslateTool(QObject* parent = nullptr) : Tool(parent) { }

    QString GetId() const override
    {
        return QStringLiteral("translate");
    }

    QString GetDisplayName() const override
    {
        return tr("Move");
    }

    QKeySequence GetShortcut() const override
    {
        return QKeySequence(Qt::Key_W);
    }

    QString GetGizmoId() const override
    {
        return QStringLiteral("translate");
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
