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

    static bool ExportToFolder(const SceneDocument* doc, const QString& folderPath);
    static bool BakeToUiBin(const SceneDocument* doc, const QString& filePath);
    static QByteArray ResolveJsonPaths(const QByteArray& json, const QString& baseDir);

private:

    static void CollectAssetPaths(const QJsonObject& elementObj, QSet<QString>& out);
    static QMap<QString, QString> BuildAssetMapping(const QSet<QString>& absolutePaths);
    static QJsonObject RewritePaths(const QJsonObject& elementObj, const QMap<QString, QString>& mapping);
};

#endif
