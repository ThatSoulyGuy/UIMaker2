#include "scene/SceneExporter.hpp"
#include "scene/SceneDocument.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDataStream>

// ---------------------------------------------------------------------------
// Path collection: walk JSON tree, find non-empty string values whose key
// ends with "Path"
// ---------------------------------------------------------------------------

void SceneExporter::CollectAssetPaths(const QJsonObject& elementObj, QSet<QString>& out)
{
    for (const QJsonValue& compVal : elementObj["components"].toArray())
    {
        QJsonObject comp = compVal.toObject();

        for (auto it = comp.begin(); it != comp.end(); ++it)
        {
            if (it.key().endsWith("Path") && it.value().isString())
            {
                QString path = it.value().toString();
                if (!path.isEmpty())
                    out.insert(path);
            }
        }
    }

    for (const QJsonValue& childVal : elementObj["children"].toArray())
        CollectAssetPaths(childVal.toObject(), out);
}

// ---------------------------------------------------------------------------
// Build mapping: absolute path -> "assets/filename.ext"
// Handles duplicate filenames by appending _1, _2, etc.
// ---------------------------------------------------------------------------

QMap<QString, QString> SceneExporter::BuildAssetMapping(const QSet<QString>& absolutePaths)
{
    QMap<QString, QString> mapping;
    QSet<QString> usedNames;

    for (const QString& absPath : absolutePaths)
    {
        QFileInfo fi(absPath);
        QString baseName = fi.completeBaseName();
        QString suffix = fi.suffix();
        QString candidate = fi.fileName();

        int counter = 1;
        while (usedNames.contains(candidate))
        {
            candidate = baseName + "_" + QString::number(counter) + (suffix.isEmpty() ? "" : "." + suffix);
            counter++;
        }

        usedNames.insert(candidate);
        mapping[absPath] = "assets/" + candidate;
    }

    return mapping;
}

// ---------------------------------------------------------------------------
// Rewrite paths in JSON tree using the mapping
// ---------------------------------------------------------------------------

QJsonObject SceneExporter::RewritePaths(const QJsonObject& elementObj, const QMap<QString, QString>& mapping)
{
    QJsonObject result = elementObj;

    QJsonArray newComps;
    for (const QJsonValue& compVal : result["components"].toArray())
    {
        QJsonObject comp = compVal.toObject();

        for (auto it = comp.begin(); it != comp.end(); ++it)
        {
            if (it.key().endsWith("Path") && it.value().isString())
            {
                QString oldPath = it.value().toString();
                if (mapping.contains(oldPath))
                    comp[it.key()] = mapping[oldPath];
            }
        }

        newComps.append(comp);
    }
    result["components"] = newComps;

    QJsonArray newChildren;
    for (const QJsonValue& childVal : result["children"].toArray())
        newChildren.append(RewritePaths(childVal.toObject(), mapping));

    if (!newChildren.isEmpty())
        result["children"] = newChildren;

    return result;
}

// ---------------------------------------------------------------------------
// Export to Folder: scene.json + assets/ subfolder
// ---------------------------------------------------------------------------

bool SceneExporter::ExportToFolder(const SceneDocument* doc, const QString& folderPath)
{
    QByteArray jsonBytes = doc->ExportJson();

    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(jsonBytes, &err);
    if (err.error != QJsonParseError::NoError || !jdoc.isObject())
        return false;

    QJsonObject rootObj = jdoc.object();

    // Collect all referenced asset paths
    QSet<QString> assetPaths;
    CollectAssetPaths(rootObj, assetPaths);

    // Build mapping and create assets directory
    QMap<QString, QString> mapping = BuildAssetMapping(assetPaths);

    QDir dir(folderPath);
    if (!assetPaths.isEmpty())
        dir.mkpath("assets");

    // Copy asset files
    for (auto it = mapping.begin(); it != mapping.end(); ++it)
    {
        QString srcPath = it.key();
        QString dstPath = dir.filePath(it.value());

        if (QFile::exists(srcPath))
        {
            QFile::remove(dstPath);
            QFile::copy(srcPath, dstPath);
        }
    }

    // Rewrite JSON with relative paths
    QJsonObject rewritten = RewritePaths(rootObj, mapping);
    QJsonDocument outDoc(rewritten);
    QByteArray outJson = outDoc.toJson(QJsonDocument::Indented);

    // Write scene.json
    QFile outFile(dir.filePath("scene.json"));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    outFile.write(outJson);
    outFile.close();

    return true;
}

// ---------------------------------------------------------------------------
// Bake to .uibin: binary format with embedded assets
//
// Format (big-endian):
//   [4 bytes]  Magic: "UIBN"
//   [uint32]   Version: 1
//   [uint32]   JSON data length
//   [uint32]   Asset count
//   [N bytes]  JSON data (UTF-8, compact)
//   Per asset:
//     [uint32]   Key string length (bytes)
//     [N bytes]  Key string (UTF-8)
//     [uint32]   Data length (bytes)
//     [N bytes]  Raw file data
// ---------------------------------------------------------------------------

bool SceneExporter::BakeToUiBin(const SceneDocument* doc, const QString& filePath)
{
    QByteArray jsonBytes = doc->ExportJson();

    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(jsonBytes, &err);
    if (err.error != QJsonParseError::NoError || !jdoc.isObject())
        return false;

    QJsonObject rootObj = jdoc.object();

    // Collect and map asset paths
    QSet<QString> assetPaths;
    CollectAssetPaths(rootObj, assetPaths);
    QMap<QString, QString> mapping = BuildAssetMapping(assetPaths);

    // Read asset files into memory
    QMap<QString, QByteArray> assets;
    for (auto it = mapping.begin(); it != mapping.end(); ++it)
    {
        QFile f(it.key());
        if (f.open(QIODevice::ReadOnly))
        {
            assets[it.value()] = f.readAll();
            f.close();
        }
    }

    // Rewrite JSON with relative keys
    QJsonObject rewritten = RewritePaths(rootObj, mapping);
    QByteArray compactJson = QJsonDocument(rewritten).toJson(QJsonDocument::Compact);

    // Write binary
    QFile outFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QDataStream stream(&outFile);
    stream.setByteOrder(QDataStream::BigEndian);

    // Magic
    stream.writeRawData("UIBN", 4);

    // Version
    stream << static_cast<quint32>(1);

    // JSON length and asset count
    stream << static_cast<quint32>(compactJson.size());
    stream << static_cast<quint32>(assets.size());

    // JSON data
    stream.writeRawData(compactJson.constData(), compactJson.size());

    // Asset table
    for (auto it = assets.begin(); it != assets.end(); ++it)
    {
        QByteArray keyUtf8 = it.key().toUtf8();
        stream << static_cast<quint32>(keyUtf8.size());
        stream.writeRawData(keyUtf8.constData(), keyUtf8.size());

        stream << static_cast<quint32>(it.value().size());
        stream.writeRawData(it.value().constData(), it.value().size());
    }

    outFile.close();
    return true;
}

// ---------------------------------------------------------------------------
// Resolve relative paths in JSON against a base directory.
// Used when loading a folder-exported scene.json.
// ---------------------------------------------------------------------------

QByteArray SceneExporter::ResolveJsonPaths(const QByteArray& json, const QString& baseDir)
{
    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !jdoc.isObject())
        return json;

    QJsonObject rootObj = jdoc.object();

    // Collect all path values that are relative
    QSet<QString> relativePaths;
    CollectAssetPaths(rootObj, relativePaths);

    // Build mapping: relative -> absolute
    QMap<QString, QString> mapping;
    for (const QString& p : relativePaths)
    {
        if (!QDir::isAbsolutePath(p))
            mapping[p] = QDir(baseDir).filePath(p);
    }

    if (mapping.isEmpty())
        return json;

    QJsonObject resolved = RewritePaths(rootObj, mapping);
    return QJsonDocument(resolved).toJson(QJsonDocument::Indented);
}
