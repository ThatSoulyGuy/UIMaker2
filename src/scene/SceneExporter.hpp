#ifndef SCENE_SCENEEXPORTER_HPP
#define SCENE_SCENEEXPORTER_HPP

#include <QByteArray>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QString>

class SceneDocument;

class SceneExporter
{
public:

    // Writes scene.json (paths kept relative to the project root) and copies
    // every referenced asset into folderPath, mirroring its relative location.
    static bool ExportToFolder(const SceneDocument* doc, const QString& folderPath);

    // Bakes the scene into the custom binary .uibin v2 container.
    static bool BakeToUiBin(const SceneDocument* doc, const QString& filePath);

private:

    static void CollectAssetPaths(const QJsonObject& elementObj, QSet<QString>& out);
    static QMap<QString, QString> BuildAssetMapping(const QSet<QString>& absolutePaths);
    static QJsonObject RewritePaths(const QJsonObject& elementObj, const QMap<QString, QString>& mapping);
};

#endif
