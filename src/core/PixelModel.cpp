#include "core/PixelModel.hpp"

#include <cmath>

// Function-local statics so there is exactly one instance and no static-init
// ordering concerns (mirrors GridSnap / AssetContext).
static PixelModel::Mode& ModeRef()
{
    static PixelModel::Mode value = PixelModel::Mode::Continuous;
    return value;
}

static double& UnitRef()
{
    static double value = 1.0;
    return value;
}

PixelModel::Mode PixelModel::GetMode()
{
    return ModeRef();
}

void PixelModel::SetMode(Mode m)
{
    ModeRef() = m;
}

double PixelModel::GetUnit()
{
    return UnitRef();
}

void PixelModel::SetUnit(double u)
{
    if (u > 0.0)
        UnitRef() = u;
}

bool PixelModel::PixelSnap()
{
    return ModeRef() == Mode::PixelGrid && UnitRef() > 0.0;
}

double PixelModel::SnapValue(double v)
{
    if (!PixelSnap())
        return v;

    const double u = UnitRef();
    return std::round(v / u) * u;
}

QPointF PixelModel::SnapPoint(const QPointF& p)
{
    if (!PixelSnap())
        return p;

    return QPointF(SnapValue(p.x()), SnapValue(p.y()));
}
