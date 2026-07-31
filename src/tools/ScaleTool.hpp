#ifndef SCALETOOL_HPP
#define SCALETOOL_HPP

#include "tools/Tool.hpp"

class ScaleTool : public Tool
{
    Q_OBJECT

public:

    explicit ScaleTool(QObject* parent = nullptr);

    QString GetId() const override;

    QString GetDisplayName() const override;

    QKeySequence GetShortcut() const override;

    QString GetGizmoId() const override;

};

#endif
