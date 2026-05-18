#include "scene/UiBinWriter.hpp"
#include "scene/UiBinCommon.hpp"
#include "scene/SceneDocument.hpp"
#include "core/UiElement.hpp"
#include "core/Component.hpp"
#include "core/AssetContext.hpp"

#include <QFile>
#include <QHash>
#include <QVector>
#include <QUuid>
#include <QColor>
#include <QPoint>
#include <QPointF>
#include <QMetaObject>
#include <QMetaProperty>
#include <QFileInfo>

using namespace uibin;

namespace
{
    // Accumulates the three logical sections during a single tree walk.
    struct Bake
    {
        QHash<QString, quint32> stringIndex;
        QVector<QString>        strings;

        struct Asset { quint32 domainId; quint32 registryId; QByteArray data; };
        QHash<QString, quint32> assetIndex;
        QVector<Asset>          assets;

        QString baseDir;

        quint32 Intern(const QString& s)
        {
            auto it = stringIndex.find(s);
            if (it != stringIndex.end())
                return it.value();

            const quint32 id = quint32(strings.size());
            strings.push_back(s);
            stringIndex.insert(s, id);
            return id;
        }

        // Resolve a relative asset path, embed its bytes once, and return the
        // asset index. Domain/registry come from sibling properties on the
        // same component and define the engine-facing identity.
        quint32 RegisterAsset(const QString& rel, const QString& domain, const QString& registry)
        {
            if (rel.isEmpty() && domain.isEmpty() && registry.isEmpty())
                return kNoAsset;

            const QString key = domain + QChar(0x1F) + registry + QChar(0x1F) + rel;

            auto it = assetIndex.find(key);
            if (it != assetIndex.end())
                return it.value();

            QByteArray data;
            if (!rel.isEmpty())
            {
                const QString abs = baseDir.isEmpty()
                    ? rel
                    : (QFileInfo(rel).isAbsolute() ? rel : baseDir + QLatin1Char('/') + rel);

                QFile f(abs);
                if (f.open(QIODevice::ReadOnly))
                {
                    data = f.readAll();
                    f.close();
                }
            }

            Asset a;
            a.domainId   = Intern(domain);
            a.registryId = Intern(registry);
            a.data       = data;

            const quint32 idx = quint32(assets.size());
            assets.push_back(a);
            assetIndex.insert(key, idx);
            return idx;
        }
    };

    void WriteComponent(Bake& bake, Writer& w, const Component* comp)
    {
        const QMetaObject* mo = comp->metaObject();

        w.U32(bake.Intern(comp->GetTypeName()));

        const int lenAt = w.pos();
        w.U32(0); // payload length, patched below

        const int countAt = w.pos();
        w.U16(0); // field count, patched below
        quint16 fieldCount = 0;

        const QString domain   = comp->property("assetDomain").toString();
        const QString registry = comp->property("assetRegistryValue").toString();

        for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
        {
            const QMetaProperty p = mo->property(i);
            const QString name = QString::fromLatin1(p.name());

            // The engine identity is folded into the asset record, not emitted
            // as plain fields.
            if (name == QLatin1String("assetDomain") || name == QLatin1String("assetRegistryValue"))
                continue;

            w.U32(bake.Intern(name));

            if (name.endsWith(QLatin1String("Path")))
            {
                w.U8(TAG_ASSET_REF);
                w.U32(bake.RegisterAsset(comp->property(p.name()).toString(), domain, registry));
                ++fieldCount;
                continue;
            }

            const QVariant v = comp->property(p.name());

            if (p.isEnumType())
            {
                w.U8(TAG_INT32);
                w.I32(v.toInt());
            }
            else
            {
                switch (p.metaType().id())
                {
                case QMetaType::Bool:
                    w.U8(TAG_BOOL);  w.U8(v.toBool() ? 1 : 0); break;
                case QMetaType::Int:
                case QMetaType::UInt:
                case QMetaType::Short:
                case QMetaType::UShort:
                case QMetaType::Char:
                case QMetaType::UChar:
                    w.U8(TAG_INT32); w.I32(v.toInt()); break;
                case QMetaType::LongLong:
                case QMetaType::ULongLong:
                case QMetaType::Long:
                case QMetaType::ULong:
                    w.U8(TAG_INT64); w.I64(v.toLongLong()); break;
                case QMetaType::Double:
                case QMetaType::Float:
                    w.U8(TAG_DOUBLE); w.F64(v.toDouble()); break;
                case QMetaType::QColor:
                    w.U8(TAG_COLOR);
                    w.U32(quint32(v.value<QColor>().rgba())); // 0xAARRGGBB
                    break;
                case QMetaType::QPointF:
                {
                    const QPointF pt = v.toPointF();
                    w.U8(TAG_POINT); w.F64(pt.x()); w.F64(pt.y());
                    break;
                }
                case QMetaType::QPoint:
                {
                    const QPoint pt = v.toPoint();
                    w.U8(TAG_POINT); w.F64(pt.x()); w.F64(pt.y());
                    break;
                }
                case QMetaType::QString:
                    w.U8(TAG_STRING); w.U32(bake.Intern(v.toString())); break;
                default:
                    if (v.canConvert<QString>())
                    {
                        w.U8(TAG_STRING); w.U32(bake.Intern(v.toString()));
                    }
                    else
                    {
                        w.U8(TAG_NONE);
                    }
                    break;
                }
            }

            ++fieldCount;
        }

        // Back-fill count and payload length (everything after the length slot).
        w.buffer()[countAt + 0] = char(fieldCount);
        w.buffer()[countAt + 1] = char(fieldCount >> 8);
        w.PatchU32(lenAt, quint32(w.pos() - (lenAt + 4)));
    }

    void WriteElement(Bake& bake, Writer& w, const UiElement* el)
    {
        w.U32(bake.Intern(el->GetName()));

        QByteArray uuid = el->GetId().toRfc4122(); // exactly 16 bytes
        uuid.resize(16);
        w.Raw(uuid.constData(), 16);

        const std::vector<Component*> comps = el->GetComponents();
        w.U16(quint16(comps.size()));
        for (const Component* c : comps)
            WriteComponent(bake, w, c);

        QVector<UiElement*> kids;
        for (QObject* o : el->children())
            if (auto* ce = qobject_cast<UiElement*>(o))
                kids.push_back(ce);

        w.U32(quint32(kids.size()));
        for (const UiElement* ce : kids)
            WriteElement(bake, w, ce);
    }
}

bool UiBinWriter::Write(const SceneDocument* doc, const QString& filePath)
{
    if (!doc || !doc->GetRoot())
        return false;

    Bake bake;
    bake.baseDir = doc->GetBaseDir();
    bake.Intern(QString()); // id 0 == empty string, by contract

    Writer tree;
    WriteElement(bake, tree, doc->GetRoot());

    // --- String table -----------------------------------------------------
    Writer strW;
    for (const QString& s : bake.strings)
    {
        const QByteArray u = s.toUtf8();
        strW.U32(quint32(u.size()));
        strW.Raw(u.constData(), u.size());
    }

    // --- Asset table ------------------------------------------------------
    Writer assetW;
    for (const Bake::Asset& a : bake.assets)
    {
        assetW.U32(a.domainId);
        assetW.U32(a.registryId);
        assetW.U32(quint32(a.data.size()));
        assetW.Raw(a.data.constData(), a.data.size());
    }

    const quint32 strOff   = kHeaderSize;
    const quint32 assetOff  = strOff + quint32(strW.buffer().size());
    const quint32 treeOff   = assetOff + quint32(assetW.buffer().size());
    const quint32 fileSize  = treeOff + quint32(tree.buffer().size());

    Writer hdr;
    hdr.Raw(kMagic, 4);
    hdr.U16(kVersion);
    hdr.U16(0);                              // flags
    hdr.U32(strOff);
    hdr.U32(quint32(bake.strings.size()));
    hdr.U32(assetOff);
    hdr.U32(quint32(bake.assets.size()));
    hdr.U32(treeOff);
    hdr.U32(fileSize);

    QFile out(filePath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    out.write(hdr.buffer());
    out.write(strW.buffer());
    out.write(assetW.buffer());
    out.write(tree.buffer());
    out.close();

    return true;
}
