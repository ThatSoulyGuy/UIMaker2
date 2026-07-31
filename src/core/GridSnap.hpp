#ifndef CORE_GRIDSNAP_HPP
#define CORE_GRIDSNAP_HPP

#include <QPointF>
#include <QRectF>

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

    // Snap a scene-space point to the nearest grid intersection of `canvas`.
    // Returns the point unchanged when snapping is off or the parameters are
    // degenerate. The grid is anchored at the canvas top-left and extends
    // conceptually beyond the canvas, so points outside it still snap.
    static QPointF Snap(const QPointF& p, const QRectF& canvas);
};

#endif
