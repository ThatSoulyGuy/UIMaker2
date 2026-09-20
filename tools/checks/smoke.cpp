// Smoke checks for the two changes that are hard to reach by clicking:
// the SceneDocument teardown order, and the UiBinReader bounds hardening.
#include "scene/SceneDocument.hpp"
#include "scene/UiBinReader.hpp"
#include "scene/UiBinCommon.hpp"
#include "scene/SceneExporter.hpp"
#include "scene/SceneExporter.hpp"
#include "core/UiElement.hpp"

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <cstdio>
#include <cstring>

static int failures = 0;
static void check(bool ok, const char* what)
{
    std::fprintf(stderr, "  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++failures;
}

// Build a v4 header whose string-table length field is hostile.
static QByteArray CraftedUiBin(quint32 strLen)
{
    QByteArray body;
    {
        QDataStream s(&body, QIODevice::WriteOnly);
        s.setByteOrder(QDataStream::LittleEndian);
        s << quint32(strLen);      // first string's length -> attacker controlled
    }

    QByteArray out;
    out.append("UIB4", 4);
    QDataStream h(&out, QIODevice::WriteOnly | QIODevice::Append);
    h.setByteOrder(QDataStream::LittleEndian);
    h << quint16(4) << quint16(0);              // version, flags
    h << quint32(32) << quint32(1);             // strOff, strCount
    h << quint32(32 + body.size()) << quint32(0);  // assetOff, assetCount
    h << quint32(32 + body.size());             // treeOff
    h << quint32(32 + body.size());             // fileSize

    while (out.size() < 32) out.append('\0');

    QByteArray masked = body;
    uibin::Obfuscate(masked.data(), masked.size());
    out.append(masked);

    // fileSize must match what is on disk or the reader bails before we get there
    const quint32 total = quint32(out.size());
    std::memcpy(out.data() + 28, &total, 4);

    return out;
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    std::fprintf(stderr, "SceneDocument teardown order\n");
    {
        auto* doc = new SceneDocument();
        UiElement* panel = doc->CreatePanelElement("Panel", nullptr);
        doc->CreateTextElement("Text", panel);
        doc->CreateButtonElement("Button", panel);
        doc->CreateImageElement("Img", nullptr);
        // Must not use-after-free: items are destroyed before the element tree.
        delete doc;
        check(true, "destroying a populated SceneDocument does not crash");
    }

    std::fprintf(stderr, "UiBinReader bounds hardening\n");
    {
        // ~2^31: `cur + n > size` used to overflow negative and pass the check.
        UiElement* r1 = UiBinReader::Read(CraftedUiBin(0x7FFFFFF0u));
        check(r1 == nullptr, "huge string length is rejected, not read out of bounds");
        delete r1;

        UiElement* r2 = UiBinReader::Read(CraftedUiBin(0xFFFFFFF0u));
        check(r2 == nullptr, "length with the high bit set is rejected");
        delete r2;

        UiElement* r3 = UiBinReader::Read(QByteArray("UIB4", 4));
        check(r3 == nullptr, "truncated file is rejected");
        delete r3;
    }

    std::fprintf(stderr, "uibin bake round-trip (the repo's only existing check)\n");
    {
        SceneDocument doc;
        UiElement* panel = doc.CreatePanelElement("Panel", nullptr);
        doc.CreateTextElement("Title", panel);
        doc.CreateButtonElement("Start", panel);

        const QString out = QStringLiteral("roundtrip.uibin");
        const bool baked = SceneExporter::BakeToUiBin(&doc, out);
        check(baked, "BakeToUiBin succeeds (writes .tmp, validates, renames)");

        QString err;
        check(UiBinReader::Validate(out, &err), "the baked file re-reads cleanly");
        if (!err.isEmpty()) std::fprintf(stderr, "      error: %s\n", qUtf8Printable(err));
    }

    std::fprintf(stderr, "%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASS",
                failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
