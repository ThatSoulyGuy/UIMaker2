#ifndef CORE_ASSETCONTEXT_HPP
#define CORE_ASSETCONTEXT_HPP

#include <QString>

// Process-wide resolver for scene-relative asset paths.
//
// imagePath / fontPath / iconPath values stored in components and scene.json
// are ALWAYS relative to the directory that contains scene.json (the project
// root). The editor needs that root to load pixmaps/fonts for preview; rather
// than thread a base directory through every Update()/Paint()/SetXxxPath()
// signature, components consult this single context. The root is owned by the
// SceneDocument and mirrored here on every change.
class AssetContext
{
public:

    static void SetBaseDir(const QString& dir);

    static QString BaseDir();

    static bool HasBaseDir();

    // Relative scene path -> absolute filesystem path. Empty, already-absolute,
    // or root-less inputs are returned unchanged so behaviour degrades
    // gracefully before a project root has been chosen.
    static QString Resolve(const QString& rel);

    // A stored path is valid only if it is relative and does not escape the
    // project root via "..". Empty is allowed (means "no asset").
    static bool IsValidRelative(const QString& v);

    // Copy an arbitrary source file into {baseDir}/assets/, de-duplicating
    // filename collisions (reusing a byte-identical existing copy), and return
    // the stored relative key ("assets/name.ext"). Empty on failure / no root.
    static QString ImportToAssets(const QString& srcAbs);

private:

    static QString& BaseDirRef();

    static bool SameContents(const QString& a, const QString& b);
};

#endif
