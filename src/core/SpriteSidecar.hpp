#ifndef CORE_SPRITESIDECAR_HPP
#define CORE_SPRITESIDECAR_HPP

#include <QString>

#include "core/PixelDraw.hpp"

// Optional per-texture metadata, read from an XML file sitting next to the
// image: foo.png -> foo.xml.
//
//     <sprite>
//         <slice left="3" top="3" right="3" bottom="4"/>
//     </sprite>
//
// A slice pins the four corners and the four edges so they are never repeated
// or scaled; only the interior tiles. That is what keeps a button's border from
// warping when the button is resized.
//
// The file is optional and absent is not an error - the texture then has no
// slice and wraps as a single interior band.
namespace SpriteSidecar
{
    struct Meta
    {
        PixelDraw::Slice slice;
        bool hasSlice = false;
    };

    // `relPath` is a scene-relative asset path, as stored in a component's
    // imagePath. Resolution goes through AssetContext, so this follows the
    // project root.
    //
    // Cached on the resolved path and invalidated by mtime, because
    // ImageComponent::Update re-stats its asset on EVERY refresh - parsing XML
    // per refresh per element would be pathological. Absence is cached too, so
    // the common no-sidecar case costs one stat.
    Meta MetaFor(const QString& relPath);

    // The sidecar path a given image would use, for copy-on-export and import.
    QString SidecarPathFor(const QString& imagePath);

    // Drop everything. Call when the project root changes, since the same
    // relative path then resolves somewhere else entirely.
    void Invalidate();
}

#endif
