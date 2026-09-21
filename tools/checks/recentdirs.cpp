// Dialog directory memory.
//
// The reported annoyance: the image browser opened at the OS default every
// time, so adding ten sprites meant navigating to the same folder ten times.
//
// These checks redirect QSettings to an INI file inside a temporary
// directory, so they never read or write the real UIMaker2 preferences - and,
// unlike a throwaway native identity, leave nothing behind in the user's
// Preferences folder either. (setPath has no effect on NativeFormat on macOS,
// which is why the format is switched too.)
#include "checks.hpp"

#include "core/AssetContext.hpp"
#include "core/RecentDirs.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

#include <cstdio>

namespace
{
    QString Touch(const QDir& dir, const QString& name)
    {
        const QString path = dir.filePath(name);

        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write("x");
        f.close();

        return path;
    }
}

void CheckRecentDirs()
{
    std::fprintf(stderr, "dialog directory memory\n");

    const QString realOrg = QCoreApplication::organizationName();
    const QString realApp = QCoreApplication::applicationName();
    const QSettings::Format realFormat = QSettings::defaultFormat();

    QTemporaryDir settingsHome;

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsHome.path());
    QCoreApplication::setOrganizationName(QStringLiteral("UIMaker2ChecksOnly"));
    QCoreApplication::setApplicationName(QStringLiteral("RecentDirsCheck"));

    {
        QSettings fresh;
        fresh.clear();
        fresh.sync();

        check(fresh.fileName().startsWith(settingsHome.path()),
              "the check's settings live in a temp dir, not the real preferences");
    }

    QTemporaryDir tmp;
    QDir root(tmp.path());

    root.mkpath(QStringLiteral("sprites"));
    root.mkpath(QStringLiteral("fonts"));
    root.mkpath(QStringLiteral("game/ui"));
    root.mkpath(QStringLiteral("scenes"));

    const QString spriteDir = root.filePath(QStringLiteral("sprites"));
    const QString fontDir   = root.filePath(QStringLiteral("fonts"));
    const QString bakeDir   = root.filePath(QStringLiteral("game/ui"));
    const QString sceneDir  = root.filePath(QStringLiteral("scenes"));

    AssetContext::SetBaseDir(QString());

    check(RecentDirs::For(RecentDirs::kImage).isEmpty(),
          "with nothing known, a dialog gets no start directory and the OS decides");

    // The actual fix: pick a file, and the next dialog of that kind starts
    // in its folder.
    RecentDirs::RememberFile(RecentDirs::kImage, Touch(QDir(spriteDir), QStringLiteral("hero.png")));

    check(RecentDirs::For(RecentDirs::kImage) == spriteDir,
          "after choosing an image, the image dialog reopens in that folder");

    // A fresh QSettings is what the next launch sees. Same store, same value.
    {
        QSettings nextLaunch;

        check(nextLaunch.value(QStringLiteral("io/lastImageDir")).toString() == spriteDir,
              "and it is persisted, so it survives the session");
    }

    // Kinds do not stomp each other.
    RecentDirs::RememberFile(RecentDirs::kFont, Touch(QDir(fontDir), QStringLiteral("face.ttf")));

    check(RecentDirs::For(RecentDirs::kFont) == fontDir, "the font dialog remembers its own folder");
    check(RecentDirs::For(RecentDirs::kImage) == spriteDir, "and choosing a font leaves the image folder alone");

    // Baking into the game's asset tree must not drag the scene dialogs along.
    QSettings().setValue(QStringLiteral("io/lastDir"), sceneDir);

    RecentDirs::RememberFile(RecentDirs::kBake, Touch(QDir(bakeDir), QStringLiteral("hud.uibin")));

    check(RecentDirs::For(RecentDirs::kBake) == bakeDir, "the bake dialog remembers the bake folder");
    check(QSettings().value(QStringLiteral("io/lastDir")).toString() == sceneDir,
          "and baking does not move where scene files open");

    // Fallbacks, in order: the project root, then the scene folder.
    {
        QSettings s;
        s.remove(QStringLiteral("io/lastProjectRootDir"));
        s.sync();
    }

    AssetContext::SetBaseDir(root.path());

    check(RecentDirs::For(RecentDirs::kProjectRoot) == root.path(),
          "a kind never used before starts at the project root");

    AssetContext::SetBaseDir(QString());

    check(RecentDirs::For(RecentDirs::kProjectRoot) == sceneDir,
          "and with no project root, at the last scene folder");

    // A folder that has since been deleted is not offered.
    const QString gone = root.filePath(QStringLiteral("temporary"));
    root.mkpath(QStringLiteral("temporary"));

    RecentDirs::RememberDir(RecentDirs::kImage, gone);
    check(RecentDirs::For(RecentDirs::kImage) == gone, "a remembered folder is used while it exists");

    QDir(gone).removeRecursively();

    check(RecentDirs::For(RecentDirs::kImage) != gone,
          "but a folder that no longer exists is skipped rather than handed to the dialog");
    check(RecentDirs::For(RecentDirs::kImage) == sceneDir, "falling through to the next best guess");

    // Nothing is recorded for a cancelled dialog.
    const QString before = RecentDirs::For(RecentDirs::kFont);
    RecentDirs::RememberFile(RecentDirs::kFont, QString());

    check(RecentDirs::For(RecentDirs::kFont) == before, "cancelling a dialog changes nothing");

    {
        QSettings cleanup;
        cleanup.clear();
        cleanup.sync();
    }

    QCoreApplication::setOrganizationName(realOrg);
    QCoreApplication::setApplicationName(realApp);
    QSettings::setDefaultFormat(realFormat);

    AssetContext::SetBaseDir(QString());
}
