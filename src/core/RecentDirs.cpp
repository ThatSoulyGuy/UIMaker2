#include "core/RecentDirs.hpp"

#include "core/AssetContext.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace RecentDirs
{
    const char* const kImage       = "io/lastImageDir";
    const char* const kFont        = "io/lastFontDir";
    const char* const kProjectRoot = "io/lastProjectRootDir";
    const char* const kBake        = "io/lastBakeDir";

    namespace
    {
        // The scene dialogs in MainWindow have always used this one; it is the
        // last resort here so that a brand-new install still opens somewhere
        // related to the work rather than at the home folder.
        const char* const kSceneDir = "io/lastDir";

        bool Usable(const QString& dir)
        {
            return !dir.isEmpty() && QDir(dir).exists();
        }
    }

    QString For(const char* kind)
    {
        QSettings settings;

        const QString remembered = settings.value(QString::fromLatin1(kind)).toString();

        if (Usable(remembered))
            return remembered;

        // Never been here before. The project root holds assets/, so it is the
        // best guess for an asset dialog.
        if (AssetContext::HasBaseDir())
        {
            const QString base = AssetContext::BaseDir();

            if (Usable(base))
                return base;
        }

        const QString scene = settings.value(QString::fromLatin1(kSceneDir)).toString();

        return Usable(scene) ? scene : QString();
    }

    void RememberFile(const char* kind, const QString& filePath)
    {
        if (filePath.isEmpty())
            return;

        RememberDir(kind, QFileInfo(filePath).absolutePath());
    }

    void RememberDir(const char* kind, const QString& dir)
    {
        if (!Usable(dir))
            return;

        QSettings().setValue(QString::fromLatin1(kind), dir);
    }
}
