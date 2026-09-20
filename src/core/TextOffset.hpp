#ifndef CORE_TEXTOFFSET_HPP
#define CORE_TEXTOFFSET_HPP

#include <QPointF>
#include <QRectF>

// Every component that renders user-authored text carries a "textOffset": a
// nudge, in scene units, applied to the rectangle the glyphs are laid out in.
//
// It exists because the glyph box a component computes is never quite where a
// given font wants to sit. A 9-sliced button's skin has its own optical centre
// that has nothing to do with the rect's geometric centre; a bitmap font's
// baseline sits a pixel or two off from where Qt's metrics put it. Without a
// nudge the only fixes were to move the whole element (which moves the skin
// too) or to pad the text with spaces.
//
// Scene units, NOT texels: it shares a space with pixelSize and the element
// rect, which is what an author is eyeballing it against. In PixelGrid mode it
// is snapped to whole virtual pixels before use, because a half-pixel nudge is
// exactly the thing that mode exists to prevent.
namespace TextOffset
{
    // The offset as it will actually be applied. Identity in Continuous mode,
    // so call sites never have to test the model.
    QPointF Resolve(const QPointF& offset);

    // The text rect, nudged. The element's own rect is left alone, so an
    // offset moves the glyphs WITHIN the element rather than resizing it, and
    // a component that clips to its bounds still clips.
    QRectF Apply(const QRectF& textRect, const QPointF& offset);
}

#endif
