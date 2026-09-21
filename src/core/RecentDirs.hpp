#ifndef CORE_RECENTDIRS_HPP
#define CORE_RECENTDIRS_HPP

#include <QString>

// Where each kind of file dialog should open.
//
// A dialog that starts at the OS default every time makes you re-navigate to
// the same folder on every single asset you add. Remembering it is only
// useful if it SURVIVES the session, so this is backed by QSettings.
//
// Kept per kind rather than as one "last folder": choosing a font should not
// move where the image browser opens, and neither should move where scene
// files open. An unknown kind falls back through the places most likely to be
// right before giving up and letting the OS decide.
namespace RecentDirs
{
    // Dialog kinds. These are settings keys; changing one forgets it.
    extern const char* const kImage;
    extern const char* const kFont;
    extern const char* const kProjectRoot;
    extern const char* const kBake;

    // Where a dialog of this kind should start, or an empty string to let the
    // platform choose. Falls back to the project root, then to the scene
    // file's folder.
    QString For(const char* kind);

    // Record where the user actually went. Takes the FILE they picked and
    // stores its folder, because that is what the next dialog wants.
    void RememberFile(const char* kind, const QString& filePath);

    void RememberDir(const char* kind, const QString& dir);
}

#endif
