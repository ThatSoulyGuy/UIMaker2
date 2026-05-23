#ifndef SCENE_UIBINCOMMON_HPP
#define SCENE_UIBINCOMMON_HPP

#include <QByteArray>
#include <QString>
#include <cstring>

// ===========================================================================
//  .uibin v4 shared primitives
//
//  This is a deliberately opaque, fixed-layout binary container. It is NOT a
//  text/JSON-shaped format: every value is positioned by an explicit offset
//  table and decoded by hand. All scalars are little-endian (native on the
//  target platforms) so reading is a raw memcpy with no parsing.
//
//  Everything after the 32-byte header is also run through a cosmetic XOR
//  mask (see Obfuscate below) so a hex dump shows noise rather than readable
//  property/component/element names and recognisable PNG/TTF signatures.
//
//  Layout (see uibin_format_spec.txt for the authoritative description):
//
//    Header (32 bytes, fixed, NOT masked)
//    String table   : interned UTF-8, referenced everywhere by u32 id
//    Asset table    : (domainStrId, registryStrId, dataLen, raw bytes)
//    Element tree   : pre-order; components carry type-tagged TLV fields
// ===========================================================================

namespace uibin
{
    // Magic + version.
    static const char   kMagic[4] = { 'U', 'I', 'B', '4' };
    static const quint16 kVersion = 4;
    static const quint32 kHeaderSize = 32;

    // Cosmetic obfuscation. Every byte AFTER the 32-byte header is XORed with
    // a position-coupled LCG stream so a hex dump of a baked file looks like
    // noise (no readable component names, no PNG signatures, etc.). The header
    // itself stays clear so a loader can locate sections without demasking.
    //
    // This is NOT crypto - anyone with this source can decode the file. Its
    // sole purpose is to keep the bytes from being eyeball-readable. Writer
    // and reader call this same function over the same byte range; XOR is
    // self-inverse so one direction encodes and the other decodes.
    inline void Obfuscate(char* data, int n)
    {
        quint32 s = 0x5BC8A93Du;
        for (int i = 0; i < n; ++i)
        {
            s = s * 1103515245u + 12345u;
            data[i] = char(quint8(data[i]) ^ quint8(s >> 16));
        }
    }

    // No-asset sentinel for an ASSET_REF field.
    static const quint32 kNoAsset = 0xFFFFFFFFu;

    // Field type tags (1 byte) used inside component records.
    enum FieldTag : quint8
    {
        TAG_NONE      = 0,   // no payload
        TAG_BOOL      = 1,   // u8
        TAG_INT32     = 2,   // i32 LE
        TAG_INT64     = 3,   // i64 LE
        TAG_DOUBLE    = 4,   // f64 LE (IEEE-754)
        TAG_STRING    = 5,   // u32 string id
        TAG_COLOR     = 6,   // u32 0xAARRGGBB
        TAG_ASSET_REF = 7,   // u32 asset index (kNoAsset = none)
        TAG_POINT     = 8    // f64 x, f64 y
    };

    // ----- Writer: append-only, little-endian -----------------------------
    class Writer
    {
    public:
        QByteArray& buffer() { return buf; }
        int pos() const { return buf.size(); }

        void Raw(const char* p, int n) { buf.append(p, n); }
        void U8(quint8 v) { buf.append(char(v)); }

        void U16(quint16 v)
        {
            char b[2]; b[0]=char(v); b[1]=char(v>>8); buf.append(b,2);
        }

        void U32(quint32 v)
        {
            char b[4];
            b[0]=char(v); b[1]=char(v>>8); b[2]=char(v>>16); b[3]=char(v>>24);
            buf.append(b,4);
        }

        void I32(qint32 v) { U32(quint32(v)); }

        void U64(quint64 v)
        {
            char b[8];
            for (int i=0;i<8;++i) b[i]=char(v>>(8*i));
            buf.append(b,8);
        }

        void I64(qint64 v) { U64(quint64(v)); }

        void F64(double v)
        {
            quint64 q; std::memcpy(&q,&v,8); U64(q);
        }

        // Patch a previously reserved u32 (for back-filled offsets/sizes).
        void PatchU32(int at, quint32 v)
        {
            buf[at+0]=char(v); buf[at+1]=char(v>>8);
            buf[at+2]=char(v>>16); buf[at+3]=char(v>>24);
        }

    private:
        QByteArray buf;
    };

    // ----- Reader: cursor over a buffer, bounds-checked --------------------
    class Reader
    {
    public:
        Reader(const char* d, int n) : data(d), size(n) {}

        bool ok() const { return !bad; }
        int  pos() const { return cur; }
        void seek(int p) { if (p<0 || p>size) bad=true; else cur=p; }
        bool atEnd() const { return cur >= size; }

        quint8 U8()
        {
            if (cur+1>size){bad=true;return 0;}
            return quint8(data[cur++]);
        }

        quint16 U16()
        {
            if (cur+2>size){bad=true;return 0;}
            quint16 v = quint8(data[cur]) | (quint16(quint8(data[cur+1]))<<8);
            cur+=2; return v;
        }

        quint32 U32()
        {
            if (cur+4>size){bad=true;return 0;}
            quint32 v = quint8(data[cur])
                      | (quint32(quint8(data[cur+1]))<<8)
                      | (quint32(quint8(data[cur+2]))<<16)
                      | (quint32(quint8(data[cur+3]))<<24);
            cur+=4; return v;
        }

        qint32 I32() { return qint32(U32()); }

        quint64 U64()
        {
            if (cur+8>size){bad=true;return 0;}
            quint64 v=0;
            for (int i=0;i<8;++i) v |= quint64(quint8(data[cur+i]))<<(8*i);
            cur+=8; return v;
        }

        qint64 I64() { return qint64(U64()); }

        double F64()
        {
            quint64 q=U64(); double v; std::memcpy(&v,&q,8); return v;
        }

        QByteArray Bytes(int n)
        {
            if (n<0 || cur+n>size){bad=true;return QByteArray();}
            QByteArray b(data+cur, n); cur+=n; return b;
        }

    private:
        const char* data;
        int size;
        int cur = 0;
        bool bad = false;
    };
}

#endif
