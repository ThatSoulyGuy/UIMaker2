// Regression checks for defects that were fixed and must stay fixed. Each one
// is here because it was either a crash, a data-loss path, or a behaviour the
// plan's later stages depend on.
#include "app/MainWindow.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/UiBinReader.hpp"
#include "scene/UiBinCommon.hpp"
#include "scene/SceneExporter.hpp"
#include "components/TransformComponent.hpp"
#include "ui/PropertyEditorPanel.hpp"
#include "ui/EntityTreeModel.hpp"
#include "input/TransformInputHandler.hpp"
#include "input/EditorContext.hpp"
#include "input/InputEvents.hpp"
#include "gizmos/GizmoManager.hpp"
#include "input/PanZoomHandler.hpp"
#include <QScrollBar>
#include "scene/SceneElementItem.hpp"
#include "core/GridSnap.hpp"
#include "core/PixelModel.hpp"
#include <QGraphicsView>
#include <QSignalSpy>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include "scene/SceneExporter.hpp"
#include "components/TransformComponent.hpp"
#include "ui/PropertyEditorPanel.hpp"
#include "ui/EntityTreeModel.hpp"
#include "input/TransformInputHandler.hpp"
#include "input/EditorContext.hpp"
#include "input/InputEvents.hpp"
#include "gizmos/GizmoManager.hpp"
#include "input/PanZoomHandler.hpp"
#include <QScrollBar>
#include "scene/SceneElementItem.hpp"
#include "core/GridSnap.hpp"
#include "core/PixelModel.hpp"
#include <QGraphicsView>
#include <QSignalSpy>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include "core/UiElement.hpp"

#include <QApplication>
#include "checks.hpp"
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <cstdio>
#include <cstring>


namespace
{
    // A v4 header whose string-table length field is hostile. Before the bounds
    // hardening, `cur + n > size` overflowed on this and read out of bounds.
    QByteArray CraftedUiBin(quint32 strLen)
    {
        QByteArray body;
        {
            QDataStream s(&body, QIODevice::WriteOnly);
            s.setByteOrder(QDataStream::LittleEndian);
            s << quint32(strLen);
        }

        QByteArray out;
        out.append("UIB4", 4);
        QDataStream h(&out, QIODevice::WriteOnly | QIODevice::Append);
        h.setByteOrder(QDataStream::LittleEndian);
        h << quint16(4) << quint16(0);
        h << quint32(32) << quint32(1);
        h << quint32(32 + body.size()) << quint32(0);
        h << quint32(32 + body.size());
        h << quint32(32 + body.size());

        while (out.size() < 32) out.append('\0');

        QByteArray masked = body;
        uibin::Obfuscate(masked.data(), masked.size());
        out.append(masked);

        const quint32 total = quint32(out.size());
        std::memcpy(out.data() + 28, &total, 4);

        return out;
    }
}

void CheckRegressions()
{


    std::fprintf(stderr, "application shutdown (the reported crash on quit)\n");
    {
        // A shutdown smoke test, NOT a regression test for the quit crash.
        // Verified: with the teardown guard removed this still passes, because
        // the offscreen platform does not reproduce the Cocoa teardown ordering
        // that segfaults on a real window. The check immediately below - that
        // destruction emits no SelectionChanged - is the one that actually pins
        // the regression; it fails without the guard.
        //
        // QSettings here uses this binary's (empty) org/app identity, so it
        // neither reads nor writes the user's real preferences.
        auto* w = new MainWindow();
        w->show();            // the crash needs a realised viewport
        Settle();

        delete w;
        Settle();

        check(true, "constructing and destroying MainWindow does not crash");
    }

    std::fprintf(stderr, "SceneDocument teardown order\n");
    {
        // The quit crash: removeItem() during destruction deselects the item,
        // QGraphicsScene emits selectionChanged, SceneDocument re-emits
        // SelectionChanged, and whatever is connected runs against a half-torn-
        // down window. Reproducing it REQUIRES a live listener - the first
        // version of this check had none, which is exactly why it passed while
        // the app segfaulted on exit.
        auto* doc = new SceneDocument();
        UiElement* panel = doc->CreatePanelElement("Panel", nullptr);
        doc->CreateTextElement("Text", panel);
        doc->SetSelected(panel);
        Settle();

        int emittedDuringTeardown = 0;
        bool tearingDown = false;

        QObject::connect(doc, &SceneDocument::SelectionChanged,
                         [&](const QList<UiElement*>&)
                         {
                             if (tearingDown)
                                 ++emittedDuringTeardown;
                         });

        tearingDown = true;
        delete doc;

        check(emittedDuringTeardown == 0,
              "destruction emits no SelectionChanged (the segfault-on-quit path)");
    }

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

    std::fprintf(stderr, "save-in-place round trip (Ctrl+S writes into the project root)\n");
    {
        QTemporaryDir tmp;
        check(tmp.isValid(), "temp project root created");

        const QString rootDir = tmp.path();
        const double markerX = 137.5;

        {
            SceneDocument doc;
            doc.SetBaseDir(rootDir);
            UiElement* panel = doc.CreatePanelElement("Panel", nullptr);
            UiElement* text  = doc.CreateTextElement("Title", panel);
            text->GetComponent<TransformComponent>()->SetPosition(QPointF(markerX, 42.0));

            check(SceneExporter::ExportToFolder(&doc, rootDir), "first save writes scene.json");

            // Second save into the SAME folder: this is the Ctrl+S path, where
            // every asset would be copied onto itself.
            check(SceneExporter::ExportToFolder(&doc, rootDir), "saving again in place succeeds");
        }

        check(QFile::exists(QDir(rootDir).filePath("scene.json")), "scene.json exists on disk");

        // Reload and confirm the edit survived both writes.
        SceneDocument reloaded;
        reloaded.SetBaseDir(rootDir);
        QFile f(QDir(rootDir).filePath("scene.json"));
        check(f.open(QIODevice::ReadOnly), "scene.json is readable");
        check(reloaded.LoadJson(f.readAll()), "scene.json reloads");

        UiElement* found = nullptr;
        for (QObject* c : reloaded.GetRoot()->children())
            if (auto* e = qobject_cast<UiElement*>(c))
                for (QObject* g : e->children())
                    if (auto* ge = qobject_cast<UiElement*>(g))
                        if (ge->GetName() == "Title") found = ge;

        check(found != nullptr, "nested Title element survived the round trip");
        if (found)
        {
            const double x = found->GetComponent<TransformComponent>()->GetPosition().x();
            check(qFuzzyCompare(x, markerX), "its edited position survived both saves");
            if (!qFuzzyCompare(x, markerX))
                std::fprintf(stderr, "      expected %.3f, got %.3f\n", markerX, x);
        }
    }

    std::fprintf(stderr, "body drags are owned by the tool system (stage 6)\n");
    {
        GridSnap::SetEnabled(false);

        SceneDocument doc;
        UiElement* panel = doc.CreatePanelElement("Panel", nullptr);
        auto* xf = panel->GetComponent<TransformComponent>();
        xf->SetScale(QPointF(500.0, 400.0));
        xf->SetPosition(QPointF(400.0, 300.0));
        qApp->processEvents();

        QGraphicsView view(doc.GetScene());
        view.resize(1200, 800);
        view.centerOn(doc.GetCanvasRect().center());

        SceneElementItem* item = doc.GetItem(panel);
        check(item != nullptr, "panel has a scene item");
        check(!(item->flags() & QGraphicsItem::ItemIsMovable),
              "ItemIsMovable is cleared, so QGraphicsView no longer moves it");

        GizmoManager gm;
        gm.SetActiveGizmoId("translate");
        TransformInputHandler handler(&gm);

        QSignalSpy ended(&handler, &TransformInputHandler::TransformEnded);

        doc.SetSelected(panel);
        qApp->processEvents();

        EditorContext ctx; ctx.document = &doc; ctx.view = &view;

        // Press on bare BODY: lower-left quadrant, clear of the gizmo's centre
        // quad and of both axis strips (which radiate from the centre).
        const QRectF sb = item->sceneBoundingRect();
        const QPointF bodyScene = sb.center() + QPointF(-sb.width() * 0.35, sb.height() * 0.35);

        MousePressEvent press;
        press.button = Qt::LeftButton;
        press.scenePos = bodyScene;
        press.viewPos = view.mapFromScene(bodyScene);

        const QPointF before = xf->GetPosition();
        InputResult pr = handler.HandlePress(press, ctx);
        check(pr.consumed, "press on a selected element body is consumed by the handler");
        check(handler.GetActiveHandleId() == "translate_free", "it arms the synthetic translate_free handle");
        check(handler.GetUndoActionName() == "Move", "which GetUndoActionName reports as \"Move\"");

        MouseMoveEvent mv;
        mv.scenePos = bodyScene + QPointF(120.0, -60.0);
        mv.viewPos = view.mapFromScene(mv.scenePos);
        handler.HandleMove(mv, ctx);
        qApp->processEvents();

        const QPointF moved = xf->GetPosition() - before;
        check(qAbs(moved.x() - 120.0) < 0.5 && qAbs(moved.y() + 60.0) < 0.5,
              "dragging the body moves it on both axes");
        if (!(qAbs(moved.x() - 120.0) < 0.5))
            std::fprintf(stderr, "      moved by (%.2f, %.2f)\n", moved.x(), moved.y());

        MouseReleaseEvent rel;
        rel.button = Qt::LeftButton;
        rel.scenePos = mv.scenePos;
        rel.viewPos = mv.viewPos;
        handler.HandleRelease(rel, ctx);

        check(ended.count() == 1, "release emits TransformEnded, so the move is undoable");

        // With the Rotate tool active a body press must NOT move anything.
        gm.SetActiveGizmoId("rotate");
        const QPointF beforeRotTool = xf->GetPosition();

        MousePressEvent p2;
        p2.button = Qt::LeftButton;
        p2.scenePos = item->sceneBoundingRect().center()
                    + QPointF(-sb.width() * 0.35, sb.height() * 0.35);
        p2.viewPos = view.mapFromScene(p2.scenePos);

        InputResult pr2 = handler.HandlePress(p2, ctx);
        check(!pr2.consumed, "with the Rotate tool, a body press is not taken as a move");

        MouseMoveEvent mv2;
        mv2.scenePos = p2.scenePos + QPointF(80.0, 80.0);
        mv2.viewPos = view.mapFromScene(mv2.scenePos);
        handler.HandleMove(mv2, ctx);
        qApp->processEvents();

        check(xf->GetPosition() == beforeRotTool, "so the Rotate tool no longer translates elements");
    }

    std::fprintf(stderr, "renames are undoable (stage 5)\n");
    {
        SceneDocument doc;
        UiElement* panel = doc.CreatePanelElement("Panel", nullptr);
        const QUuid id = panel->GetId();

        PropertyEditorPanel inspector;
        inspector.SetTarget(panel);

        QList<PropertyEditRecord> captured;
        QObject::connect(&inspector, &PropertyEditorPanel::PropertyChangeApplied,
                         [&captured](const QList<PropertyEditRecord>& r){ captured = r; });

        // Exactly what both rename entry points now call.
        inspector.ApplyPropertyChange(panel, "name", QStringLiteral("Renamed"));

        check(panel->GetName() == "Renamed", "the rename is applied");
        check(captured.size() == 1, "it emits one PropertyEditRecord");

        if (captured.size() == 1)
        {
            const PropertyEditRecord& r = captured.first();
            check(r.componentKind.isEmpty(),
                  "with an EMPTY componentKind - the element-level case PropertyEditCommand handles");
            check(r.elementId == id, "keyed to the element's id, so undo re-resolves it after a rebuild");
            check(r.before.toString() == "Panel" && r.after.toString() == "Renamed",
                  "carrying both the before and after name");

            // Replaying the record backwards is literally what undo does.
            doc.FindById(r.elementId)->setProperty(r.propName.constData(), r.before);
            check(panel->GetName() == "Panel", "replaying 'before' through setProperty restores the old name");
        }

        // And the tree asks rather than mutating.
        EntityTreeModel model(doc.GetRoot());
        QSignalSpy asked(&model, &EntityTreeModel::RenameRequested);
        const QModelIndex idx = model.GetIndexFromElement(panel);
        check(idx.isValid(), "the panel has a tree row");
        model.setData(idx, QStringLiteral("FromTree"), Qt::EditRole);
        check(asked.count() == 1, "setData emits RenameRequested instead of renaming directly");
        check(panel->GetName() == "Panel", "so the model itself no longer mutates the document");
    }

    std::fprintf(stderr, "the snap grid is square in PixelGrid mode\n");
    {
        const QRectF canvas(0.0, 0.0, 1920.0, 1080.0);

        // Continuous: the X/Y division counts still apply, and a 16:9 canvas
        // split 320x240 legitimately gives non-square cells.
        PixelModel::SetMode(PixelModel::Mode::Continuous);
        GridSnap::SetEnabled(true);
        GridSnap::SetDivisions(320, 240);

        const QSizeF cont = GridSnap::CellSize(canvas);
        check(!GridSnap::DrivenByPixelModel(), "Continuous mode uses the division counts");
        check(qFuzzyCompare(cont.width(), 6.0) && qFuzzyCompare(cont.height(), 4.5),
              "which on a 16:9 canvas at 320x240 gives the old 6.0 x 4.5 cell");

        // PixelGrid: the cell IS the virtual pixel, so it is square whatever the
        // division counts say.
        PixelModel::SetMode(PixelModel::Mode::PixelGrid);
        PixelModel::SetUnit(6.0);

        const QSizeF px = GridSnap::CellSize(canvas);
        check(GridSnap::DrivenByPixelModel(), "PixelGrid mode drives the cell from the unit");
        check(qFuzzyCompare(px.width(), px.height()), "so the cell is SQUARE");
        check(qFuzzyCompare(px.width(), 6.0), "and equals the unit");

        // A snapped point lands on whole units in both axes.
        const QPointF snapped = GridSnap::Snap(QPointF(103.0, 47.0), canvas);
        check(qFuzzyCompare(snapped.x(), 102.0) && qFuzzyCompare(snapped.y(), 48.0),
              "and snapping lands on whole virtual pixels on both axes");

        // A non-integer unit still yields a square cell.
        PixelModel::SetUnit(4.5);
        const QSizeF odd = GridSnap::CellSize(canvas);
        check(qFuzzyCompare(odd.width(), odd.height()), "a fractional unit is still square");

        PixelModel::SetMode(PixelModel::Mode::Continuous);
        PixelModel::SetUnit(1.0);
        GridSnap::SetEnabled(false);
    }

    std::fprintf(stderr, "trackpad scrolls, wheel zooms\n");
    {
        SceneDocument doc;
        doc.CreatePanelElement("P", nullptr);
        Settle();

        QGraphicsView view(doc.GetScene());
        view.resize(900, 600);
        view.setTransform(QTransform::fromScale(1.0, 1.0));
        view.centerOn(doc.GetCanvasRect().center());

        PanZoomHandler pz;
        EditorContext ctx; ctx.document = &doc; ctx.view = &view;

        auto zoom = [&]{ return view.transform().m11(); };
        auto scrollPos = [&]{ return QPoint(view.horizontalScrollBar()->value(),
                                            view.verticalScrollBar()->value()); };

        // A two-finger trackpad swipe must PAN, not zoom. Previously every wheel
        // event zoomed, which is what made scrolling a jittery mess.
        {
            const double z0 = zoom();
            const QPoint s0 = scrollPos();

            WheelEvent e;
            e.viewPos = QPoint(450, 300);
            e.pixelDelta = QPoint(0, 40);
            e.angleDelta = QPoint(0, 0);
            e.fromTrackpad = true;

            const InputResult r = pz.HandleWheel(e, ctx);

            check(r.consumed, "a trackpad swipe is handled");
            check(qFuzzyCompare(zoom(), z0), "and does NOT change the zoom");
            check(scrollPos() != s0, "it scrolls the view instead");
        }

        // Horizontal-only swipe must scroll horizontally. This used to have
        // angleDelta.y()==0, so delta was 0 and it fell through entirely.
        {
            const QPoint s0 = scrollPos();

            WheelEvent e;
            e.viewPos = QPoint(450, 300);
            e.pixelDelta = QPoint(55, 0);
            e.fromTrackpad = true;

            check(pz.HandleWheel(e, ctx).consumed, "a horizontal swipe is handled");
            check(scrollPos().x() != s0.x(), "and scrolls horizontally");
        }

        // A mouse wheel detent still zooms.
        {
            const double z0 = zoom();

            WheelEvent e;
            e.viewPos = QPoint(450, 300);
            e.angleDelta = QPoint(0, 120);
            e.delta = 120;
            e.fromTrackpad = false;

            check(pz.HandleWheel(e, ctx).consumed, "a wheel detent is handled");
            check(zoom() > z0, "and zooms in");
        }

        // Ctrl/Cmd + trackpad zooms, so pinch-style zoom still works on a laptop.
        {
            const double z0 = zoom();

            WheelEvent e;
            e.viewPos = QPoint(450, 300);
            e.pixelDelta = QPoint(0, 30);
            e.angleDelta = QPoint(0, 30);
            e.fromTrackpad = true;
            e.modifiers = Qt::ControlModifier;

            check(pz.HandleWheel(e, ctx).consumed, "ctrl + trackpad is handled");
            check(zoom() > z0, "and zooms rather than scrolling");
        }

        // Pinch. BOTH directions: macOS reports the increment as positive when
        // spreading fingers and NEGATIVE when pinching together, so a guard on
        // the sign of the increment silently drops every zoom-out.
        {
            const double z0 = zoom();
            check(pz.HandlePinch(QPoint(450, 300), 0.10, ctx).consumed, "a pinch out is handled");
            check(zoom() > z0, "and zooms in");

            const double z1 = zoom();
            check(pz.HandlePinch(QPoint(450, 300), -0.10, ctx).consumed, "a pinch IN is handled");
            check(zoom() < z1, "and zooms OUT");
        }

        // Wheel, both directions.
        {
            const double z0 = zoom();

            WheelEvent down;
            down.viewPos = QPoint(450, 300);
            down.angleDelta = QPoint(0, -120);
            down.delta = -120;
            down.fromTrackpad = false;

            check(pz.HandleWheel(down, ctx).consumed, "a wheel detent DOWN is handled");
            check(zoom() < z0, "and zooms out");
        }

        // A view parked OUTSIDE the zoom limits (fitInView/FitToItem set the
        // transform directly and bypass ZoomAt) must never have its gesture
        // REVERSED by the clamp. Clamping the factor rather than the resulting
        // zoom turned a zoom-out request into a 2.5x zoom IN.
        {
            view.setTransform(QTransform::fromScale(30.0, 30.0));   // above max 20

            WheelEvent in;
            in.viewPos = QPoint(450, 300);
            in.angleDelta = QPoint(0, 120);
            in.delta = 120;

            pz.HandleWheel(in, ctx);
            check(view.transform().m11() >= 30.0,
                  "above the max, a zoom-IN gesture never zooms OUT");

            view.setTransform(QTransform::fromScale(0.02, 0.02));   // below min 0.05

            WheelEvent out;
            out.viewPos = QPoint(450, 300);
            out.angleDelta = QPoint(0, -120);
            out.delta = -120;

            pz.HandleWheel(out, ctx);
            check(view.transform().m11() <= 0.02,
                  "below the min, a zoom-OUT gesture never zooms IN");

            // And coming back INTO range is still allowed.
            view.setTransform(QTransform::fromScale(30.0, 30.0));
            pz.HandleWheel(out, ctx);
            check(view.transform().m11() < 30.0, "but zooming back toward the range still works");

            view.setTransform(QTransform::fromScale(1.0, 1.0));
        }

        // Ctrl + trackpad, both directions.
        {
            const double z0 = zoom();

            WheelEvent e;
            e.viewPos = QPoint(450, 300);
            e.pixelDelta = QPoint(0, -30);
            e.angleDelta = QPoint(0, -30);
            e.fromTrackpad = true;
            e.modifiers = Qt::ControlModifier;

            check(pz.HandleWheel(e, ctx).consumed, "ctrl + trackpad DOWN is handled");
            check(zoom() < z0, "and zooms out");
        }
    }

    std::fprintf(stderr, "structure batching preserves geometry and linearises load (stage 7)\n");
    {
        // Build a tree through the UNBATCHED authoring path (CreateXElement),
        // then round-trip it through the BATCHED load path and require the
        // laid-out geometry to come back bit-identical.
        auto checksum = [](SceneDocument& d)
        {
            double sum = 0.0;
            std::function<void(UiElement*)> walk = [&](UiElement* e)
            {
                for (QObject* c : e->children())
                {
                    if (auto* ce = qobject_cast<UiElement*>(c))
                    {
                        if (SceneElementItem* it = d.GetItem(ce))
                        {
                            sum += it->scenePos().x() + it->scenePos().y()
                                 + it->boundingRect().width() + it->boundingRect().height();
                        }
                        walk(ce);
                    }
                }
            };
            walk(d.GetRoot());
            return sum;
        };

        auto build = [](SceneDocument& d, int panels, int perPanel)
        {
            for (int i = 0; i < panels; ++i)
            {
                UiElement* p = d.CreatePanelElement(QStringLiteral("P%1").arg(i), nullptr);
                p->GetComponent<TransformComponent>()->SetPosition(QPointF(i * 13.0, i * 7.0));

                for (int j = 0; j < perPanel; ++j)
                {
                    UiElement* t = d.CreateTextElement(QStringLiteral("T%1_%2").arg(i).arg(j), p);
                    t->GetComponent<TransformComponent>()->SetPosition(QPointF(j * 11.0, j * 5.0));
                }
            }
        };


        SceneDocument authored;
        build(authored, 12, 9);
        Settle();
        const double authoredSum = checksum(authored);

        const QByteArray json = authored.ExportJson();

        SceneDocument loaded;
        check(loaded.LoadJson(json), "the authored scene reloads");
        Settle();
        const double loadedSum = checksum(loaded);

        check(authoredSum == loadedSum, "batched load produces bit-identical geometry");
        if (authoredSum != loadedSum)
            std::fprintf(stderr, "      authored %.6f vs loaded %.6f\n", authoredSum, loadedSum);

        // Scaling: the per-element cost must stay flat, not grow with N.
        double perElement[3] = {0, 0, 0};
        const int sizes[3] = {10, 40, 90};

        for (int k = 0; k < 3; ++k)
        {
            SceneDocument src;
            build(src, sizes[k], 9);
            Settle();
            const QByteArray blob = src.ExportJson();
            const int count = sizes[k] * 10;

            SceneDocument dst;
            QElapsedTimer t; t.start();
            dst.LoadJson(blob);
            perElement[k] = double(t.nsecsElapsed()) / 1e6 / count;

            std::fprintf(stderr, "      %4d elements: %7.4f ms total, %.5f ms/element\n",
                         count, perElement[k] * count, perElement[k]);
        }

        // Quadratic would make the largest case's per-element cost balloon.
        // Allow generous slack for noise; the pre-batch code was ~9x here.
        check(perElement[2] < perElement[0] * 3.0,
              "per-element load cost stays flat as the scene grows (linear, not quadratic)");
    }

}
