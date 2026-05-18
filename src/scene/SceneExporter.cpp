#include "scene/SceneExporter.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/UiBinWriter.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>

// ---------------------------------------------------------------------------
// Path collection: walk JSON tree, find non-empty string values whose key
// ends with "Path" (imagePath / fontPath / iconPath).
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
// Build mapping for ABSOLUTE leftover paths only: absolute -> "assets/name.ext"
// (relative paths are already project-root-relative and need no remapping).
// Handles duplicate filenames by appending _1, _2, ...
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
// Rewrite paths in JSON tree using the mapping (used only for absolute paths).
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
// Export to Folder: scene.json + assets/ subfolder.
//
// imagePath/fontPath/iconPath are stored relative to the project root. Each
// referenced file is resolved against the document's base directory and copied
// into folderPath at the SAME relative location, so scene.json can be written
// verbatim. Any stray absolute path is flattened into assets/ and rewritten.
// ---------------------------------------------------------------------------

bool SceneExporter::ExportToFolder(const SceneDocument* doc, const QString& folderPath)
{
    QByteArray jsonBytes = doc->ExportJson();

    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(jsonBytes, &err);
    if (err.error != QJsonParseError::NoError || !jdoc.isObject())
        return false;

    QJsonObject rootObj = jdoc.object();

    QSet<QString> allPaths;
    CollectAssetPaths(rootObj, allPaths);

    const QString srcBase = doc->GetBaseDir();
    QDir outDir(folderPath);

    // Absolute leftovers get flattened into assets/ and rewritten in the JSON.
    QSet<QString> absolutePaths;
    for (const QString& p : allPaths)
        if (QDir::isAbsolutePath(p))
            absolutePaths.insert(p);

    QMap<QString, QString> absMapping = BuildAssetMapping(absolutePaths);

    auto copyInto = [](const QString& src, const QString& dst) -> bool
    {
        if (src.isEmpty() || !QFile::exists(src))
            return false;

        // Skip copying a file onto itself (exporting into the project root).
        if (QFileInfo(src).canonicalFilePath() == QFileInfo(dst).canonicalFilePath()
            && !QFileInfo(src).canonicalFilePath().isEmpty())
            return true;

        QDir().mkpath(QFileInfo(dst).absolutePath());
        QFile::remove(dst);
        return QFile::copy(src, dst);
    };

    // Relative assets: keep their relative location under folderPath.
    for (const QString& p : allPaths)
    {
        if (QDir::isAbsolutePath(p))
            continue;

        const QString src = srcBase.isEmpty() ? p : QDir(srcBase).filePath(p);
        copyInto(src, outDir.filePath(p));
    }

    // Absolute assets: copy into assets/<name>.
    for (auto it = absMapping.begin(); it != absMapping.end(); ++it)
        copyInto(it.key(), outDir.filePath(it.value()));

    QJsonObject rewritten = absMapping.isEmpty() ? rootObj : RewritePaths(rootObj, absMapping);
    QByteArray outJson = QJsonDocument(rewritten).toJson(QJsonDocument::Indented);

    QDir().mkpath(folderPath);
    QFile outFile(outDir.filePath("scene.json"));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    outFile.write(outJson);
    outFile.close();

    return true;
}

// ---------------------------------------------------------------------------
// Bake to .uibin: fully custom binary container (see uibin_format_spec.txt).
// ---------------------------------------------------------------------------

bool SceneExporter::BakeToUiBin(const SceneDocument* doc, const QString& filePath)
{
    return UiBinWriter::Write(doc, filePath);
}
