#include "scene/UiBinCommon.hpp"

#include <cstring>

namespace uibin
{
    void Obfuscate(char* data, int n)
    {
        quint32 s = 0x5BC8A93Du;
        for (int i = 0; i < n; ++i)
        {
            s = s * 1103515245u + 12345u;
            data[i] = char(quint8(data[i]) ^ quint8(s >> 16));
        }
    }

    QByteArray& Writer::buffer() { return buf; }
    int Writer::pos() const { return buf.size(); }

    void Writer::Raw(const char* p, int n) { buf.append(p, n); }
    void Writer::U8(quint8 v) { buf.append(char(v)); }

    void Writer::U16(quint16 v)
    {
        char b[2]; b[0]=char(v); b[1]=char(v>>8); buf.append(b,2);
    }

    void Writer::U32(quint32 v)
    {
        char b[4];
        b[0]=char(v); b[1]=char(v>>8); b[2]=char(v>>16); b[3]=char(v>>24);
        buf.append(b,4);
    }

    void Writer::I32(qint32 v) { U32(quint32(v)); }

    void Writer::U64(quint64 v)
    {
        char b[8];
        for (int i=0;i<8;++i) b[i]=char(v>>(8*i));
        buf.append(b,8);
    }

    void Writer::I64(qint64 v) { U64(quint64(v)); }

    void Writer::F64(double v)
    {
        quint64 q; std::memcpy(&q,&v,8); U64(q);
    }

    void Writer::PatchU32(int at, quint32 v)
    {
        buf[at+0]=char(v); buf[at+1]=char(v>>8);
        buf[at+2]=char(v>>16); buf[at+3]=char(v>>24);
    }

    Reader::Reader(const char* d, int n) : data(d), size(n) {}

    bool Reader::ok() const { return !bad; }
    int  Reader::pos() const { return cur; }
    void Reader::seek(int p) { if (p<0 || p>size) bad=true; else cur=p; }
    bool Reader::atEnd() const { return cur >= size; }

    quint8 Reader::U8()
    {
        if (cur+1>size){bad=true;return 0;}
        return quint8(data[cur++]);
    }

    quint16 Reader::U16()
    {
        if (cur+2>size){bad=true;return 0;}
        quint16 v = quint8(data[cur]) | (quint16(quint8(data[cur+1]))<<8);
        cur+=2; return v;
    }

    quint32 Reader::U32()
    {
        if (cur+4>size){bad=true;return 0;}
        quint32 v = quint8(data[cur])
                  | (quint32(quint8(data[cur+1]))<<8)
                  | (quint32(quint8(data[cur+2]))<<16)
                  | (quint32(quint8(data[cur+3]))<<24);
        cur+=4; return v;
    }

    qint32 Reader::I32() { return qint32(U32()); }

    quint64 Reader::U64()
    {
        if (cur+8>size){bad=true;return 0;}
        quint64 v=0;
        for (int i=0;i<8;++i) v |= quint64(quint8(data[cur+i]))<<(8*i);
        cur+=8; return v;
    }

    qint64 Reader::I64() { return qint64(U64()); }

    double Reader::F64()
    {
        quint64 q=U64(); double v; std::memcpy(&v,&q,8); return v;
    }

    QByteArray Reader::Bytes(int n)
    {
        if (n<0 || cur+n>size){bad=true;return QByteArray();}
        QByteArray b(data+cur, n); cur+=n; return b;
    }
}
