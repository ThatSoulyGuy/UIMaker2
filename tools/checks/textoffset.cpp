// Checks for the text offset and for inspector property groups.
//
// Both features share a failure mode worth pinning down: they are declarative.
// A text offset that is never read by a paint path, or a group entry naming a
// property that does not exist, fails SILENTLY - the property is simply not
// where you expect it, and nothing complains.
#include "checks.hpp"

#include "core/Component.hpp"
#include "core/PixelModel.hpp"
#include "core/UiElement.hpp"
#include "scene/SceneDocument.hpp"
#include "scene/SceneExporter.hpp"
#include "scene/UiBinReader.hpp"
#include "ui/PropertyEditorPanel.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QImage>
#include <QSize>
#include <QLabel>
#include <QMetaClassInfo>
#include <QMetaProperty>
#include <QPainter>
#include <QSet>
#include <QToolButton>
#include <QTreeWidget>

#include <functional>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cstdio>

namespace
{
    // Every component that renders user-authored text. Anything that draws
    // glyphs an author typed belongs here; the placeholder strings some
    // components stamp out ("ICO", "F0") deliberately do not.
    const char* const kTextComponents[] = {
        "Text", "Button", "TextInput", "Dropdown",
        "Tooltip", "TabContainer", "Toggle", "ListRepeater",
    };

    QImage RenderAt(Component* comp, const QSize& size, const QPointF& offset)
    {
        comp->setProperty("textOffset", offset);

        QImage img(size, QImage::Format_ARGB32);
        img.fill(qRgb(3, 5, 7));

        QPainter p(&img);
        comp->Paint(&p, QRectF(QPointF(0, 0), QSizeF(size)), false);

        return img;
    }

    // The bounding box of the pixels that CHANGE between two offsets.
    //
    // Measuring absolute ink does not work here: most of these components
    // paint a full-bleed background, so "everything that is not the
    // background colour" is the whole rect no matter where the glyphs are.
    // The difference between two renders, though, is exactly the glyphs that
    // moved.
    //
    // If glyph ink spans rows [t, b] at offset 0 and [t+dy, b+dy] at offset
    // dy, the rows that differ span [t, b+dy]. So the TOP of this box is
    // pinned at the original glyph top while the BOTTOM grows with dy - which
    // is a direction-sensitive signal, unlike "the images differ".
    QRect DiffBounds(Component* comp, const QSize& size, double dyA, double dyB)
    {
        const QImage a = RenderAt(comp, size, QPointF(0.0, dyA));
        const QImage b = RenderAt(comp, size, QPointF(0.0, dyB));

        QRect bounds;

        for (int y = 0; y < a.height(); ++y)
        {
            for (int x = 0; x < a.width(); ++x)
            {
                if (a.pixel(x, y) != b.pixel(x, y))
                    bounds = bounds.isNull() ? QRect(x, y, 1, 1) : bounds.united(QRect(x, y, 1, 1));
            }
        }

        return bounds;
    }

    int CountRows(QFormLayout* form)
    {
        return form ? form->rowCount() : 0;
    }
}

void CheckTextOffset()
{
    std::fprintf(stderr, "text offset\n");

    PixelModel::SetMode(PixelModel::Mode::Continuous);

    // Every text-rendering component declares it, writable, as a POINT.
    for (const char* kind : kTextComponents)
    {
        Component* comp = Component::Create(QString::fromLatin1(kind), nullptr);

        if (!comp)
        {
            check(false, "component kind is registered");
            std::fprintf(stderr, "      unknown kind: %s\n", kind);
            continue;
        }

        const QMetaObject* mo = comp->metaObject();
        const int idx = mo->indexOfProperty("textOffset");

        const bool declared = idx >= mo->propertyOffset();
        check(declared, qPrintable(QStringLiteral("%1 declares textOffset on itself").arg(kind)));

        if (declared)
        {
            const QMetaProperty prop = mo->property(idx);

            check(prop.isWritable() && prop.isReadable() && prop.metaType().id() == QMetaType::QPointF,
                  qPrintable(QStringLiteral("%1.textOffset is a readable, writable POINT").arg(kind)));

            // A default of anything but (0,0) would silently move existing
            // scenes' text the moment they were reopened.
            check(comp->property("textOffset").toPointF() == QPointF(0.0, 0.0),
                  qPrintable(QStringLiteral("%1.textOffset defaults to (0, 0)").arg(kind)));

            // Round trip through scene.json.
            comp->setProperty("textOffset", QPointF(7.0, -3.0));

            QJsonObject json;
            comp->ToJson(json);

            Component* fresh = Component::Create(QString::fromLatin1(kind), nullptr);
            fresh->FromJson(json);

            check(fresh->property("textOffset").toPointF() == QPointF(7.0, -3.0),
                  qPrintable(QStringLiteral("%1.textOffset survives a scene.json round trip").arg(kind)));

            delete fresh;
        }

        delete comp;
    }

    // The offset reaches the GLYPHS. Without this the property could be
    // declared, serialised and edited while the paint path ignored it.
    for (const char* kind : kTextComponents)
    {
        Component* comp = Component::Create(QString::fromLatin1(kind), nullptr);

        if (!comp)
            continue;

        // Give every kind some text under whichever property it uses.
        for (const char* nameProp : { "text", "tooltipText", "label", "tabNames", "labels", "options", "placeholder" })
        {
            if (comp->metaObject()->indexOfProperty(nameProp) >= 0)
                comp->setProperty(nameProp, QStringLiteral("Wg"));
        }

        const QSize size(220, 90);

        const QRect small = DiffBounds(comp, size, 0.0, 4.0);
        const QRect large = DiffBounds(comp, size, 0.0, 12.0);

        const bool reached = !small.isNull();

        check(reached, qPrintable(QStringLiteral("%1's paint path reads textOffset at all").arg(kind)));

        // Same glyph top, lower bottom: the text moved DOWN, it did not just
        // change somehow.
        const bool movedDown = reached
                            && !large.isNull()
                            && large.top() == small.top()
                            && large.bottom() > small.bottom();

        check(movedDown, qPrintable(QStringLiteral("%1 draws its text lower as textOffset.y grows").arg(kind)));

        if (!movedDown)
            std::fprintf(stderr, "      dy=4 diff rows %d..%d, dy=12 diff rows %d..%d\n",
                         small.top(), small.bottom(), large.top(), large.bottom());

        delete comp;
    }

    // In PixelGrid mode a fractional nudge must land on the grid, because a
    // half-pixel offset is the exact thing that mode exists to prevent.
    {
        PixelModel::SetMode(PixelModel::Mode::PixelGrid);
        PixelModel::SetUnit(4.0);

        Component* comp = Component::Create(QStringLiteral("Text"), nullptr);
        comp->setProperty("text", QStringLiteral("Wg"));

        const QSize size(220, 90);

        // 8.0 and 9.4 both snap to 8 at a unit of 4, so they must render
        // identically. Without the snap, 9.4 would land a pixel lower.
        check(RenderAt(comp, size, QPointF(0.0, 8.0)) == RenderAt(comp, size, QPointF(0.0, 9.4)),
              "in PixelGrid mode a fractional offset snaps to the virtual-pixel grid");

        check(RenderAt(comp, size, QPointF(0.0, 8.0)) != RenderAt(comp, size, QPointF(0.0, 12.0)),
              "but a whole-unit change still moves it");

        // Continuous mode must NOT snap, or sub-pixel typography would be
        // impossible in the mode that exists for it.
        PixelModel::SetMode(PixelModel::Mode::Continuous);

        check(RenderAt(comp, size, QPointF(0.0, 8.0)) != RenderAt(comp, size, QPointF(0.0, 9.4)),
              "in Continuous mode the offset is applied exactly, not snapped");

        delete comp;
    }

    PixelModel::SetMode(PixelModel::Mode::Continuous);

    // Through the real bake, not just scene.json: the writer picks the field
    // up from the property walk and encodes it as TAG_POINT, and the reader
    // has to put it back on the right component.
    {
        QTemporaryDir tmp;

        SceneDocument doc;
        UiElement* label = doc.CreateTextElement("Label", nullptr);
        UiElement* button = doc.CreateButtonElement("Btn", nullptr);

        for (Component* c : label->GetComponents())
            if (c->GetTypeName() == QLatin1String("Text"))
                c->setProperty("textOffset", QPointF(-2.0, 5.0));

        for (Component* c : button->GetComponents())
            if (c->GetTypeName() == QLatin1String("Button"))
                c->setProperty("textOffset", QPointF(3.0, -1.5));

        Settle();

        const QString out = QDir(tmp.path()).filePath("offsets.uibin");

        check(SceneExporter::BakeToUiBin(&doc, out), "a scene with text offsets bakes");

        UiElement* root = nullptr;
        {
            QFile f(out);
            f.open(QIODevice::ReadOnly);
            root = UiBinReader::Read(f.readAll());
        }

        check(root != nullptr, "and decodes");

        if (root)
        {
            auto offsetOf = [](UiElement* e, const char* kind) -> QPointF
            {
                if (!e)
                    return QPointF(-999, -999);

                for (Component* c : e->GetComponents())
                    if (c->GetTypeName() == QLatin1String(kind))
                        return c->property("textOffset").toPointF();

                return QPointF(-999, -999);
            };

            const QPointF a = offsetOf(root->ChildElementAt(0), "Text");
            const QPointF b = offsetOf(root->ChildElementAt(1), "Button");

            check(a == QPointF(-2.0, 5.0), "the Text element's offset survives the bake");
            check(b == QPointF(3.0, -1.5), "and the Button's separate offset survives alongside it");

            if (a != QPointF(-2.0, 5.0) || b != QPointF(3.0, -1.5))
                std::fprintf(stderr, "      text=(%.2f, %.2f) button=(%.2f, %.2f)\n", a.x(), a.y(), b.x(), b.y());

            delete root;
        }
    }
}

void CheckPropertyGroups()
{
    std::fprintf(stderr, "inspector property groups\n");

    static const QLatin1String prefix("propertyGroup/");

    int groupsSeen = 0;

    // Walk every registered component, not a hand-written list, so a group
    // added to a new component is covered the day it is written.
    const QStringList kinds = Component::Registry().keys();

    for (const QString& kind : kinds)
    {
        Component* comp = Component::Create(kind, nullptr);

        if (!comp)
            continue;

        const QMetaObject* mo = comp->metaObject();

        QSet<QByteArray> claimed;

        for (int i = mo->classInfoOffset(); i < mo->classInfoCount(); ++i)
        {
            const QMetaClassInfo info = mo->classInfo(i);
            const QString key = QString::fromLatin1(info.name());

            if (!key.startsWith(prefix))
                continue;

            ++groupsSeen;

            const QString label = key.mid(prefix.size());
            const QStringList entries = QString::fromLatin1(info.value()).split(QLatin1Char(','), Qt::SkipEmptyParts);

            check(!entries.isEmpty(), qPrintable(QStringLiteral("%1/%2 lists at least one property").arg(kind, label)));

            for (const QString& entry : entries)
            {
                const int eq = entry.indexOf(QLatin1Char('='));

                check(eq > 0, qPrintable(QStringLiteral("%1/%2 entry \"%3\" is prop=leafLabel").arg(kind, label, entry)));

                if (eq <= 0)
                    continue;

                const QByteArray prop = entry.left(eq).trimmed().toLatin1();

                // The silent failure this exists for: a typo here does not
                // break the build, it just leaves the property sitting at the
                // top level while the group shows one fewer row than intended.
                const int pi = mo->indexOfProperty(prop.constData());

                check(pi >= mo->propertyOffset(),
                      qPrintable(QStringLiteral("%1/%2 names a real property: %3").arg(kind, label, QString::fromLatin1(prop))));

                // Listed twice, it would render twice.
                check(!claimed.contains(prop),
                      qPrintable(QStringLiteral("%1.%2 belongs to exactly one group").arg(kind, QString::fromLatin1(prop))));

                claimed.insert(prop);
            }
        }

        delete comp;
    }

    check(groupsSeen > 0, "at least one component declares property groups");
    std::fprintf(stderr, "      %d groups across %lld component kinds\n", groupsSeen, (long long)kinds.size());

    // The invariant that matters in the panel: grouping RELOCATES properties,
    // it never drops them. Walk what the tree actually built.
    {
        SceneDocument doc;
        UiElement* image = doc.CreateImageElement("Image", nullptr);
        Settle();

        PropertyEditorPanel panel;
        panel.SetTarget(image);
        Settle();

        Component* img = nullptr;
        for (Component* c : image->GetComponents())
            if (c->GetTypeName() == QLatin1String("Image"))
                img = c;

        check(img != nullptr, "the image element has an Image component");

        QTreeWidget* tree = panel.findChild<QTreeWidget*>();

        check(tree != nullptr, "the panel is built on a QTreeWidget, not hand-rolled disclosure widgets");
        check(panel.findChildren<QToolButton*>().isEmpty(),
              "and draws no QToolButton arrows of its own - the tree draws native branch indicators");

        if (img && tree)
        {
            const QMetaObject* mo = img->metaObject();

            int editable = 0;
            for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
                if (mo->property(i).isWritable() && mo->property(i).isReadable())
                    ++editable;

            QTreeWidgetItem* node = nullptr;
            for (int i = 0; i < tree->topLevelItemCount(); ++i)
                if (tree->topLevelItem(i)->text(0) == QLatin1String("Image"))
                    node = tree->topLevelItem(i);

            check(node != nullptr, "the Image component is a top-level node");

            if (node)
            {
                check(node->isExpanded(), "a component node starts expanded");

                int groups = 0;
                int leaves = 0;
                int withEditor = 0;

                std::function<void(QTreeWidgetItem*)> walk = [&](QTreeWidgetItem* parent)
                {
                    for (int i = 0; i < parent->childCount(); ++i)
                    {
                        QTreeWidgetItem* child = parent->child(i);

                        if (child->childCount() > 0)
                        {
                            ++groups;

                            check(!child->isExpanded(),
                                  qPrintable(QStringLiteral("group \"%1\" starts collapsed").arg(child->text(0))));

                            walk(child);
                            continue;
                        }

                        ++leaves;

                        if (tree->itemWidget(child, 1))
                            ++withEditor;
                    }
                };

                walk(node);

                check(groups == 3, "Image shows three group nodes");
                check(leaves == editable, "every editable property is a leaf exactly once");
                check(withEditor == leaves, "and every leaf carries its editor widget in the value column");

                std::fprintf(stderr, "      %d editable properties -> %d direct children (%d groups), %d leaves\n",
                             editable, node->childCount(), groups, leaves);

                check(node->childCount() < editable, "so the component node is shorter than the flat list was");

            }
        }
    }
}
