#ifndef ROTATETOOL_HPP
#define ROTATETOOL_HPP

#include "tools/Tool.hpp"

class RotateTool : public Tool
{
    Q_OBJECT

public:

    explicit RotateTool(QObject* parent = nullptr);

    QString GetId() const override;

    QString GetDisplayName() const override;

    QKeySequence GetShortcut() const override;

    QString GetGizmoId() const override;

};

#endif
