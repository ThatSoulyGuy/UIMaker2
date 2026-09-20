#include "core/PixelDraw.hpp"

#include "core/PixelModel.hpp"

#include <QPainter>
#include <QPixmap>

#include <algorithm>
#include <cmath>

namespace PixelDraw
{
    bool Slice::IsNull() const noexcept
    {
        return left <= 0 && top <= 0 && right <= 0 && bottom <= 0;
    }

    bool Slice::FitsWidth(int w) const noexcept
    {
        return left >= 0 && right >= 0 && left + right < w;
    }

    bool Slice::FitsHeight(int h) const noexcept
    {
        return top >= 0 && bottom >= 0 && top + bottom < h;
    }

    int ClampFill(int v) noexcept
    {
        return (v == FillWrap) ? FillWrap : FillStretch;
    }

    int ClampAnchor(int v) noexcept
    {
        return (v < TopLeft || v > BottomRight) ? Center : v;
    }

    Side SideX(int anchor) noexcept
    {
        switch (anchor)
        {
            case TopLeft: case Left: case BottomLeft:    return SideNear;
            case TopRight: case Right: case BottomRight: return SideFar;
            default:                                     return SideCenter;
        }
    }

    Side SideY(int anchor) noexcept
    {
        switch (anchor)
        {
            case TopLeft: case Top: case TopRight:          return SideNear;
            case BottomLeft: case Bottom: case BottomRight:  return SideFar;
            default:                                         return SideCenter;
        }
    }

    bool SolveAxis(int n, int a, int b, int d, Side side, int cropOffset, QVector<Span>& out)
    {
        if (n <= 0 || d <= 0)
            return false;

        // An inset pair that leaves no interior cannot describe a 9-slice on
        // this axis; fall back to plain wrapping of the whole texture.
        if (a < 0 || b < 0 || a + b >= n)
        {
            a = 0;
            b = 0;
        }

        // When the destination is narrower than the two insets, shrink them
        // rather than dropping one edge entirely: half the space to each.
        const int da = std::min(a, d / 2);
        const int db = std::min(b, d - da);

        if (da > 0)
            out.append(Span{ 0, 0, da });

        if (db > 0)
            out.append(Span{ n - db, d - db, db });

        const int dstMid = d - da - db;   // destination interior, virtual pixels
        const int srcMid = n - a - b;     // source interior period, texels

        if (dstMid <= 0 || srcMid <= 0)
            return true;

        // Phase: how many texels into the interior the first visible column is.
        //
        //   Near   start at the interior's first texel; the shortfall lands at
        //          the far end.
        //   Far    start so the LAST visible texel is the interior's last one.
        //   Center split the shortfall evenly between the two ends.
        //
        // `short` is how many texels of a whole tile are missing from the last
        // repeat; it is 0 when the interior tiles exactly.
        const int shortfall = (srcMid - (dstMid % srcMid)) % srcMid;

        int phase = 0;

        if (side == SideFar)
            phase = shortfall;
        else if (side == SideCenter)
            phase = shortfall / 2;

        phase = ((phase + cropOffset) % srcMid + srcMid) % srcMid;

        // Walk the repeated strip. Starting at -phase makes the first tile
        // partially clipped by exactly `phase` texels, which is what shifts the
        // window; every emitted span has srcLen == dstLen.
        for (int x = -phase; x < dstMid; x += srcMid)
        {
            const int visibleStart = std::max(0, x);
            const int visibleEnd = std::min(dstMid, x + srcMid);
            const int len = visibleEnd - visibleStart;

            if (len <= 0)
                continue;

            out.append(Span{ a + (visibleStart - x), da + visibleStart, len });
        }

        return true;
    }

    double Unit()
    {
        if (!PixelModel::PixelSnap())
            return 1.0;

        const double u = PixelModel::GetUnit();

        return u > 0.0 ? u : 1.0;
    }

    double SnapLength(double v)
    {
        if (!PixelModel::PixelSnap())
            return v;

        const double u = Unit();

        // Never collapse a visible element to nothing: a sub-unit element still
        // occupies one virtual pixel.
        return std::max(u, std::round(v / u) * u);
    }

    double Radius(double r)
    {
        return PixelModel::PixelSnap() ? 0.0 : r;
    }

    QRectF SnapRect(const QRectF& r)
    {
        if (!PixelModel::PixelSnap())
            return r;

        const double u = Unit();

        const double x = std::round(r.left() / u) * u;
        const double y = std::round(r.top() / u) * u;

        return QRectF(x, y, SnapLength(r.width()), SnapLength(r.height()));
    }

    void DrawTexture(QPainter* painter,
                     const QRectF& dest,
                     const QPixmap& tex,
                     const Slice& slice,
                     int anchor,
                     int cropOffsetX,
                     int cropOffsetY,
                     Fill fill)
    {
        if (!painter || tex.isNull() || dest.isEmpty())
            return;

        const QRectF src(0.0, 0.0, tex.width(), tex.height());

        if (fill == FillStretch || !PixelModel::PixelSnap())
        {
            painter->drawPixmap(dest, tex, src);
            return;
        }

        const double u = Unit();

        // Destination in whole virtual pixels. It is expected to be on the
        // lattice already; rounding here only absorbs floating-point dust.
        const int dw = std::max(1, int(std::lround(dest.width() / u)));
        const int dh = std::max(1, int(std::lround(dest.height() / u)));

        const int tw = tex.width();
        const int th = tex.height();

        Slice s = slice;

        if (!s.FitsWidth(tw))
        {
            s.left = 0;
            s.right = 0;
        }

        if (!s.FitsHeight(th))
        {
            s.top = 0;
            s.bottom = 0;
        }

        QVector<Span> cols;
        QVector<Span> rows;

        if (!SolveAxis(tw, s.left, s.right, dw, SideX(anchor), cropOffsetX, cols))
            return;

        if (!SolveAxis(th, s.top, s.bottom, dh, SideY(anchor), cropOffsetY, rows))
            return;

        // Nearest-neighbour: a texel must land on the grid, not be resampled.
        const bool smooth = painter->renderHints().testFlag(QPainter::SmoothPixmapTransform);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

        // The 3x3 (or NxM once tiling) is the cross product of the two axis
        // solutions, which is exactly why corners never repeat, edges repeat on
        // one axis and the interior repeats on both.
        for (const Span& c : cols)
        {
            for (const Span& r : rows)
            {
                const QRectF d(dest.left() + c.dstStart * u,
                               dest.top()  + r.dstStart * u,
                               c.len * u,
                               r.len * u);

                const QRectF sr(c.srcStart, r.srcStart, c.len, r.len);

                painter->drawPixmap(d, tex, sr);
            }
        }

        painter->setRenderHint(QPainter::SmoothPixmapTransform, smooth);
    }
}
