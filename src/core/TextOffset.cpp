#include "core/TextOffset.hpp"

#include "core/PixelModel.hpp"

namespace TextOffset
{
    QPointF Resolve(const QPointF& offset)
    {
        // SnapPoint is the identity in Continuous mode and when the unit is
        // degenerate, so this is unconditional on purpose.
        return PixelModel::SnapPoint(offset);
    }

    QRectF Apply(const QRectF& textRect, const QPointF& offset)
    {
        return textRect.translated(Resolve(offset));
    }
}
