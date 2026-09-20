// Layout container checks.
//
// Two defects motivated these, both reported as "layouts are simply broken":
//
//   1. A stretch-marked child inside a layout grew the layout without bound -
//      +2*padding on every interaction. Stretch resolved against the parent,
//      but a layout's size IS its children's, so the child sized to the
//      layout and the layout re-measured from the child. Circular.
//
//   2. A layout appearing to size to one child rather than all of them. That
//      is the same defect seen from the cross axis: every stretch child
//      collapsed to the same circular width, so the widest-child rule had
//      only one width to pick from.
//
// The sizing matrix below is here to keep the second claim honest - it
// computes each expected size independently of the layout code.
#include "checks.hpp"

#include "core/Anchor.hpp"
#include "core/GridSnap.hpp"
#include "core/PixelModel.hpp"
#include "core/UiElement.hpp"
#include "components/GridLayoutComponent.hpp"
#include "components/ScrollBoxComponent.hpp"
#include "components/StackLayoutComponent.hpp"
#include "components/TransformComponent.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneElementItem.hpp"

#include <QString>

#include <cstdio>

namespace
{
    const AnchorFlags kStretchX((int)Anchor::LEFT | (int)Anchor::RIGHT);
    const AnchorFlags kStretchY((int)Anchor::TOP | (int)Anchor::BOTTOM);
    const AnchorFlags kStretchBoth((int)Anchor::LEFT | (int)Anchor::RIGHT
                                 | (int)Anchor::TOP | (int)Anchor::BOTTOM);

    QSizeF SizeOf(SceneDocument& doc, UiElement* e)
    {
        SceneElementItem* item = doc.GetItem(e);

        return item ? item->boundingRect().size() : QSizeF();
    }

    bool Near(double a, double b)
    {
        return qAbs(a - b) < 0.51;
    }

    void ExpectSize(SceneDocument& doc, UiElement* e, double w, double h, const QString& what)
    {
        const QSizeF s = SizeOf(doc, e);

        const bool ok = Near(s.width(), w) && Near(s.height(), h);

        check(ok, qPrintable(what));

        if (!ok)
            std::fprintf(stderr, "      got %.1f x %.1f, expected %.1f x %.1f\n",
                         s.width(), s.height(), w, h);
    }

    UiElement* Panel(SceneDocument& doc, const QString& name, UiElement* parent, double w, double h)
    {
        UiElement* p = doc.CreatePanelElement(name, parent);
        p->GetComponent<TransformComponent>()->SetScale(QPointF(w, h));

        return p;
    }
}

void CheckLayouts()
{
    std::fprintf(stderr, "layout sizing\n");

    GridSnap::SetEnabled(false);
    PixelModel::SetMode(PixelModel::Mode::Continuous);

    // --- a layout is the SUM of all its children, not one of them ----------
    for (int dir = 0; dir < 2; ++dir)
    {
        const bool vertical = (dir == 0);
        const double pad = 7.0;
        const double gap = 3.0;

        // Deliberately unequal, and deliberately not largest-first: a loop that
        // measured only the first child would pass with uniform children.
        const double w[4] = { 30.0, 90.0, 55.0, 20.0 };
        const double h[4] = { 12.0, 40.0, 25.0, 60.0 };

        for (int n = 1; n <= 4; ++n)
        {
            SceneDocument doc;
            UiElement* stack = doc.CreateStackLayoutElement("Stack", nullptr);

            auto* sl = stack->GetComponent<StackLayoutComponent>();
            sl->SetDirection(vertical ? StackLayoutComponent::Vertical : StackLayoutComponent::Horizontal);
            sl->SetPadding(pad);
            sl->SetSpacing(gap);

            double run = 0.0;
            double cross = 0.0;

            for (int i = 0; i < n; ++i)
            {
                Panel(doc, QStringLiteral("P%1").arg(i), stack, w[i], h[i]);

                run   += vertical ? h[i] : w[i];
                cross  = std::max(cross, vertical ? w[i] : h[i]);
            }

            Settle();
            Settle();

            run += gap * (n - 1) + 2 * pad;
            cross += 2 * pad;

            ExpectSize(doc, stack, vertical ? cross : run, vertical ? run : cross,
                       QStringLiteral("%1 stack of %2 unequal children sums all of them")
                           .arg(vertical ? "vertical" : "horizontal").arg(n));
        }
    }

    // --- grid: widest cell per column, tallest per row ----------------------
    {
        SceneDocument doc;
        UiElement* grid = doc.CreateGridLayoutElement("Grid", nullptr);

        auto* gl = grid->GetComponent<GridLayoutComponent>();
        gl->SetColumns(2);
        gl->SetPadding(5.0);
        gl->SetSpacingH(4.0);
        gl->SetSpacingV(6.0);

        //  col0        col1
        //  30 x 10     50 x 20      row0 -> heights 20, widths 30 / 50
        //  40 x 15     20 x 35      row1 -> heights 35, widths 40 / 20
        Panel(doc, "A", grid, 30, 10);
        Panel(doc, "B", grid, 50, 20);
        Panel(doc, "C", grid, 40, 15);
        Panel(doc, "D", grid, 20, 35);

        Settle();
        Settle();

        const double wantW = 40.0 + 50.0 + 4.0 + 2 * 5.0;
        const double wantH = 20.0 + 35.0 + 6.0 + 2 * 5.0;

        ExpectSize(doc, grid, wantW, wantH, "a grid sums its column widths and row heights");
    }

    // --- nesting: an inner layout contributes its OWN summed size -----------
    {
        SceneDocument doc;
        UiElement* outer = doc.CreateStackLayoutElement("Outer", nullptr);
        outer->GetComponent<StackLayoutComponent>()->SetPadding(10.0);
        outer->GetComponent<StackLayoutComponent>()->SetSpacing(5.0);

        for (int i = 0; i < 2; ++i)
        {
            UiElement* inner = doc.CreateStackLayoutElement(QStringLiteral("Inner%1").arg(i), outer);
            inner->GetComponent<StackLayoutComponent>()->SetPadding(4.0);
            inner->GetComponent<StackLayoutComponent>()->SetSpacing(2.0);

            Panel(doc, QStringLiteral("P%1a").arg(i), inner, 50, 20);
            Panel(doc, QStringLiteral("P%1b").arg(i), inner, 30, 20);
        }

        Settle();
        Settle();
        Settle();

        // inner: 4 + 20 + 2 + 20 + 4 = 50 tall, 4 + 50 + 4 = 58 wide
        // outer: 10 + 50 + 5 + 50 + 10 = 125 tall, 10 + 58 + 10 = 78 wide
        ExpectSize(doc, outer, 78.0, 125.0, "an outer layout sums its inner layouts' real sizes");

        // ...and a change to a LEAF still reaches the outermost container.
        UiElement* leaf = nullptr;
        for (QObject* c : outer->children())
            if (auto* inner = qobject_cast<UiElement*>(c))
                if (inner->GetName() == QLatin1String("Inner0"))
                    for (QObject* cc : inner->children())
                        if (auto* p = qobject_cast<UiElement*>(cc))
                            if (p->GetName() == QLatin1String("P0a"))
                                leaf = p;

        check(leaf != nullptr, "found the nested leaf");

        if (leaf)
        {
            leaf->GetComponent<TransformComponent>()->SetScale(QPointF(50.0, 70.0));
            Settle();
            Settle();
            Settle();

            // Inner0 grows by 50 -> 100 tall; outer follows.
            ExpectSize(doc, outer, 78.0, 175.0, "and a leaf resize propagates out through the sub-layout");
        }
    }
}

void CheckLayoutStretch()
{
    std::fprintf(stderr, "stretch inside a layout\n");

    GridSnap::SetEnabled(false);
    PixelModel::SetMode(PixelModel::Mode::Continuous);

    // --- the runaway ---------------------------------------------------------
    {
        SceneDocument doc;
        UiElement* outer = Panel(doc, "Outer", nullptr, 400.0, 300.0);

        UiElement* stack = doc.CreateStackLayoutElement("Stack", outer);
        auto* sl = stack->GetComponent<StackLayoutComponent>();
        sl->SetPadding(10.0);
        sl->SetSpacing(5.0);

        UiElement* child = doc.CreatePanelElement("Stretchy", stack);
        auto* cx = child->GetComponent<TransformComponent>();
        cx->SetStretch(kStretchBoth);

        Settle();
        Settle();

        const QSizeF settled = SizeOf(doc, stack);

        // Each poke is one interaction: selecting the child, nudging it, an
        // ancestor resizing. Before the fix every one of these added 2*padding
        // and never stopped.
        for (int i = 0; i < 8; ++i)
        {
            cx->SetRotationDegrees(i % 2 ? 0.0 : 0.0001);
            Settle();
        }

        const QSizeF after = SizeOf(doc, stack);

        const bool stable = Near(settled.width(), after.width()) && Near(settled.height(), after.height());

        check(stable, "a stretch child does not grow its layout on every interaction");

        if (!stable)
            std::fprintf(stderr, "      %.1f x %.1f settled, %.1f x %.1f after 8 pokes\n",
                         settled.width(), settled.height(), after.width(), after.height());

        // It fills the nearest NON-layout ancestor, which is what stretch means.
        ExpectSize(doc, child, 400.0, 300.0, "and fills the nearest non-layout ancestor instead");
    }

    // --- the canonical pattern: a column of full-width rows -----------------
    {
        SceneDocument doc;
        UiElement* outer = Panel(doc, "Outer", nullptr, 400.0, 300.0);

        UiElement* stack = doc.CreateStackLayoutElement("Stack", outer);
        stack->GetComponent<StackLayoutComponent>()->SetPadding(0.0);
        stack->GetComponent<StackLayoutComponent>()->SetSpacing(0.0);

        for (int i = 0; i < 3; ++i)
        {
            UiElement* row = Panel(doc, QStringLiteral("Row%1").arg(i), stack, 50.0, 20.0);
            row->GetComponent<TransformComponent>()->SetStretch(kStretchX);
        }

        Settle();
        Settle();

        ExpectSize(doc, stack, 400.0, 60.0, "a column of full-width rows is as wide as the panel and as tall as the rows");

        for (QObject* c : stack->children())
            if (auto* row = qobject_cast<UiElement*>(c))
                ExpectSize(doc, row, 400.0, 20.0,
                           qPrintable(QStringLiteral("%1 spans the full width").arg(row->GetName())));
    }

    // --- per axis: a ScrollBox owns its scroll axis -------------------------
    {
        SceneDocument doc;
        UiElement* outer = Panel(doc, "Outer", nullptr, 400.0, 300.0);

        UiElement* box = doc.CreateScrollBoxElement("Box", outer);
        box->GetComponent<TransformComponent>()->SetScale(QPointF(180.0, 120.0));
        box->GetComponent<ScrollBoxComponent>()->SetDirection(ScrollBoxComponent::Vertical);
        box->GetComponent<ScrollBoxComponent>()->SetPadding(0.0);
        box->GetComponent<ScrollBoxComponent>()->SetSpacing(0.0);

        UiElement* child = doc.CreatePanelElement("Child", box);
        child->GetComponent<TransformComponent>()->SetStretch(kStretchBoth);

        Settle();
        Settle();

        // Height comes from the box, which keeps its own extent along the
        // scroll axis. Width comes from Outer, because the box fits its width
        // to its children and so cannot hand one out.
        ExpectSize(doc, child, 400.0, 120.0,
                   "in a vertical ScrollBox a stretch child takes the box's height but the panel's width");
    }

    // --- a layout at the very top falls back to the canvas -------------------
    {
        SceneDocument doc;
        UiElement* stack = doc.CreateStackLayoutElement("Stack", nullptr);
        stack->GetComponent<StackLayoutComponent>()->SetPadding(0.0);

        UiElement* child = doc.CreatePanelElement("Child", stack);
        child->GetComponent<TransformComponent>()->SetStretch(kStretchBoth);

        Settle();
        Settle();

        const QRectF canvas = doc.GetCanvasRect();

        ExpectSize(doc, child, canvas.width(), canvas.height(),
                   "with no non-layout ancestor at all, stretch fills the design canvas");
    }
}
