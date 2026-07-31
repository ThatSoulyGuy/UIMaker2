#ifndef TRANSLATETOOL_HPP
#define TRANSLATETOOL_HPP

#include "tools/Tool.hpp"

class TranslateTool : public Tool
{
    Q_OBJECT

public:

    explicit TranslateTool(QObject* parent = nullptr);

    QString GetId() const override;

    QString GetDisplayName() const override;

    QKeySequence GetShortcut() const override;

    QString GetGizmoId() const override;

};

#endif
