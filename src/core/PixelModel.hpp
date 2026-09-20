#ifndef CORE_PIXELMODEL_HPP
#define CORE_PIXELMODEL_HPP

#include <QPointF>

// Process-wide "rendering model" for the active document (same all-static
// pattern as GridSnap / AssetContext; the editor is single-document).
//
//   Continuous - positions are real numbers, layout math is exact, sub-pixel
//                centering is correct. For high-fidelity engines (Vulkan/RTX)
//                where you can't see individual pixels and forcing integer
//                alignment would only add asymmetric margins and jitter.
//
//   PixelGrid  - computed layout geometry (anchor/centre resolution and the
//                layout containers) rounds to whole virtual pixels - a square
//                unit measured in scene units - so pixel-art UIs stay pixel-
//                perfect. Centering an even child in an odd space can't be both
//                symmetric AND on-grid, so a <=0.5-unit centre bias is accepted
//                on purpose, exactly as hardcoded voxel-game layout does.
//
// The model is a DOCUMENT property (serialised in scene.json); it is mirrored
// here so the anchor math and layout components can read it without threading
// it through every Update()/paint call.
class PixelModel
{
public:

    enum class Mode { Continuous, PixelGrid };

    static Mode GetMode();
    static void SetMode(Mode m);

    // Scene units per virtual pixel (square). Only meaningful in PixelGrid mode.
    static double GetUnit();
    static void SetUnit(double u);

    // True when computed geometry should snap to the virtual-pixel grid.
    static bool PixelSnap();

    // Round to the nearest whole unit. Identity in Continuous mode (or when the
    // unit is degenerate), so call sites can wrap positions unconditionally.
    static double SnapValue(double v);
    static QPointF SnapPoint(const QPointF& p);
};

#endif
