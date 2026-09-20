#include "core/GridSnap.hpp"

#include "core/PixelModel.hpp"

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

bool GridSnap::DrivenByPixelModel()
{
    return PixelModel::PixelSnap() && PixelModel::GetUnit() > 0.0;
}

QSizeF GridSnap::CellSize(const QRectF& canvas)
{
    if (DrivenByPixelModel())
    {
        const double u = PixelModel::GetUnit();

        return QSizeF(u, u);
    }

    const int dx = DivXRef();
    const int dy = DivYRef();

    if (dx <= 0 || dy <= 0 || canvas.width() <= 0.0 || canvas.height() <= 0.0)
        return QSizeF();

    return QSizeF(canvas.width() / dx, canvas.height() / dy);
}

QPointF GridSnap::Snap(const QPointF& p, const QRectF& canvas)
{
    if (!EnabledRef())
        return p;

    const QSizeF cell = CellSize(canvas);

    if (cell.width() <= 0.0 || cell.height() <= 0.0)
        return p;

    const double x = canvas.left() + std::round((p.x() - canvas.left()) / cell.width()) * cell.width();
    const double y = canvas.top() + std::round((p.y() - canvas.top()) / cell.height()) * cell.height();

    return QPointF(x, y);
}
