#include "scene/UiBinReader.hpp"
#include "scene/UiBinCommon.hpp"
#include "core/UiElement.hpp"
#include "core/Component.hpp"

#include <QFile>
#include <QVector>
#include <QUuid>
#include <QColor>
#include <QPointF>

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
        const int payloadEnd = payloadStart + int(payloadLen);

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
                    comp->setProperty("assetDomain", ctx.Str(a.domainId));
                    comp->setProperty("assetRegistryValue", ctx.Str(a.registryId));
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

    UiElement* ReadElement(Reader& r, const Ctx& ctx, UiElement* parent)
    {
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
            ReadElement(r, ctx, el);

        return el;
    }
}

UiElement* UiBinReader::Read(const QByteArray& bytes)
{
    if (bytes.size() < int(kHeaderSize) || std::memcmp(bytes.constData(), kMagic, 4) != 0)
        return nullptr;

    // Take a writable copy and demask everything after the header. The header
    // itself was left clear by the writer so we could locate sections and
    // validate magic/version before doing any work. See uibin::Obfuscate.
    QByteArray buf = bytes;
    Obfuscate(buf.data() + kHeaderSize, buf.size() - int(kHeaderSize));

    Reader r(buf.constData(), buf.size());

    r.seek(4);
    const quint16 version = r.U16();
    r.U16();                                 // flags
    const quint32 strOff   = r.U32();
    const quint32 strCount = r.U32();
    const quint32 assetOff  = r.U32();
    const quint32 assetCount= r.U32();
    const quint32 treeOff   = r.U32();
    const quint32 fileSize  = r.U32();

    if (version != kVersion || !r.ok())
        return nullptr;

    // The header's total-file-size field is a truncation sanity check (spec
    // section 3): a mismatch means the file is truncated or corrupt.
    if (qsizetype(fileSize) != bytes.size())
        return nullptr;

    Ctx ctx;

    // String table.
    r.seek(int(strOff));
    for (quint32 i = 0; i < strCount && r.ok(); ++i)
    {
        const quint32 len = r.U32();
        ctx.strings.push_back(QString::fromUtf8(r.Bytes(int(len))));
    }

    // Asset table.
    r.seek(int(assetOff));
    for (quint32 i = 0; i < assetCount && r.ok(); ++i)
    {
        AssetRec a;
        a.domainId   = r.U32();
        a.registryId = r.U32();
        const quint32 dl = r.U32();
        a.data = r.Bytes(int(dl));
        ctx.assets.push_back(a);
    }

    if (!r.ok())
        return nullptr;

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
