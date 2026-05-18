#ifndef CORE_ASSETCONTEXT_HPP
#define CORE_ASSETCONTEXT_HPP

#include <QString>
#include <QDir>
#include <QFile>
#include <QFileInfo>

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

    static void SetBaseDir(const QString& dir)
    {
        BaseDirRef() = dir;
    }

    static QString BaseDir()
    {
        return BaseDirRef();
    }

    static bool HasBaseDir()
    {
        return !BaseDirRef().isEmpty();
    }

    // Relative scene path -> absolute filesystem path. Empty, already-absolute,
    // or root-less inputs are returned unchanged so behaviour degrades
    // gracefully before a project root has been chosen.
    static QString Resolve(const QString& rel)
    {
        if (rel.isEmpty() || QDir::isAbsolutePath(rel) || BaseDirRef().isEmpty())
            return rel;

        return QDir(BaseDirRef()).filePath(rel);
    }

    // A stored path is valid only if it is relative and does not escape the
    // project root via "..". Empty is allowed (means "no asset").
    static bool IsValidRelative(const QString& v)
    {
        if (v.isEmpty())
            return true;

        if (QDir::isAbsolutePath(v))
            return false;

        const QStringList parts = v.split(QLatin1Char('/'), Qt::SkipEmptyParts);

        return !parts.contains(QStringLiteral(".."));
    }

    // Copy an arbitrary source file into {baseDir}/assets/, de-duplicating
    // filename collisions (reusing a byte-identical existing copy), and return
    // the stored relative key ("assets/name.ext"). Empty on failure / no root.
    static QString ImportToAssets(const QString& srcAbs)
    {
        if (BaseDirRef().isEmpty() || !QFile::exists(srcAbs))
            return QString();

        QDir root(BaseDirRef());
        root.mkpath(QStringLiteral("assets"));

        const QFileInfo fi(srcAbs);
        const QString baseName = fi.completeBaseName();
        const QString suffix = fi.suffix();
        QString fileName = fi.fileName();

        int counter = 1;
        while (QFile::exists(root.filePath(QStringLiteral("assets/") + fileName)))
        {
            if (SameContents(srcAbs, root.filePath(QStringLiteral("assets/") + fileName)))
                return QStringLiteral("assets/") + fileName;

            fileName = baseName + QLatin1Char('_') + QString::number(counter++)
                       + (suffix.isEmpty() ? QString() : QLatin1Char('.') + suffix);
        }

        const QString rel = QStringLiteral("assets/") + fileName;

        if (!QFile::copy(srcAbs, root.filePath(rel)))
            return QString();

        return rel;
    }

private:

    static QString& BaseDirRef()
    {
        static QString dir;
        return dir;
    }

    static bool SameContents(const QString& a, const QString& b)
    {
        QFile fa(a);
        QFile fb(b);

        if (!fa.open(QIODevice::ReadOnly) || !fb.open(QIODevice::ReadOnly))
            return false;

        if (fa.size() != fb.size())
            return false;

        return fa.readAll() == fb.readAll();
    }
};

#endif
