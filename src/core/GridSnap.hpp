#ifndef CORE_GRIDSNAP_HPP
#define CORE_GRIDSNAP_HPP

#include <QPointF>
#include <QRectF>
#include <QSizeF>

// Process-wide grid-snapping settings for the editor (same all-static pattern
// as AssetContext). The grid divides the design canvas into DivisionsX by
// DivisionsY cells; when enabled, gizmo drags snap element positions to the
// nearest cell intersection. Kept global so the View>Snapping menu and the
// gizmo/transform code can share it without threading it through the document,
// which is replaced on every load.
class GridSnap
{
public:

    static bool Enabled();
    static void SetEnabled(bool on);

    static int DivisionsX();
    static int DivisionsY();
    static void SetDivisions(int x, int y);

    // The snap cell, in scene units.
    //
    // In PixelGrid mode this IS the virtual pixel: square by construction, and
    // the division counts are derived from the canvas rather than set by hand.
    // A pixel-art grid that is not square is not a pixel grid - the old
    // independent X/Y divisions gave 6.0 x 4.5 cells on a 1920x1080 canvas.
    //
    // In Continuous mode it stays the canvas divided by the X/Y counts, where
    // a non-square grid is a legitimate layout aid.
    static QSizeF CellSize(const QRectF& canvas);

    // True when the cell is currently driven by PixelModel rather than by the
    // division counts, so the UI can present the unit instead of X/Y.
    static bool DrivenByPixelModel();

    // Snap a scene-space point to the nearest grid intersection of `canvas`.
    // Returns the point unchanged when snapping is off or the parameters are
    // degenerate. The grid is anchored at the canvas top-left and extends
    // conceptually beyond the canvas, so points outside it still snap.
    static QPointF Snap(const QPointF& p, const QRectF& canvas);
};

#endif
