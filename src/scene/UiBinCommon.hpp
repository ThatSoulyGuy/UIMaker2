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
    void Obfuscate(char* data, int n);

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
        QByteArray& buffer();
        int pos() const;

        void Raw(const char* p, int n);
        void U8(quint8 v);
        void U16(quint16 v);
        void U32(quint32 v);
        void I32(qint32 v);
        void U64(quint64 v);
        void I64(qint64 v);
        void F64(double v);

        // Patch a previously reserved u32 (for back-filled offsets/sizes).
        void PatchU32(int at, quint32 v);

    private:
        QByteArray buf;
    };

    // ----- Reader: cursor over a buffer, bounds-checked --------------------
    class Reader
    {
    public:
        Reader(const char* d, int n);

        bool ok() const;
        int  pos() const;
        void seek(int p);
        bool atEnd() const;

        quint8 U8();
        quint16 U16();
        quint32 U32();
        qint32 I32();
        quint64 U64();
        qint64 I64();
        double F64();

        QByteArray Bytes(int n);

    private:
        const char* data;
        int size;
        int cur = 0;
        bool bad = false;
    };
}

#endif
