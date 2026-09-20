#include "core/SpriteSidecar.hpp"

#include "core/AssetContext.hpp"

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QXmlStreamReader>

namespace
{
    struct Entry
    {
        SpriteSidecar::Meta meta;
        QDateTime stamp;      // sidecar mtime when parsed; null when absent
        qint64 checkedAtMs = 0;
    };

    QHash<QString, Entry>& Cache()
    {
        static QHash<QString, Entry> cache;
        return cache;
    }

    // Monotonic clock for the re-stat throttle. A stat is cheap but not free,
    // and this is reached once per element per refresh.
    qint64 NowMs()
    {
        static QElapsedTimer timer;

        if (!timer.isValid())
            timer.start();

        return timer.elapsed();
    }

    constexpr qint64 kRestatIntervalMs = 500;

    SpriteSidecar::Meta Parse(const QString& absPath)
    {
        SpriteSidecar::Meta meta;

        QFile f(absPath);

        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return meta;

        QXmlStreamReader xml(&f);

        while (!xml.atEnd() && !xml.hasError())
        {
            if (xml.readNext() != QXmlStreamReader::StartElement)
                continue;

            // Only <slice> is understood today. Anything else is skipped rather
            // than rejected, so a sidecar that later grows <pivot> or
            // <animation> still loads its slice in an older build.
            if (xml.name() != QLatin1String("slice"))
                continue;

            const QXmlStreamAttributes a = xml.attributes();

            auto readInset = [&a](QLatin1String name)
            {
                bool ok = false;
                const int v = a.value(name).toInt(&ok);

                return (ok && v > 0) ? v : 0;
            };

            meta.slice.left   = readInset(QLatin1String("left"));
            meta.slice.top    = readInset(QLatin1String("top"));
            meta.slice.right  = readInset(QLatin1String("right"));
            meta.slice.bottom = readInset(QLatin1String("bottom"));
            meta.hasSlice = !meta.slice.IsNull();
        }

        if (xml.hasError())
            return SpriteSidecar::Meta();

        return meta;
    }
}

QString SpriteSidecar::SidecarPathFor(const QString& imagePath)
{
    if (imagePath.isEmpty())
        return QString();

    const int dot = imagePath.lastIndexOf(QLatin1Char('.'));
    const int slash = imagePath.lastIndexOf(QLatin1Char('/'));

    // Only strip a real extension, not a dot in a directory name.
    if (dot > slash && dot >= 0)
        return imagePath.left(dot) + QStringLiteral(".xml");

    return imagePath + QStringLiteral(".xml");
}

SpriteSidecar::Meta SpriteSidecar::MetaFor(const QString& relPath)
{
    if (relPath.isEmpty())
        return Meta();

    const QString abs = AssetContext::Resolve(SidecarPathFor(relPath));

    if (abs.isEmpty())
        return Meta();

    const qint64 now = NowMs();

    auto it = Cache().find(abs);

    if (it != Cache().end() && now - it->checkedAtMs < kRestatIntervalMs)
        return it->meta;

    const QFileInfo info(abs);
    const QDateTime stamp = info.exists() ? info.lastModified() : QDateTime();

    if (it != Cache().end())
    {
        it->checkedAtMs = now;

        if (it->stamp == stamp)
            return it->meta;
    }

    Entry e;
    e.stamp = stamp;
    e.checkedAtMs = now;
    e.meta = stamp.isValid() ? Parse(abs) : Meta();

    Cache().insert(abs, e);

    return e.meta;
}

void SpriteSidecar::Invalidate()
{
    Cache().clear();
}
