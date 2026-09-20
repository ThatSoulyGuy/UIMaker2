// The wrap/crop/9-slice span geometry.
//
// This is the riskiest arithmetic in the pixel-perfect rendering model: every
// other part of it either reduces to these spans or merely calls them. Two
// independent designs of this algorithm disagreed about the Center phase (25 vs
// 75 for a 150-wide destination over a 100-texel period), which is exactly the
// kind of thing that is invisible on the reference art and wrong everywhere
// else, so the phase cases are asserted explicitly.

#include "core/PixelDraw.hpp"
#include "core/PixelModel.hpp"
#include "core/AssetContext.hpp"
#include "components/DocumentComponent.hpp"
#include "components/ImageComponent.hpp"
#include "components/TransformComponent.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneExporter.hpp"
#include "scene/UiBinReader.hpp"
#include "scene/UiBinCommon.hpp"
#include "components/ButtonComponent.hpp"
#include <QDataStream>
#include <cstring>
#include "core/UiElement.hpp"
#include <QTemporaryDir>
#include <QDir>

#include <QVector>
#include <QImage>
#include <QPainter>
#include <QPixmap>

#include <cstdio>

#include "checks.hpp"

using namespace PixelDraw;

namespace
{
    // Every span copies texels 1:1 - there is no scale factor in a Span - and
    // the spans must exactly tile the destination with no gap and no overlap.
    bool CoversExactly(const QVector<Span>& spans, int d)
    {
        QVector<int> hits(d, 0);

        for (const Span& s : spans)
        {
            if (s.len <= 0)
                return false;

            for (int i = 0; i < s.len; ++i)
            {
                const int at = s.dstStart + i;

                if (at < 0 || at >= d)
                    return false;

                ++hits[at];
            }
        }

        for (int i = 0; i < d; ++i)
        {
            if (hits[i] != 1)
                return false;
        }

        return true;
    }

    bool SourceInRange(const QVector<Span>& spans, int n)
    {
        for (const Span& s : spans)
        {
            if (s.srcStart < 0 || s.srcStart + s.len > n)
                return false;
        }

        return true;
    }

    // The source texel that ends up at destination column `at`.
    int SourceAt(const QVector<Span>& spans, int at)
    {
        for (const Span& s : spans)
        {
            if (at >= s.dstStart && at < s.dstStart + s.len)
                return s.srcStart + (at - s.dstStart);
        }

        return -1;
    }
}

void CheckPixelSpans()
{
    std::fprintf(stderr, "4. wrap / crop / 9-slice span geometry\n");

    // ---- unsliced, destination larger than the texture: it REPEATS --------
    {
        QVector<Span> v;
        check(SolveAxis(10, 0, 0, 25, SideNear, 0, v), "solves an unsliced axis");
        check(CoversExactly(v, 25), "the spans tile the destination exactly, no gaps or overlaps");
        check(SourceInRange(v, 10), "and never read outside the texture");

        // Near anchor: texel 0 at the near edge, and it repeats every 10.
        check(SourceAt(v, 0) == 0 && SourceAt(v, 10) == 0 && SourceAt(v, 20) == 0,
              "a wider destination REPEATS the texture rather than stretching it");
        check(SourceAt(v, 24) == 4, "with the last repeat cropped at the far edge");
    }

    // ---- unsliced, destination smaller: it CROPS, per anchor --------------
    {
        QVector<Span> nearV, farV, midV;
        SolveAxis(10, 0, 0, 4, SideNear, 0, nearV);
        SolveAxis(10, 0, 0, 4, SideFar, 0, farV);
        SolveAxis(10, 0, 0, 4, SideCenter, 0, midV);

        check(CoversExactly(nearV, 4) && CoversExactly(farV, 4) && CoversExactly(midV, 4),
              "a narrower destination crops cleanly for every anchor");

        check(SourceAt(nearV, 0) == 0, "near anchor keeps the texture's first texel");
        check(SourceAt(farV, 3) == 9, "far anchor keeps its last texel");
        check(SourceAt(midV, 0) == 3, "centre anchor takes the middle (10-4)/2 = 3");
    }

    // ---- the Center phase the two designs disagreed about -----------------
    {
        QVector<Span> v;
        SolveAxis(100, 0, 0, 150, SideCenter, 0, v);

        check(CoversExactly(v, 150), "a 150-wide window over a 100-texel period tiles exactly");

        // 150 over a period of 100 leaves a 50-texel shortfall; centred, 25 is
        // hidden at each end, so the first visible texel is source 25.
        check(SourceAt(v, 0) == 25, "centre phase starts 25 texels in, not 75");
        check(SourceAt(v, 74) == 99, "wrapping at the period boundary");
        check(SourceAt(v, 75) == 0, "and resuming at texel 0");
    }

    // ---- crop offset shifts the phase, and wraps -------------------------
    {
        QVector<Span> base, shifted, wrapped;
        SolveAxis(10, 0, 0, 20, SideNear, 0, base);
        SolveAxis(10, 0, 0, 20, SideNear, 3, shifted);
        SolveAxis(10, 0, 0, 20, SideNear, 13, wrapped);

        check(SourceAt(base, 0) == 0, "no crop offset starts at texel 0");
        check(SourceAt(shifted, 0) == 3, "a crop offset of 3 starts at texel 3");
        check(SourceAt(wrapped, 0) == 3, "an offset beyond the period wraps modulo it");

        QVector<Span> negative;
        SolveAxis(10, 0, 0, 20, SideNear, -1, negative);
        check(SourceAt(negative, 0) == 9, "and a negative offset wraps the other way");
    }

    // ---- the real asset: example_button.png 200x20, slice 3/3/3/4 ---------
    {
        QVector<Span> cols, rows;

        // Destination 400 x 40 virtual pixels: wider AND taller than the art.
        SolveAxis(200, 3, 3, 400, SideCenter, 0, cols);
        SolveAxis(20, 3, 4, 40, SideCenter, 0, rows);

        check(CoversExactly(cols, 400) && CoversExactly(rows, 40),
              "the example button's slice tiles a 400x40 destination exactly");
        check(SourceInRange(cols, 200) && SourceInRange(rows, 20),
              "reading only inside the 200x20 texture");

        // The sliced edges must NOT wrap in their pinned direction: the 3px
        // left inset is the texture's leftmost 3 texels, 1:1, and likewise the
        // 4px bottom inset is its bottom 4 rows.
        check(SourceAt(cols, 0) == 0 && SourceAt(cols, 1) == 1 && SourceAt(cols, 2) == 2,
              "the left slice is pinned: destination 0,1,2 are texels 0,1,2");
        check(SourceAt(cols, 399) == 199 && SourceAt(cols, 397) == 197,
              "the right slice is pinned to the texture's last 3 columns");
        check(SourceAt(rows, 0) == 0 && SourceAt(rows, 2) == 2,
              "the top slice is pinned to the first 3 rows");
        check(SourceAt(rows, 39) == 19 && SourceAt(rows, 36) == 16,
              "the bottom slice is pinned to the last 4 rows");

        // The interior between them repeats.
        const int interiorA = SourceAt(cols, 3);
        const int interiorB = SourceAt(cols, 3 + 194);
        check(interiorA >= 3 && interiorA < 197, "the interior samples the interior band");
        check(interiorA == interiorB, "and repeats with the interior's 194-texel period");
    }

    // ---- a slice that cannot fit degrades instead of blanking -------------
    {
        QVector<Span> v;
        check(SolveAxis(4, 3, 3, 12, SideNear, 0, v),
              "a 3+3 slice on a 4-texel axis still solves");
        check(CoversExactly(v, 12), "by falling back to plain wrapping rather than producing nothing");
        check(SourceAt(v, 0) == 0 && SourceAt(v, 4) == 0, "which repeats the whole 4 texels");
    }

    // ---- destination narrower than the two insets ------------------------
    {
        QVector<Span> v;
        check(SolveAxis(200, 3, 3, 4, SideNear, 0, v), "a destination narrower than the insets solves");
        check(CoversExactly(v, 4), "and still covers exactly");
        check(SourceAt(v, 0) == 0, "keeping the near corner");
        check(SourceAt(v, 3) == 199, "and the far corner");
    }

    // ---- degenerate input -------------------------------------------------
    {
        QVector<Span> v;
        check(!SolveAxis(0, 0, 0, 10, SideNear, 0, v), "a zero-width texture is refused");
        check(!SolveAxis(10, 0, 0, 0, SideNear, 0, v), "a zero-width destination is refused");
        check(v.isEmpty(), "and neither emits spans");
    }

    // ---- exhaustive: every combination must tile exactly ------------------
    {
        bool allExact = true;
        bool allInRange = true;

        const Side sides[3] = { SideNear, SideCenter, SideFar };

        for (int n = 1; n <= 24 && allExact; ++n)
        {
            for (int d = 1; d <= 40; ++d)
            {
                for (int si = 0; si < 3; ++si)
                {
                    for (int a = 0; a <= 4; ++a)
                    {
                        for (int b = 0; b <= 4; ++b)
                        {
                            for (int off = -3; off <= 3; ++off)
                            {
                                QVector<Span> v;

                                if (!SolveAxis(n, a, b, d, sides[si], off, v))
                                    continue;

                                if (!CoversExactly(v, d))
                                {
                                    allExact = false;
                                    std::fprintf(stderr,
                                        "      n=%d a=%d b=%d d=%d side=%d off=%d did not tile exactly\n",
                                        n, a, b, d, si, off);
                                    break;
                                }

                                if (!SourceInRange(v, n))
                                {
                                    allInRange = false;
                                    std::fprintf(stderr,
                                        "      n=%d a=%d b=%d d=%d side=%d off=%d read out of range\n",
                                        n, a, b, d, si, off);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        check(allExact, "exhaustive sweep: every span set tiles its destination exactly");
        check(allInRange, "exhaustive sweep: no span ever reads outside the texture");
    }

    // ---- SnapLength / SnapRect -------------------------------------------
    {
        PixelModel::SetMode(PixelModel::Mode::Continuous);
        check(qFuzzyCompare(SnapLength(37.4), 37.4), "lengths are untouched in Continuous mode");

        PixelModel::SetMode(PixelModel::Mode::PixelGrid);
        PixelModel::SetUnit(6.0);

        check(qFuzzyCompare(SnapLength(37.4), 36.0), "and snap to whole units in PixelGrid mode");
        check(qFuzzyCompare(SnapLength(0.0), 6.0), "a collapsed element keeps one whole virtual pixel");

        const QRectF r = SnapRect(QRectF(10.2, 20.9, 37.4, 11.1));
        check(qFuzzyCompare(r.left(), 12.0) && qFuzzyCompare(r.top(), 18.0),
              "rect origins land on the lattice");
        check(qFuzzyCompare(r.width(), 36.0) && qFuzzyCompare(r.height(), 12.0),
              "and so do their extents");

        PixelModel::SetMode(PixelModel::Mode::Continuous);
        PixelModel::SetUnit(1.0);
    }
}


// ---------------------------------------------------------------------------
// The RASTER, not just the geometry: render through DrawTexture and assert that
// every destination virtual pixel is a SOLID unit x unit block carrying exactly
// the texel the span solver says it should. Solid blocks prove nearest-neighbour
// with no resampling (requirement b); matching texels prove 1 texel == 1 virtual
// pixel (requirement c) and that wrapping honours the slice (requirements d, e).
// ---------------------------------------------------------------------------
void CheckPixelRaster()
{
    std::fprintf(stderr, "5. texel-exact raster\n");

    const int tw = 8, th = 8;
    const int unit = 5;
    const int dwVpx = 21, dhVpx = 17;   // deliberately not a multiple of anything

    // Each texel a unique colour, so a mis-sampled pixel is unambiguous.
    QImage texImg(tw, th, QImage::Format_ARGB32);
    for (int y = 0; y < th; ++y)
        for (int x = 0; x < tw; ++x)
            texImg.setPixel(x, y, qRgb(20 + x * 28, 20 + y * 28, 90));

    const QPixmap tex = QPixmap::fromImage(texImg);

    PixelModel::SetMode(PixelModel::Mode::PixelGrid);
    PixelModel::SetUnit(double(unit));

    Slice slice;
    slice.left = 2; slice.top = 2; slice.right = 2; slice.bottom = 2;

    QImage out(dwVpx * unit, dhVpx * unit, QImage::Format_ARGB32);
    out.fill(Qt::black);
    {
        QPainter p(&out);
        DrawTexture(&p, QRectF(0, 0, dwVpx * unit, dhVpx * unit), tex,
                    slice, Center, 0, 0, FillWrap);
    }

    // What the solver says each destination column/row should sample.
    QVector<Span> cols, rows;
    SolveAxis(tw, slice.left, slice.right, dwVpx, SideX(Center), 0, cols);
    SolveAxis(th, slice.top, slice.bottom, dhVpx, SideY(Center), 0, rows);

    auto srcAt = [](const QVector<Span>& spans, int at)
    {
        for (const Span& s : spans)
            if (at >= s.dstStart && at < s.dstStart + s.len)
                return s.srcStart + (at - s.dstStart);
        return -1;
    };

    bool allSolid = true;
    bool allMatch = true;
    int firstBadX = -1, firstBadY = -1;

    for (int dy = 0; dy < dhVpx && allMatch; ++dy)
    {
        for (int dx = 0; dx < dwVpx; ++dx)
        {
            const QRgb expected = texImg.pixel(srcAt(cols, dx), srcAt(rows, dy));
            const QRgb got = out.pixel(dx * unit, dy * unit);

            if (got != expected)
            {
                allMatch = false;
                firstBadX = dx; firstBadY = dy;
                break;
            }

            // Every pixel of the block must be identical - no interpolation.
            for (int py = 0; py < unit && allSolid; ++py)
                for (int px = 0; px < unit; ++px)
                    if (out.pixel(dx * unit + px, dy * unit + py) != expected)
                        { allSolid = false; break; }
        }
    }

    check(allMatch, "every virtual pixel samples exactly the texel the solver chose");
    if (!allMatch)
        std::fprintf(stderr, "      first mismatch at vpx (%d,%d)\n", firstBadX, firstBadY);

    check(allSolid, "and each is a SOLID unit x unit block - nearest-neighbour, never resampled");

    // The pinned edges really are pinned: destination column 0 is texel 0.
    check(srcAt(cols, 0) == 0 && srcAt(cols, 1) == 1, "left slice pinned in the raster");
    check(srcAt(cols, dwVpx - 1) == tw - 1, "right slice pinned in the raster");
    check(srcAt(rows, dhVpx - 1) == th - 1, "bottom slice pinned in the raster");

    // Stretch mode must still scale, so the legacy look is unchanged.
    {
        QImage st(dwVpx * unit, dhVpx * unit, QImage::Format_ARGB32);
        st.fill(Qt::black);
        {
            QPainter p(&st);
            DrawTexture(&p, QRectF(0, 0, st.width(), st.height()), tex,
                        slice, Center, 0, 0, FillStretch);
        }
        check(st.pixel(0, 0) == texImg.pixel(0, 0), "stretch still maps the source corner to the dest corner");
        check(st != out, "and produces a different raster from wrap");
    }

    PixelModel::SetMode(PixelModel::Mode::Continuous);
    PixelModel::SetUnit(1.0);
}


// ---------------------------------------------------------------------------
// A.1: the rendering model and the 9-slice insets must survive a BAKE, or an
// engine cannot reproduce wrap rendering from the file alone. Previously
// neither did: the model lived only in scene.json and the slice only in a
// sidecar .xml that is not embedded.
// ---------------------------------------------------------------------------
void CheckDocumentBake()
{
    std::fprintf(stderr, "6. rendering model and slice survive a bake\n");

    QTemporaryDir tmp;
    check(tmp.isValid(), "temp project root");
    AssetContext::SetBaseDir(tmp.path());

    // A tiny texture plus a sidecar describing its slice.
    {
        QImage art(16, 16, QImage::Format_ARGB32);
        art.fill(qRgb(200, 120, 40));
        art.save(QDir(tmp.path()).filePath("art.png"));

        QFile f(QDir(tmp.path()).filePath("art.xml"));
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        f.write("<sprite><slice left=\"2\" top=\"3\" right=\"4\" bottom=\"5\"/></sprite>");
        f.close();
    }

    PixelModel::SetMode(PixelModel::Mode::PixelGrid);
    PixelModel::SetUnit(3.5);

    SceneDocument doc;
    UiElement* e = doc.CreateImageElement("Art", nullptr);
    auto* img = e->GetComponent<ImageComponent>();
    img->SetImagePath("art.png");
    img->SetTextureFill(PixelDraw::FillWrap);
    Settle();

    check(img->GetSliceLeft() == 2 && img->GetSliceTop() == 3
       && img->GetSliceRight() == 4 && img->GetSliceBottom() == 5,
          "the sidecar slice is adopted into the component's properties");

    const QString out = QDir(tmp.path()).filePath("scene.uibin");
    check(SceneExporter::BakeToUiBin(&doc, out), "the scene bakes");

    // Decode it back through the reference reader.
    UiElement* root = nullptr;
    {
        QFile f(out);
        f.open(QIODevice::ReadOnly);
        root = UiBinReader::Read(f.readAll());
    }

    check(root != nullptr, "and decodes");

    if (root)
    {
        auto* docComp = root->GetComponent<DocumentComponent>();
        check(docComp != nullptr, "the baked root carries a Document component");

        if (docComp)
        {
            check(docComp->GetRenderModel() == 1, "carrying renderModel = PixelGrid");
            check(qFuzzyCompare(docComp->GetPixelUnit(), 3.5), "and the exact pixel unit");
        }

        // The image element is the root's only child.
        UiElement* child = root->ChildElementAt(0);
        check(child != nullptr, "the image element round-trips");

        if (child)
        {
            auto* ri = child->GetComponent<ImageComponent>();
            check(ri != nullptr, "with its Image component");

            if (ri)
            {
                check(ri->GetSliceLeft() == 2 && ri->GetSliceTop() == 3
                   && ri->GetSliceRight() == 4 && ri->GetSliceBottom() == 5,
                      "and the 9-slice insets, which the sidecar alone could not deliver");
                check(ri->GetTextureFill() == PixelDraw::FillWrap, "and the wrap mode");
            }
        }

        delete root;
    }

    PixelModel::SetMode(PixelModel::Mode::Continuous);
    PixelModel::SetUnit(1.0);
    AssetContext::SetBaseDir(QString());
}


// ---------------------------------------------------------------------------
// Appendix A fixes: per-slot asset identity, and the reader conformance rules
// the spec states but the decoder did not previously enforce.
// ---------------------------------------------------------------------------
void CheckUiBinConformance()
{
    std::fprintf(stderr, "7. uibin conformance fixes\n");

    // --- A.3: Button's two asset slots keep distinct identities -------------
    {
        QTemporaryDir tmp;
        AssetContext::SetBaseDir(tmp.path());

        {
            QImage skin(8, 8, QImage::Format_ARGB32);
            skin.fill(qRgb(90, 90, 110));
            skin.save(QDir(tmp.path()).filePath("skin.png"));

            QFile f(QDir(tmp.path()).filePath("face.ttf"));
            f.open(QIODevice::WriteOnly);
            f.write("not a real font, but non-empty");
            f.close();
        }

        SceneDocument doc;
        UiElement* e = doc.CreateButtonElement("Btn", nullptr);
        auto* b = e->GetComponent<ButtonComponent>();

        b->SetImagePath("skin.png");
        b->SetAssetDomain("textures");
        b->SetAssetRegistryValue("ui/button_skin");

        b->SetFontPath("face.ttf");
        b->SetFontDomain("fonts");
        b->SetFontRegistryValue("ui/display_face");
        Settle();

        const QString out = QDir(tmp.path()).filePath("btn.uibin");
        check(SceneExporter::BakeToUiBin(&doc, out), "a two-asset Button bakes");

        UiElement* root = nullptr;
        {
            QFile f(out);
            f.open(QIODevice::ReadOnly);
            root = UiBinReader::Read(f.readAll());
        }
        check(root != nullptr, "and decodes");

        if (root)
        {
            UiElement* c = root->ChildElementAt(0);
            auto* rb = c ? c->GetComponent<ButtonComponent>() : nullptr;
            check(rb != nullptr, "with its Button component");

            if (rb)
            {
                // Before the fix, one identity was stamped on BOTH asset
                // records and the last one applied won, so the font's registry
                // key came back as the skin's.
                check(rb->GetAssetDomain() == "textures"
                   && rb->GetAssetRegistryValue() == "ui/button_skin",
                      "the skin keeps its own identity");
                check(rb->GetFontDomain() == "fonts"
                   && rb->GetFontRegistryValue() == "ui/display_face",
                      "and the font keeps a DIFFERENT one (A.3)");
            }
            delete root;
        }

        AssetContext::SetBaseDir(QString());
    }

    // --- Reader conformance -------------------------------------------------
    // Build a minimal valid file, then corrupt one field at a time.
    auto makeFile = [](quint16 version, quint32 strOff, quint32 assetOff,
                       quint32 treeOff, quint32 strCount, quint32 strLen) -> QByteArray
    {
        QByteArray body;
        {
            QDataStream s(&body, QIODevice::WriteOnly);
            s.setByteOrder(QDataStream::LittleEndian);
            s << quint32(strLen);              // one string, length as given
        }

        QByteArray out;
        out.append("UIB4", 4);
        QDataStream h(&out, QIODevice::WriteOnly | QIODevice::Append);
        h.setByteOrder(QDataStream::LittleEndian);
        h << version << quint16(0);
        h << strOff << strCount;
        h << assetOff << quint32(0);
        h << treeOff;
        h << quint32(0);                       // fileSize, patched below
        while (out.size() < 32) out.append('\0');

        QByteArray masked = body;
        uibin::Obfuscate(masked.data(), masked.size());
        out.append(masked);

        const quint32 total = quint32(out.size());
        std::memcpy(out.data() + 28, &total, 4);
        return out;
    };

    {
        // Wrong version is refused (and now before the demask pass, R3.5).
        UiElement* r = UiBinReader::Read(makeFile(9, 32, 36, 36, 0, 0));
        check(r == nullptr, "a wrong version is refused");
        delete r;
    }
    {
        // strOff must be exactly 32 (R3.8).
        UiElement* r = UiBinReader::Read(makeFile(4, 33, 36, 36, 0, 0));
        check(r == nullptr, "a string table that does not start at 32 is refused");
        delete r;
    }
    {
        // Sections must not run backwards (R3.8).
        UiElement* r = UiBinReader::Read(makeFile(4, 32, 36, 34, 0, 0));
        check(r == nullptr, "sections that run backwards are refused");
        delete r;
    }
    {
        // A string length that stays inside the FILE but overruns its own
        // SECTION must be refused (R17.1). assetOff is 36, so the single
        // string's 8 bytes run past it.
        UiElement* r = UiBinReader::Read(makeFile(4, 32, 36, 36, 1, 8));
        check(r == nullptr, "a string overrunning its section is refused (R17.1)");
        delete r;
    }
}
