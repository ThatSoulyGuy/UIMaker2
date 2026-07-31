#include "core/GridSnap.hpp"

#include <cmath>

// Function-local statics so there is exactly one instance and no static-init
// ordering concerns (mirrors AssetContext).
static bool& EnabledRef()
{
    static bool value = false;
    return value;
}

static int& DivXRef()
{
    static int value = 16;
    return value;
}

static int& DivYRef()
{
    static int value = 16;
    return value;
}

bool GridSnap::Enabled()
{
    return EnabledRef();
}

void GridSnap::SetEnabled(bool on)
{
    EnabledRef() = on;
}

int GridSnap::DivisionsX()
{
    return DivXRef();
}

int GridSnap::DivisionsY()
{
    return DivYRef();
}

void GridSnap::SetDivisions(int x, int y)
{
    if (x > 0)
        DivXRef() = x;
    if (y > 0)
        DivYRef() = y;
}

QPointF GridSnap::Snap(const QPointF& p, const QRectF& canvas)
{
    const int dx = DivXRef();
    const int dy = DivYRef();

    if (!EnabledRef() || dx <= 0 || dy <= 0 || canvas.width() <= 0.0 || canvas.height() <= 0.0)
        return p;

    const double cellW = canvas.width() / dx;
    const double cellH = canvas.height() / dy;

    const double x = canvas.left() + std::round((p.x() - canvas.left()) / cellW) * cellW;
    const double y = canvas.top() + std::round((p.y() - canvas.top()) / cellH) * cellH;

    return QPointF(x, y);
}
