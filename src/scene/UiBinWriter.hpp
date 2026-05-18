#ifndef SCENE_UIBINWRITER_HPP
#define SCENE_UIBINWRITER_HPP

#include <QString>

class SceneDocument;

// Bakes a SceneDocument into the custom binary .uibin v2 container.
//
// imagePath / fontPath / iconPath are NOT encoded. Each is resolved against
// the document's project root, the file's raw bytes are embedded once in the
// asset table, and the component instead carries an ASSET_REF index plus the
// engine-facing (domain, registryValue) identity stored on the asset record.
class UiBinWriter
{
public:

    static bool Write(const SceneDocument* doc, const QString& filePath);
};

#endif
