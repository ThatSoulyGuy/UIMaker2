#ifndef CORE_PIXELDRAW_HPP
#define CORE_PIXELDRAW_HPP

#include <QRect>
#include <QRectF>
#include <QVector>
#include <QSize>
#include <QSizeF>

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
    //
    // FillAuto is the default and follows the document's rendering model:
    // stretch in Continuous, wrap in PixelGrid. Switching a document to
    // PixelGrid should make its textures texel-exact immediately - having to
    // visit every image and flip a per-element switch is not a pixel-perfect
    // mode, it is a pixel-perfect checkbox.
    //
    // 0 keeps its Continuous meaning exactly (Auto resolves to Stretch there),
    // so no existing scene changes appearance.
    enum Fill
    {
        FillAuto    = 0,
        FillStretch = 1,   // always scale to fit. Never texel-exact.
        FillWrap    = 2    // always repeat and crop. Texel-exact.
    };

    // Resolve FillAuto against the live rendering model.
    Fill ResolveFill(int fill);

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

    // The scene-unit size at which a texture of `texels` renders exactly 1:1 -
    // one texel per virtual pixel. This is what an element's intrinsic size
    // must be, NOT the raw texel count.
    QSizeF NaturalSize(const QSize& texels);

    // Round a length/rect to whole virtual pixels. Identity in Continuous mode.
    double SnapLength(double v);
    QRectF SnapRect(const QRectF& r);

    // The per-element knobs, shared by every textured component.
    //
    // They are declared as four separate int Q_PROPERTYs on each component
    // rather than on a common base class: PropertyEditorPanel and UiBinWriter
    // both walk metaObject() from propertyOffset(), which restricts them to the
    // most-derived class, so a property on an intermediate base would silently
    // vanish from both the inspector and the bake.
    struct TextureParams
    {
        int fill = FillAuto;      // follows the document's rendering model
        int anchor = Center;
        int cropOffsetX = 0;
        int cropOffsetY = 0;
    };

    // Clamping setters, so a hand-edited scene.json or a stale .uibin cannot
    // put an out-of-range value into the draw path.
    int ClampFill(int v) noexcept;
    int ClampAnchor(int v) noexcept;

    // Corner radius for the placeholder mocks: 0 in PixelGrid mode, since a
    // rounded corner cannot be drawn on a pixel grid without either
    // antialiasing or an arbitrary stair-step. Identity in Continuous mode.
    double Radius(double r);

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
                     int fill);
}

#endif
