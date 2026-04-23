#ifndef SCALETOOL_HPP
#define SCALETOOL_HPP

#include "tools/Tool.hpp"
#include "input/InputHandler.hpp"

class ScaleTool : public Tool
{
    Q_OBJECT

public:

    explicit ScaleTool(QObject* parent = nullptr) : Tool(parent) { }

    QString GetId() const override
    {
        return QStringLiteral("scale");
    }

    QString GetDisplayName() const override
    {
        return tr("Scale");
    }

    QKeySequence GetShortcut() const override
    {
        return QKeySequence(Qt::Key_R);
    }

    QString GetGizmoId() const override
    {
        return QStringLiteral("scale");
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
