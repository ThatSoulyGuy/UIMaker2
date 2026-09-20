// Smoke checks for the two changes that are hard to reach by clicking:
// the SceneDocument teardown order, and the UiBinReader bounds hardening.
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
#include "scene/SceneElementItem.hpp"
#include "core/GridSnap.hpp"
#include <QGraphicsView>
#include <QSignalSpy>
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
#include "scene/SceneElementItem.hpp"
#include "core/GridSnap.hpp"
#include <QGraphicsView>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
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

    std::fprintf(stderr, "%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASS",
                failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
