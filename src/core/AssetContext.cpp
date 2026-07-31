#include "core/AssetContext.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QString>
#include <QStringList>

void AssetContext::SetBaseDir(const QString& dir)
{
    BaseDirRef() = dir;
}

QString AssetContext::BaseDir()
{
    return BaseDirRef();
}

bool AssetContext::HasBaseDir()
{
    return !BaseDirRef().isEmpty();
}

QString AssetContext::Resolve(const QString& rel)
{
    if (rel.isEmpty() || QDir::isAbsolutePath(rel) || BaseDirRef().isEmpty())
        return rel;

    return QDir(BaseDirRef()).filePath(rel);
}

bool AssetContext::IsValidRelative(const QString& v)
{
    if (v.isEmpty())
        return true;

    if (QDir::isAbsolutePath(v))
        return false;

    const QStringList parts = v.split(QLatin1Char('/'), Qt::SkipEmptyParts);

    return !parts.contains(QStringLiteral(".."));
}

QString AssetContext::ImportToAssets(const QString& srcAbs)
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

QString& AssetContext::BaseDirRef()
{
    static QString dir;
    return dir;
}

bool AssetContext::SameContents(const QString& a, const QString& b)
{
    QFile fa(a);
    QFile fb(b);

    if (!fa.open(QIODevice::ReadOnly) || !fb.open(QIODevice::ReadOnly))
        return false;

    if (fa.size() != fb.size())
        return false;

    return fa.readAll() == fb.readAll();
}
