#ifndef CORE_PIXELDRAW_HPP
#define CORE_PIXELDRAW_HPP

#include <QRect>
#include <QRectF>
#include <QVector>

class QPainter;
class QPixmap;

// Texel-exact texture drawing for the PixelGrid rendering model.
//
// The rule the whole file exists to enforce: ONE TEXEL IS ONE VIRTUAL PIXEL.
// A texture is never scaled to fit its element. When the element is larger the
// texture REPEATS; when it is smaller the texture is CROPPED. An optional
// 9-slice pins the edges so only the interior repeats.
//
// Everything reduces to one idea: a destination band is a window onto an
// infinitely repeated source strip, and the crop anchor decides where that
// window sits. Wrap, crop and the centre of a 9-slice are then the same code.
namespace PixelDraw
{
    // Where the texture sits when it does not exactly fill the destination.
    // Plain ints, not a Q_ENUM: a Q_ENUM declared in a holder class reports
    // isEnumType() == false on the property, which sends PropertyEditorPanel
    // through every branch to the dead read-only QLabel fallback. Components
    // expose these as int Q_PROPERTYs with clamping setters and the panel
    // dispatches on the property NAME, like it already does for "anchors".
    enum Side { SideNear = 0, SideCenter = 1, SideFar = 2 };

    enum Anchor
    {
        TopLeft = 0, Top = 1, TopRight = 2,
        Left    = 3, Center = 4, Right = 5,
        BottomLeft = 6, Bottom = 7, BottomRight = 8
    };

    Side SideX(int anchor) noexcept;
    Side SideY(int anchor) noexcept;

    // How the interior behaves when it does not match the destination.
    enum Fill
    {
        FillStretch = 0,   // legacy: scale to fit. Not texel-exact.
        FillWrap    = 1    // repeat and crop. Texel-exact.
    };

    // 9-slice insets in texels. A null slice means the whole texture is one
    // interior band.
    struct Slice
    {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;

        bool IsNull() const noexcept;

        // A slice is only usable when the insets leave a non-empty interior on
        // both axes. 3/3/3/4 is fine on a 200x20 image and impossible on a 4x4
        // one; an unusable axis falls back to plain wrapping rather than being
        // rejected, so a bad sidecar degrades instead of blanking the element.
        bool FitsWidth(int w) const noexcept;
        bool FitsHeight(int h) const noexcept;
    };

    // One run of texels copied 1:1. srcStart and dstStart are texel indices on
    // a single axis and `len` is shared by both, which IS the one-texel-to-one-
    // virtual-pixel guarantee - there is no scale factor anywhere in a Span.
    struct Span
    {
        int srcStart = 0;
        int dstStart = 0;
        int len = 0;
    };

    // Solve one axis. Pure geometry, no QPainter, so the checks target can
    // assert on it directly.
    //
    //   n          source extent in texels
    //   a, b       near/far slice insets (0 for an unsliced axis)
    //   d          destination extent in VIRTUAL PIXELS
    //   side       which end the crop favours
    //   cropOffset signed texel shift of the tiling phase
    //
    // Appends to `out` and returns false on degenerate input. The near and far
    // slice bands are emitted first and never repeat or scale; only the
    // interior tiles, which is what "sliced sections do not wrap in their
    // specified direction" means.
    bool SolveAxis(int n, int a, int b, int d, Side side, int cropOffset, QVector<Span>& out);

    // Scene units per texel: PixelModel's unit in PixelGrid mode, 1.0 otherwise.
    double Unit();

    // Round a length/rect to whole virtual pixels. Identity in Continuous mode.
    double SnapLength(double v);
    QRectF SnapRect(const QRectF& r);

    // Draw `tex` into `dest` (SCENE units). In FillWrap the texture is tiled and
    // cropped per the slice and anchor; in FillStretch it is scaled, which is
    // the pre-existing behaviour and what Continuous mode keeps using.
    //
    // `dest` is expected to already be on the virtual-pixel lattice - snapping
    // happens once, on localRect in SceneElementItem::RefreshFromComponents, so
    // that boundingRect() and the painted area agree. Expanding a rect here
    // would under-invalidate the scene and leave drag trails.
    void DrawTexture(QPainter* painter,
                     const QRectF& dest,
                     const QPixmap& tex,
                     const Slice& slice,
                     int anchor,
                     int cropOffsetX,
                     int cropOffsetY,
                     Fill fill);
}

#endif
