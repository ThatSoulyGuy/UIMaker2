#ifndef SCENE_UIBINREADER_HPP
#define SCENE_UIBINREADER_HPP

#include <QString>
#include <QByteArray>

class UiElement;

// Decodes a .uibin v4 container back into a UiElement tree. Used for
// round-trip validation (the editor authors from JSON; this proves the binary
// container is self-consistent and re-readable).
//
// imagePath/fontPath/iconPath are intentionally not restored (they are not
// stored); instead the engine identity (assetDomain / assetRegistryValue) is
// reapplied from the referenced asset record.
class UiBinReader
{
public:

    // Returns a newly allocated root (caller owns) or nullptr on any
    // structural error / magic / version mismatch.
    static UiElement* Read(const QByteArray& bytes);

    // Convenience: parse a file and report whether it is a valid container.
    static bool Validate(const QString& filePath, QString* error = nullptr);
};

#endif
