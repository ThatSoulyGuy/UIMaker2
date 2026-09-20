#include "scene/UiBinReader.hpp"
#include "scene/UiBinCommon.hpp"
#include "core/UiElement.hpp"
#include "core/Component.hpp"

#include <QFile>
#include <QVector>
#include <QUuid>
#include <QColor>
#include <QPointF>

#include <limits>

using namespace uibin;

namespace
{
    struct AssetRec { quint32 domainId; quint32 registryId; QByteArray data; };

    struct Ctx
    {
        QVector<QString>  strings;
        QVector<AssetRec> assets;

        QString Str(quint32 id) const
        {
            return id < quint32(strings.size()) ? strings[int(id)] : QString();
        }
    };

    void ReadComponent(Reader& r, const Ctx& ctx, UiElement* el)
    {
        const QString typeName = ctx.Str(r.U32());
        const quint32 payloadLen = r.U32();
        const int payloadStart = r.pos();

        // payloadLen is attacker-controlled: compute the end in 64-bit and reject
        // anything that cannot be a position in this buffer, rather than letting
        // the addition wrap and hand seek() a bogus-but-in-range cursor.
        const qint64 payloadEnd64 = qint64(payloadStart) + qint64(payloadLen);

        if (payloadEnd64 > qint64(std::numeric_limits<int>::max()))
        {
            r.seek(-1);   // forces the reader into its error state
            return;
        }

        const int payloadEnd = int(payloadEnd64);

        Component* comp = Component::Create(typeName, el);

        const quint16 fieldCount = r.U16();

        for (quint16 f = 0; f < fieldCount && r.ok(); ++f)
        {
            const QString name = ctx.Str(r.U32());
            const quint8 tag = r.U8();

            QVariant value;
            quint32 assetRef = kNoAsset;
            bool unknownTag = false;

            switch (tag)
            {
            case TAG_NONE:                                   break;
            case TAG_BOOL:   value = bool(r.U8());            break;
            case TAG_INT32:  value = int(r.I32());            break;
            case TAG_INT64:  value = qlonglong(r.I64());      break;
            case TAG_DOUBLE: value = r.F64();                 break;
            case TAG_STRING: value = ctx.Str(r.U32());        break;
            case TAG_COLOR:  value = QColor::fromRgba(QRgb(r.U32())); break;
            case TAG_POINT:  { const double x = r.F64(); const double y = r.F64(); value = QPointF(x, y); break; }
            case TAG_ASSET_REF: assetRef = r.U32();           break;
            default: unknownTag = true;                       break;
            }

            // Unknown tag: cannot know its width — abandon this component
            // and resync via the payload length (spec section 8).
            if (unknownTag)
            {
                r.seek(payloadEnd);
                break;
            }

            if (!comp)
                continue;

            if (tag == TAG_ASSET_REF)
            {
                if (assetRef != kNoAsset && assetRef < quint32(ctx.assets.size()))
                {
                    const AssetRec& a = ctx.assets[int(assetRef)];

                    // Identity is per asset SLOT: a component with two
                    // "...Path" properties keeps them apart. For "xxxPath" try
                    // "xxxDomain"/"xxxRegistryValue" first, and fall back to the
                    // component-wide pair. Mirrors UiBinWriter.
                    const QString stem = name.left(name.size() - 4);
                    const QByteArray slotDomain = (stem + "Domain").toLatin1();
                    const QByteArray slotRegistry = (stem + "RegistryValue").toLatin1();

                    const bool perSlot = comp->property(slotDomain.constData()).isValid();

                    comp->setProperty(perSlot ? slotDomain.constData() : "assetDomain",
                                      ctx.Str(a.domainId));
                    comp->setProperty(perSlot ? slotRegistry.constData() : "assetRegistryValue",
                                      ctx.Str(a.registryId));
                }
                // The path string itself is deliberately not restored.
            }
            else if (value.isValid())
            {
                comp->setProperty(name.toLatin1().constData(), value);
            }
        }

        // Always resync to the declared end of the record so a single bad or
        // unknown component cannot derail the rest of the tree.
        r.seek(payloadEnd);
    }

    // A child record costs only 26 bytes on disk, so an uncapped recursion lets a
    // few-MB file blow the stack. No authored UI nests anywhere near this deep.
    constexpr int kMaxElementDepth = 64;

    UiElement* ReadElement(Reader& r, const Ctx& ctx, UiElement* parent, int depth = 0)
    {
        if (depth > kMaxElementDepth)
        {
            r.seek(-1);   // forces the reader into its error state
            return nullptr;
        }

        const QString name = ctx.Str(r.U32());
        const QByteArray uuid = r.Bytes(16);

        if (!r.ok())
            return nullptr;

        auto* el = new UiElement(name, parent);
        el->SetId(QUuid::fromRfc4122(uuid));

        const quint16 compCount = r.U16();
        for (quint16 c = 0; c < compCount && r.ok(); ++c)
            ReadComponent(r, ctx, el);

        const quint32 childCount = r.U32();
        for (quint32 i = 0; i < childCount && r.ok(); ++i)
            ReadElement(r, ctx, el, depth + 1);

        return el;
    }
}

UiElement* UiBinReader::Read(const QByteArray& bytes)
{
    if (bytes.size() < int(kHeaderSize) || std::memcmp(bytes.constData(), kMagic, 4) != 0)
        return nullptr;

    // Spec R3.5: validate the version BEFORE demasking. The header is written
    // in the clear precisely so a foreign or future file can be rejected
    // without paying for a full copy and a full XOR pass over the body.
    {
        Reader pre(bytes.constData(), int(bytes.size()));
        pre.seek(4);

        if (pre.U16() != kVersion || !pre.ok())
            return nullptr;
    }

    QByteArray buf = bytes;
    Obfuscate(buf.data() + kHeaderSize, buf.size() - int(kHeaderSize));

    Reader r(buf.constData(), buf.size());

    r.seek(4);
    r.U16();                                 // version, already validated
    r.U16();                                 // flags: spec R3.6 - ignore every bit
    const quint32 strOff   = r.U32();
    const quint32 strCount = r.U32();
    const quint32 assetOff  = r.U32();
    const quint32 assetCount= r.U32();
    const quint32 treeOff   = r.U32();
    const quint32 fileSize  = r.U32();

    if (!r.ok())
        return nullptr;

    // Spec R3.7: the declared size must match reality, which catches truncation.
    if (qsizetype(fileSize) != bytes.size())
        return nullptr;

    // Spec R3.8: the sections must start where the format says and must not
    // overlap or run backwards.
    if (strOff != kHeaderSize
        || assetOff < strOff
        || treeOff < assetOff
        || qsizetype(treeOff) > bytes.size())
    {
        return nullptr;
    }

    Ctx ctx;

    // String table. Bounded to [strOff, assetOff) per spec R17.1: a string
    // length that overruns into the asset table is corrupt, even though it
    // still lies inside the file.
    Reader strReader(buf.constData(), int(assetOff));
    strReader.seek(int(strOff));

    for (quint32 i = 0; i < strCount && strReader.ok(); ++i)
    {
        const quint32 len = strReader.U32();
        ctx.strings.push_back(QString::fromUtf8(strReader.Bytes(int(len))));
    }

    if (!strReader.ok())
        return nullptr;

    // Asset table, bounded to [assetOff, treeOff).
    Reader assetReader(buf.constData(), int(treeOff));
    assetReader.seek(int(assetOff));

    for (quint32 i = 0; i < assetCount && assetReader.ok(); ++i)
    {
        AssetRec a;
        a.domainId   = assetReader.U32();
        a.registryId = assetReader.U32();
        const quint32 dl = assetReader.U32();
        a.data = assetReader.Bytes(int(dl));
        ctx.assets.push_back(a);
    }

    if (!assetReader.ok())
        return nullptr;

    // Element tree: the remainder of the file.
    r.seek(int(treeOff));
    UiElement* root = ReadElement(r, ctx, nullptr);

    if (!r.ok())
    {
        delete root;
        return nullptr;
    }

    return root;
}

bool UiBinReader::Validate(const QString& filePath, QString* error)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error) *error = QStringLiteral("cannot open file");
        return false;
    }

    const QByteArray bytes = f.readAll();
    f.close();

    UiElement* root = Read(bytes);
    if (!root)
    {
        if (error) *error = QStringLiteral("structural decode failed");
        return false;
    }

    delete root;
    return true;
}
