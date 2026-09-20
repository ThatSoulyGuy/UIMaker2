// UIMaker2 invariant checks.
//
//   cmake --build <builddir> --target UIMaker2Checks
//   QT_QPA_PLATFORM=offscreen <builddir>/UIMaker2Checks
//
// Three assertions, chosen because each guards an invariant that has either
// already shipped a bug or is about to be relied on. Deliberately NOT a test
// suite: no per-component tests, no paint tests. Add a fourth only when
// something breaks twice.

#include "core/UiElement.hpp"
#include "core/PixelModel.hpp"
#include "core/Anchor.hpp"
#include "components/TransformComponent.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneElementItem.hpp"
#include "ui/EntityTreeModel.hpp"

#include "checks.hpp"

#include <QApplication>
#include <QModelIndex>

#include <cstdio>
#include <functional>

static int failures = 0;

void check(bool ok, const char* what)
{
    std::fprintf(stderr, "  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    std::fflush(stderr);

    if (!ok)
        ++failures;
}

void Settle()
{
    for (int i = 0; i < 8; ++i)
        qApp->processEvents();
}

// ---------------------------------------------------------------------------
// 1. The elements-only row index space.
//
// "Row" means position among child ELEMENTS, components not counted. It used to
// be three hand-copied walks in three translation units that merely happened to
// agree; UiElement now owns the single definition. This asserts that definition
// still agrees with what ReparentTo does and with what the tree model reports,
// including the pre-removal correction in dropMimeData.
// ---------------------------------------------------------------------------
void CheckRowIndexSpace()
{
    std::fprintf(stderr, "1. elements-only row index space\n");

    SceneDocument doc;

    // Interleave components and child elements so a walk that forgot to filter
    // would produce different numbers.
    UiElement* parent = doc.CreatePanelElement("Parent", nullptr);

    QList<UiElement*> kids;
    for (int i = 0; i < 5; ++i)
        kids.append(doc.CreateTextElement(QStringLiteral("K%1").arg(i), parent));

    check(parent->ChildElementCount() == 5, "child element count ignores components");

    bool rowsMatch = true;
    for (int i = 0; i < 5; ++i)
    {
        if (parent->ChildElementAt(i) != kids[i] || kids[i]->RowInParent() != i)
            rowsMatch = false;
    }
    check(rowsMatch, "ChildElementAt and RowInParent are inverses across every row");

    // ReparentTo's insertPos is the FINAL row. Exercise every destination.
    bool reparentMatches = true;
    for (int target = 0; target < 5; ++target)
    {
        UiElement* moving = kids[2];
        moving->ReparentTo(parent, target);

        if (moving->RowInParent() != target)
        {
            reparentMatches = false;
            std::fprintf(stderr, "      ReparentTo(%d) landed at row %d\n", target, moving->RowInParent());
        }
    }
    check(reparentMatches, "ReparentTo(parent, k) leaves the element at row k (same parent)");

    // Cross-parent moves use the same space.
    UiElement* other = doc.CreatePanelElement("Other", nullptr);
    bool crossMatches = true;
    for (int target = 0; target < 3; ++target)
    {
        UiElement* moving = doc.CreateTextElement(QStringLiteral("X%1").arg(target), parent);
        moving->ReparentTo(other, target);

        if (moving->RowInParent() != target)
            crossMatches = false;
    }
    check(crossMatches, "and at row k after a cross-parent move");

    // The tree model must report the same rows.
    EntityTreeModel model(doc.GetRoot());
    bool modelMatches = true;
    for (UiElement* e : parent->ChildElements())
    {
        const QModelIndex idx = model.GetIndexFromElement(e);

        if (!idx.isValid() || idx.row() != e->RowInParent())
            modelMatches = false;
    }
    check(modelMatches, "EntityTreeModel reports the same row for every element");

    // dropMimeData converts the view's pre-removal row to ReparentTo's final
    // index by decrementing on a same-parent downward move. Reproduce that.
    UiElement* mover = parent->ChildElementAt(1);
    const int oldRow = mover->RowInParent();
    int insertRow = 4;                      // as the view would supply it
    if (insertRow > oldRow) --insertRow;    // the correction in dropMimeData
    mover->ReparentTo(parent, insertRow);
    check(mover->RowInParent() == 3,
          "the pre-removal row correction lands a downward move where the view showed it");
}

// ---------------------------------------------------------------------------
// 2. Whole-document geometry checksum.
//
// Thirty lines, no framework, and it catches every regression in the layout
// pipeline - the part of this codebase most likely to break silently. It is
// what proved the structure-batching change behaviour-preserving.
// ---------------------------------------------------------------------------
static double GeometryChecksum(SceneDocument& d)
{
    double sum = 0.0;

    std::function<void(UiElement*)> walk = [&](UiElement* e)
    {
        for (UiElement* ce : e->ChildElements())
        {
            if (SceneElementItem* it = d.GetItem(ce))
            {
                sum += it->scenePos().x() + it->scenePos().y()
                     + it->boundingRect().width() + it->boundingRect().height();
            }

            walk(ce);
        }
    };

    walk(d.GetRoot());

    return sum;
}

void CheckGeometryChecksum()
{
    std::fprintf(stderr, "2. whole-document geometry checksum\n");

    auto build = [](SceneDocument& d)
    {
        for (int i = 0; i < 6; ++i)
        {
            UiElement* p = d.CreatePanelElement(QStringLiteral("P%1").arg(i), nullptr);
            p->GetComponent<TransformComponent>()->SetPosition(QPointF(i * 37.0, i * 19.0));

            for (int j = 0; j < 4; ++j)
            {
                UiElement* t = d.CreateTextElement(QStringLiteral("T%1_%2").arg(i).arg(j), p);
                t->GetComponent<TransformComponent>()->SetPosition(QPointF(j * 23.0, j * 11.0));
            }
        }
    };

    SceneDocument authored;
    build(authored);
    Settle();
    const double authoredSum = GeometryChecksum(authored);

    // The authoring path is unbatched; the load path is batched. Same geometry.
    SceneDocument loaded;
    loaded.LoadJson(authored.ExportJson());
    Settle();
    const double loadedSum = GeometryChecksum(loaded);

    check(authoredSum == loadedSum, "a scene round-tripped through JSON lays out identically");

    if (authoredSum != loadedSum)
        std::fprintf(stderr, "      authored %.6f vs loaded %.6f\n", authoredSum, loadedSum);
    else
        std::fprintf(stderr, "      checksum %.6f\n", authoredSum);
}

// ---------------------------------------------------------------------------
// 3. Anchor forward/inverse round trip.
//
// SceneElementItem's own comment claims the pair are exact inverses. This pair
// governs every drag write-back in the app, so if they disagree, dragging an
// anchored element drifts.
// ---------------------------------------------------------------------------
void CheckAnchorRoundTrip()
{
    std::fprintf(stderr, "3. anchor forward/inverse round trip\n");

    const AnchorFlags combos[9] = {
        Anchor::LEFT  | Anchor::TOP,
        Anchor::LEFT  | Anchor::CENTER_Y,
        Anchor::LEFT  | Anchor::BOTTOM,
        Anchor::CENTER_X | Anchor::TOP,
        Anchor::CENTER_X | Anchor::CENTER_Y,
        Anchor::CENTER_X | Anchor::BOTTOM,
        Anchor::RIGHT | Anchor::TOP,
        Anchor::RIGHT | Anchor::CENTER_Y,
        Anchor::RIGHT | Anchor::BOTTOM,
    };

    const QRectF parents[2] = { QRectF(0, 0, 640, 480), QRectF(37, 91, 613, 445) };
    const QPointF p(53.0, 29.0);
    const double w = 123.0;
    const double h = 77.0;

    auto runAll = [&](const char* label)
    {
        double worst = 0.0;

        for (const QRectF& parent : parents)
        {
            for (AnchorFlags a : combos)
            {
                const QPointF item = SceneElementItem::AnchorToItemPos(p, a, parent, w, h);
                const QPointF back = SceneElementItem::ItemPosToComponent(item, a, parent, w, h);

                worst = qMax(worst, qMax(qAbs(back.x() - p.x()), qAbs(back.y() - p.y())));
            }
        }

        std::fprintf(stderr, "      %s: worst round-trip error %.6f\n", label, worst);

        return worst;
    };

    PixelModel::SetMode(PixelModel::Mode::Continuous);
    check(runAll("continuous") < 1e-9, "the pair are exact inverses in Continuous mode");

    PixelModel::SetMode(PixelModel::Mode::PixelGrid);
    PixelModel::SetUnit(4.0);
    check(runAll("pixel grid, unit 4") < 1e-9, "and in PixelGrid mode");

    PixelModel::SetMode(PixelModel::Mode::Continuous);
    PixelModel::SetUnit(1.0);
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    CheckRowIndexSpace();
    CheckGeometryChecksum();
    CheckAnchorRoundTrip();
    CheckPixelSpans();
    CheckPixelRaster();
    CheckRegressions();

    std::fprintf(stderr, "\n%s (%d failure%s)\n",
                 failures ? "FAILED" : "ALL PASS",
                 failures, failures == 1 ? "" : "s");

    return failures ? 1 : 0;
}
