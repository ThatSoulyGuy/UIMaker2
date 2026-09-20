#include "ui/PropertyEditorPanel.hpp"
#include "core/AssetContext.hpp"
#include "core/UiElement.hpp"
#include "core/Component.hpp"
#include "core/Anchor.hpp"
#include <QMetaProperty>
#include <QMessageBox>
#include <QApplication>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QAbstractSpinBox>
#include <QPushButton>
#include <QColor>
#include <QColorDialog>
#include <QFileDialog>
#include <QPointer>
#include <QTimer>
#include <QFile>
#include <QSet>
#include <QAbstractButton>
#include <QScrollBar>

#include <limits>

static Component* ComponentOfKind(UiElement* el, const QString& kind)
{
    if (!el)
        return nullptr;

    for (Component* c : el->GetComponents())
    {
        if (c->GetTypeName() == kind)
            return c;
    }

    return nullptr;
}

// True if the named property has the same value across the same-kind component of every
// selected element. A property that is uniform needs no "mixed" annotation.
static bool PropertyIsUniform(const QList<UiElement*>& targets, const QString& kind, const char* propName)
{
    bool haveFirst = false;
    QVariant first;

    for (UiElement* el : targets)
    {
        Component* c = ComponentOfKind(el, kind);
        if (!c)
            continue;

        const QVariant v = c->property(propName);

        if (!haveFirst)
        {
            first = v;
            haveFirst = true;
        }
        else if (v != first)
        {
            return false;
        }
    }

    return true;
}

PropertyEditorPanel::PropertyEditorPanel(QWidget* parent) : QWidget(parent), target(nullptr)
{
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    container = new QWidget(scrollArea);
    layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignTop);
    container->setLayout(layout);
    scrollArea->setWidget(container);

    scrollArea->setFrameShape(QFrame::NoFrame);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->setContentsMargins(6, 6, 6, 6);

    auto* outer = new QVBoxLayout();

    outer->setContentsMargins(4, 4, 4, 4);
    outer->addWidget(scrollArea);

    setLayout(outer);

    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget* now)
    {
        const bool focusInsidePanel = (now && this->isAncestorOf(now));

        if (!focusInsidePanel && pendingRebuild && !suppressRebuild)
        {
            pendingRebuild = false;

            QTimer::singleShot(0, this, [this](){ Rebuild(); });
        }
    });
}

void PropertyEditorPanel::SetTarget(UiElement* element)
{
    SetTargets(element ? QList<UiElement*>{ element } : QList<UiElement*>{});
}

void PropertyEditorPanel::SetTargets(const QList<UiElement*>& elements)
{
    // Disconnect from all prior targets.
    for (UiElement* prior : targets)
    {
        if (!prior)
            continue;

        QObject::disconnect(prior, nullptr, this, nullptr);

        for (Component* comp : prior->GetComponents())
            QObject::disconnect(comp, nullptr, this, nullptr);
    }

    targets = elements;
    target = targets.isEmpty() ? nullptr : targets.last();

    for (UiElement* el : targets)
    {
        if (!el)
            continue;

        QObject::connect(el, &UiElement::ComponentListChanged, this, [this](UiElement*) { Rebuild(); });
        QObject::connect(el, &QObject::destroyed, this, [this, el]()
        {
            targets.removeAll(el);
            if (target == el)
                target = targets.isEmpty() ? nullptr : targets.last();
            pendingRebuild = false;
            Rebuild();
        });
    }

    Rebuild();
}

void PropertyEditorPanel::RefreshTargets()
{
    pendingRebuild = false;
    Rebuild();
}

namespace
{
    // QObject::property() hands back an enum/QFlags property as its declared type,
    // while the panel's anchors/alignment/stretch editors write a plain int. In Qt6
    // QVariant equality across two different metatypes is unconditionally false, so
    // a straight `before != value` reported every such edit as a change and pushed a
    // no-op command - re-picking the anchors you already had filled the undo stack.
    bool SamePropertyValue(const QVariant& before, const QVariant& after)
    {
        if (before.metaType() == after.metaType())
            return before == after;

        const bool enumLike = (before.metaType().flags() & QMetaType::IsEnumeration)
                           || (after.metaType().flags()  & QMetaType::IsEnumeration);

        if (!enumLike)
            return before == after;

        bool okBefore = false;
        bool okAfter  = false;
        const int a = before.toInt(&okBefore);
        const int b = after.toInt(&okAfter);

        return okBefore && okAfter && a == b;
    }
}

void PropertyEditorPanel::ApplyPropertyChange(QObject* primary, const QByteArray& propName, const QVariant& value)
{
    if (!primary)
        return;

    // Record each object's prior value alongside the write so MainWindow can
    // build an undo command out of real changes only.
    QList<PropertyEditRecord> records;

    auto recordAndSet = [&](QObject* obj, const QUuid& elementId, const QString& kind)
    {
        const QVariant before = obj->property(propName.constData());
        obj->setProperty(propName.constData(), value);

        if (!elementId.isNull() && !SamePropertyValue(before, value))
            records.append({ elementId, kind, propName, before, value });
    };

    auto* primaryComp = qobject_cast<Component*>(primary);

    if (!primaryComp)
    {
        auto* el = qobject_cast<UiElement*>(primary);
        recordAndSet(primary, el ? el->GetId() : QUuid(), QString());

        if (!records.isEmpty())
            emit PropertyChangeApplied(records);

        return;
    }

    auto* primaryOwner = qobject_cast<UiElement*>(primaryComp->parent());
    const QString kind = primaryComp->GetTypeName();

    recordAndSet(primary, primaryOwner ? primaryOwner->GetId() : QUuid(), kind);

    // Broadcast to every other selected element's same-kind component, so a property
    // edit in the panel updates the whole multiselection in lock-step.
    for (UiElement* el : targets)
    {
        if (!el || el == primaryOwner)
            continue;

        for (Component* otherComp : el->GetComponents())
        {
            if (otherComp == primaryComp)
                continue;

            if (otherComp->GetTypeName() == kind)
            {
                recordAndSet(otherComp, el->GetId(), kind);
                break;
            }
        }
    }

    if (!records.isEmpty())
        emit PropertyChangeApplied(records);
}

void PropertyEditorPanel::SetLive(bool on)
{
    if (live == on)
        return;

    live = on;

    // Anything that arrived while suspended is folded into one rebuild here.
    if (live && pendingRebuild)
    {
        pendingRebuild = false;
        Rebuild();
    }
}

void PropertyEditorPanel::OnComponentChanged()
{
    if (suppressRebuild)
        return;

    // A viewport gesture is in flight; hold still and catch up on mouse-up.
    if (!live)
    {
        pendingRebuild = true;
        return;
    }

    QWidget* fw = QApplication::focusWidget();

    // QAbstractButton covers the checkbox editors and the colour button: with
    // focus on one of those, a queued rebuild would deleteLater() the very widget
    // being clicked, out from under the click.
    const bool editingInPanel = fw && (this->isAncestorOf(fw))
        && (qobject_cast<QLineEdit*>(fw)
         || qobject_cast<QAbstractSpinBox*>(fw)
         || qobject_cast<QComboBox*>(fw)
         || qobject_cast<QAbstractButton*>(fw));

    if (editingInPanel)
    {
        pendingRebuild = true;
        return;
    }

    // Coalesce: ComponentChanged is queued and arrives once per property write, so
    // a multi-select edit or a drag frame produced one full panel rebuild each.
    if (rebuildQueued)
        return;

    rebuildQueued = true;
    QTimer::singleShot(0, this, [this](){ Rebuild(); });
}

QWidget* PropertyEditorPanel::EditorForProperty(QObject* object, const QMetaProperty& prop, bool mixed)
{
    const QString name = QString::fromLatin1(prop.name());


#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    auto isType = [&](auto t) { return prop.metaType() == QMetaType::fromType<decltype(t)>(); };
#else
    auto isType = [&](int meta) { return prop.type() == meta; };
#endif

    if (name == "direction")
    {
        auto* combo = new QComboBox();
        combo->addItem("Vertical");
        combo->addItem("Horizontal");
        combo->setCurrentIndex(object->property(prop.name()).toInt());

        QPointer<QObject> obj = object;
        QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, obj, prop](int index)
        {
            if (!obj)
                return;
            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(),index);
            suppressRebuild = false;
            emit PropertyEdited();
        });

        return combo;
    }

    if (name == "shape")
    {
        auto* combo = new QComboBox();
        combo->addItem("Rectangle");
        combo->addItem("Circle");
        combo->setCurrentIndex(object->property(prop.name()).toInt());

        QPointer<QObject> obj = object;
        QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, obj, prop](int index)
        {
            if (!obj)
                return;
            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(),index);
            suppressRebuild = false;
            emit PropertyEdited();
        });

        return combo;
    }

    // Pixel-perfect texture knobs. Dispatched by NAME for the same reason
    // "shape" and "direction" are: a Q_ENUM declared in a holder class reports
    // isEnumType() == false, so the generic enum branch below never fires and
    // the property would render as a dead read-only label.
    if (name == "textureFill" || name == "cropAnchor")
    {
        auto* combo = new QComboBox();

        if (name == "textureFill")
        {
            combo->addItem("Stretch");            // PixelDraw::FillStretch
            combo->addItem("Wrap (pixel-exact)"); // PixelDraw::FillWrap
        }
        else
        {
            // Order matches PixelDraw::Anchor exactly: 0..8 reading rows.
            combo->addItem("Top Left");    combo->addItem("Top");    combo->addItem("Top Right");
            combo->addItem("Left");        combo->addItem("Center"); combo->addItem("Right");
            combo->addItem("Bottom Left"); combo->addItem("Bottom"); combo->addItem("Bottom Right");
        }

        combo->setCurrentIndex(object->property(prop.name()).toInt());

        QPointer<QObject> obj = object;
        QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, obj, prop](int index)
        {
            if (!obj)
                return;
            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(), index);
            suppressRebuild = false;
            emit PropertyEdited();
        });

        return combo;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (isType(bool{}))
#else
    if (prop.type() == QVariant::Bool)
#endif
    {
        auto* cb = new QCheckBox();
        if (mixed)
        {
            cb->setTristate(true);
            cb->setCheckState(Qt::PartiallyChecked);
            cb->setText(QStringLiteral("mixed"));
        }
        else
        {
            cb->setChecked(object->property(prop.name()).toBool());
        }

        QPointer<QObject> obj = object;
        QObject::connect(cb, &QCheckBox::toggled, this, [this, obj, prop](bool checked)
        {
            if (!obj)
                return;
            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(),checked);
            suppressRebuild = false;
            emit PropertyEdited();
        });

        return cb;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (isType(double{}))
#else
    if (isType(QMetaType::Double))
#endif
    {
        auto* box = new QDoubleSpinBox();

        box->setDecimals(3);

        // Commit on editingFinished/stepping only. With tracking on, clearing "100"
        // and typing "250" applied 2, then 25, then 250 - three full relayouts and
        // three undo records for one edit.
        box->setKeyboardTracking(false);

        if (mixed)
        {
            // Park at an out-of-range sentinel so the box shows "mixed" with no number.
            // Typing any real value moves off the sentinel and commits normally.
            box->setRange(-100001.0, 100000.0);
            box->setSpecialValueText(QStringLiteral("mixed"));
            box->setValue(box->minimum());
        }
        else
        {
            box->setRange(-100000.0, 100000.0);
            box->setValue(object->property(prop.name()).toDouble());
        }

        QPointer<QObject> obj = object;
        QObject::connect(box, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, obj, prop](double v)
        {
            if (!obj)
                return;
            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(),v);
            suppressRebuild = false;
            emit PropertyEdited();
        });

        return box;
    }

    if (
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        isType(int{})
#else
        isType(QMetaType::Int)
#endif
        )
    {
        auto* box = new QSpinBox();

        box->setKeyboardTracking(false);

        // Full int range. The old 0..4096 clamp applied to EVERY int property in
        // the app, so a legitimately negative value could be displayed but never
        // typed back - RadialMenuComponent::highlightIndex defaults to -1 ("no
        // highlight"), showed as 0, and was unrecoverable once touched.
        if (mixed)
        {
            box->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            box->setSpecialValueText(QStringLiteral("mixed"));
            box->setValue(box->minimum());
        }
        else
        {
            box->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            box->setValue(object->property(prop.name()).toInt());
        }

        QPointer<QObject> obj = object;

        QObject::connect(box, qOverload<int>(&QSpinBox::valueChanged), this, [this, obj, prop](int v)
        {
            if (!obj)
                return;

            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(),v);
            suppressRebuild = false;

            emit PropertyEdited();
        });

        return box;
    }

    if (name.endsWith("Path", Qt::CaseInsensitive))
    {
        auto* row = new QWidget();
        auto* h = new QHBoxLayout();
        h->setContentsMargins(0,0,0,0);

        auto* edit = new QLineEdit(mixed ? QString() : object->property(prop.name()).toString());
        if (mixed)
        {
            edit->setPlaceholderText(QStringLiteral("mixed"));
            edit->setProperty("mixedUntouched", true);
            QObject::connect(edit, &QLineEdit::textEdited, edit, [edit](const QString&)
            {
                edit->setProperty("mixedUntouched", false);
            });
        }
        auto* browse = new QPushButton("...");
        browse->setFixedWidth(30);

        QPointer<QObject> obj = object;
        // edit/browse are recreated whenever the panel rebuilds. The modal file
        // dialogs below spin the event loop, so a refresh can free this row
        // mid-callback; track the line edit through a QPointer and never touch
        // it (directly or via markValidity) once it has died.
        QPointer<QLineEdit> editp = edit;

        // Paths are stored relative to scene.json. Flag anything absolute,
        // escaping the project root via "..", or pointing at a file that does
        // not resolve under the current project root.
        auto markValidity = [](QLineEdit* e, const QString& v)
        {
            if (!e)
                return;

            const bool bad = !v.isEmpty()
                             && (!AssetContext::IsValidRelative(v)
                                 || !QFile::exists(AssetContext::Resolve(v)));

            e->setStyleSheet(bad ? "QLineEdit { border: 1px solid #cc4444; }" : QString());
        };

        markValidity(edit, mixed ? QString() : object->property(prop.name()).toString());

        QObject::connect(edit, &QLineEdit::editingFinished, this, [this, editp, obj, prop, markValidity]()
        {
            if (!obj || !editp)
                return;

            // Don't broadcast an empty value to the whole selection just because the
            // user tabbed through an untouched "mixed" path field.
            if (editp->property("mixedUntouched").toBool() && editp->text().isEmpty())
                return;

            QString v = editp->text().trimmed();
            markValidity(editp, v);

            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(),v);
            suppressRebuild = false;

            emit PropertyEdited();
        });

        QObject::connect(browse, &QPushButton::clicked, this, [this, editp, obj, prop, name, markValidity]()
        {
            QString filter;
            QString title;

            if (name.contains("font", Qt::CaseInsensitive))
            {
                filter = "Fonts (*.ttf *.otf)";
                title = "Choose Font";
            }
            else
            {
                filter = "Images (*.png *.jpg *.jpeg *.bmp *.svg)";
                title = "Choose Image";
            }

            const QString src = QFileDialog::getOpenFileName(nullptr, title, QString(), filter);

            if (src.isEmpty() || !obj)
                return;

            // Relative paths need a project root (the folder that will hold
            // scene.json + assets/). Establish one on first use.
            if (!AssetContext::HasBaseDir())
            {
                const QString root = QFileDialog::getExistingDirectory(
                    nullptr, "Choose Project Root (folder for scene.json + assets)");

                if (root.isEmpty())
                    return;

                AssetContext::SetBaseDir(root);
                emit ProjectRootChanged(root);
            }

            const QString rel = AssetContext::ImportToAssets(src);

            if (rel.isEmpty())
            {
                QMessageBox::warning(this, "Asset Import Failed",
                    "Could not copy the selected file into the project's assets/ folder.");
                return;
            }

            // The row may have been rebuilt while the dialog(s) were open; only
            // touch the line edit if it is still alive. The property change is
            // still applied via the (separately tracked) target object.
            if (editp)
            {
                editp->setText(rel);
                markValidity(editp, rel);
            }

            if (!obj)
                return;

            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(), rel);
            suppressRebuild = false;

            emit PropertyEdited();
        });

        h->addWidget(edit);
        h->addWidget(browse);
        row->setLayout(h);

        return row;
    }

    if (
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        isType(QString{})
#else
        isType(QMetaType::QString)
#endif
        )
    {
        auto* edit = new QLineEdit(mixed ? QString() : object->property(prop.name()).toString());
        if (mixed)
            edit->setPlaceholderText(QStringLiteral("mixed"));

        QPointer<QObject> obj = object;
        QPointer<QLineEdit> editp = edit;

        // editingFinished, not textEdited: this matches the name field and the
        // *Path editor, and it matters most for a TabContainer's tabNames, where
        // every keystroke used to run SceneDocument::EnsureSlots - creating and
        // destroying whole slot subtrees and resetting the tree model per character.
        // PropertyEditCommand::mergeWith still collapses the edit into one undo step.
        QObject::connect(edit, &QLineEdit::editingFinished, this, [this, obj, prop, editp]()
        {
            if (!obj || !editp)
                return;
            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(), editp->text());
            suppressRebuild = false;
            emit PropertyEdited();
        });

        return edit;
    }

    if (name == "anchors" || name == "alignment")
    {
        auto* row = new QWidget();
        auto* h = new QHBoxLayout();

        h->setContentsMargins(0,0,0,0);

        auto* labelH = new QLabel("H");
        auto* comboH = new QComboBox();
        comboH->addItem("Left");
        comboH->addItem("Center");
        comboH->addItem("Right");

        auto* labelV = new QLabel("V");
        auto* comboV = new QComboBox();

        comboV->addItem("Top");
        comboV->addItem("Center");
        comboV->addItem("Bottom");

        int flags = object->property(prop.name()).toInt();

        int selH = 0;

        if (flags & static_cast<int>(Anchor::CENTER_X))
            selH = 1;
        else if (flags & static_cast<int>(Anchor::RIGHT))
            selH = 2;
        else
            selH = 0;

        comboH->setCurrentIndex(selH);

        int selV = 0;

        if (flags & static_cast<int>(Anchor::CENTER_Y))
            selV = 1;
        else if (flags & static_cast<int>(Anchor::BOTTOM))
            selV = 2;
        else
            selV = 0;

        comboV->setCurrentIndex(selV);

        QPointer<QObject> obj = object;
        QObject::connect(comboH, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, obj, prop, comboV](int index)
        {
            if (!obj)
                return;
            suppressRebuild = true;

            int v = obj->property(prop.name()).toInt();

            v &= ~(static_cast<int>(Anchor::LEFT) | static_cast<int>(Anchor::RIGHT) | static_cast<int>(Anchor::CENTER_X));

            if (index == 0)
                v |= static_cast<int>(Anchor::LEFT);
            else if (index == 1)
                v |= static_cast<int>(Anchor::CENTER_X);
            else if (index == 2)
                v |= static_cast<int>(Anchor::RIGHT);

            ApplyPropertyChange(obj, prop.name(),v);
            suppressRebuild = false;
            emit PropertyEdited();
        });

        QObject::connect(comboV, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, obj, prop, comboH](int index)
        {
            if (!obj)
                return;
            suppressRebuild = true;

            int v = obj->property(prop.name()).toInt();

            v &= ~(static_cast<int>(Anchor::TOP) | static_cast<int>(Anchor::BOTTOM) | static_cast<int>(Anchor::CENTER_Y));

            if (index == 0)
                v |= static_cast<int>(Anchor::TOP);
            else if (index == 1)
                v |= static_cast<int>(Anchor::CENTER_Y);
            else if (index == 2)
                v |= static_cast<int>(Anchor::BOTTOM);

            ApplyPropertyChange(obj, prop.name(),v);
            suppressRebuild = false;
            emit PropertyEdited();
        });

        h->addWidget(labelH);
        h->addWidget(comboH);
        h->addSpacing(8);
        h->addWidget(labelV);
        h->addWidget(comboV);
        h->addStretch(1);
        row->setLayout(h);

        return row;
    }

    if (name == "stretch")
    {
        auto* combo = new QComboBox();

        combo->addItem("None");
        combo->addItem("Horizontal");
        combo->addItem("Vertical");
        combo->addItem("Both");

        int flags = object->property(prop.name()).toInt();

        bool horizontal = (flags & static_cast<int>(Anchor::LEFT)) && (flags & static_cast<int>(Anchor::RIGHT));
        bool vertical = (flags & static_cast<int>(Anchor::TOP)) && (flags & static_cast<int>(Anchor::BOTTOM));

        int sel = 0;

        if (horizontal && vertical)
            sel = 3;
        else if (horizontal)
            sel = 1;
        else if (vertical)
            sel = 2;
        else
            sel = 0;

        combo->setCurrentIndex(sel);

        QPointer<QObject> obj = object;
        QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, obj, prop](int index)
        {
            if (!obj)
                return;

            suppressRebuild = true;

            int v = 0;

            if (index == 1 || index == 3)
            {
                v |= static_cast<int>(Anchor::LEFT);
                v |= static_cast<int>(Anchor::RIGHT);
            }

            if (index == 2 || index == 3)
            {
                v |= static_cast<int>(Anchor::TOP);
                v |= static_cast<int>(Anchor::BOTTOM);
            }

            ApplyPropertyChange(obj, prop.name(),v);
            suppressRebuild = false;

            emit PropertyEdited();
        });

        return combo;
    }

    if (prop.isEnumType() && !prop.enumerator().isFlag())
    {
        auto* combo = new QComboBox();
        QMetaEnum e = prop.enumerator();
        int curr = object->property(prop.name()).toInt();
        int selectedIndex = 0;

        for (int i = 0; i < e.keyCount(); ++i)
        {
            combo->addItem(QString::fromLatin1(e.key(i)));
            if (e.value(i) == curr)
                selectedIndex = i;
        }

        combo->setCurrentIndex(selectedIndex);

        QPointer<QObject> obj = object;
        QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, obj, prop, e](int index)
        {
            if (!obj || index < 0 || index >= e.keyCount())
                return;
            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(),e.value(index));
            suppressRebuild = false;
            emit PropertyEdited();
        });

        return combo;
    }

    if (prop.isEnumType() && prop.enumerator().isFlag())
    {
        auto* row = new QWidget();
        auto* h = new QHBoxLayout();

        h->setContentsMargins(0,0,0,0);

        QMetaEnum e = prop.enumerator();
        int curr = object->property(prop.name()).toInt();

        for (int i = 0; i < e.keyCount(); ++i)
        {
            int value = e.value(i);

            if (value == 0)
                continue;

            auto* cb = new QCheckBox(QString::fromLatin1(e.key(i)));
            cb->setChecked(curr & value);

            QPointer<QObject> obj = object;

            QObject::connect(cb, &QCheckBox::toggled, this, [this, obj, prop, value](bool checked)
            {
                if (!obj)
                    return;
                suppressRebuild = true;

                int v = obj->property(prop.name()).toInt();

                if (checked)
                    v |= value;
                else
                    v &= ~value;

                ApplyPropertyChange(obj, prop.name(),v);
                suppressRebuild = false;

                emit PropertyEdited();
            });

            h->addWidget(cb);
        }

        row->setLayout(h);
        return row;
    }

    if (
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        isType(QPointF{})
#else
        isType(QMetaType::QPointF)
#endif
        )
    {
        auto* row = new QWidget();

        auto* h = new QHBoxLayout();
        h->setContentsMargins(0,0,0,0);

        auto* x = new QDoubleSpinBox();
        auto* y = new QDoubleSpinBox();

        x->setDecimals(3);
        y->setDecimals(3);

        // See the scalar double editor: commit on editingFinished, not per keystroke.
        x->setKeyboardTracking(false);
        y->setKeyboardTracking(false);

        const QPointF p = object->property(prop.name()).toPointF();

        if (mixed)
        {
            x->setRange(-100001.0, 100000.0);
            y->setRange(-100001.0, 100000.0);
            x->setSpecialValueText(QStringLiteral("mixed"));
            y->setSpecialValueText(QStringLiteral("mixed"));
            x->setValue(x->minimum());
            y->setValue(y->minimum());
        }
        else
        {
            x->setRange(-100000.0, 100000.0);
            y->setRange(-100000.0, 100000.0);
            x->setValue(p.x());
            y->setValue(p.y());
        }

        QPointer<QObject> obj = object;

        // When an axis is still parked on the "mixed" sentinel, fall back to the primary
        // element's value for that axis so editing one axis doesn't clobber the other.
        auto axisVal = [](QDoubleSpinBox* b, double fallback) -> double
        {
            return b->value() <= b->minimum() ? fallback : b->value();
        };

        QObject::connect(x, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, obj, prop, y, p, axisVal](double xv)
        {
            if (!obj)
                return;

            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(), QPointF(xv, axisVal(y, p.y())));
            suppressRebuild = false;

            emit PropertyEdited();
        });

        QObject::connect(y, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, obj, prop, x, p, axisVal](double yv)
        {
            if (!obj)
                return;

            suppressRebuild = true;
            ApplyPropertyChange(obj, prop.name(), QPointF(axisVal(x, p.x()), yv));
            suppressRebuild = false;

            emit PropertyEdited();
        });

        h->addWidget(x);
        h->addWidget(y);
        row->setLayout(h);

        return row;
    }

    if (
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        isType(QColor{})
#else
        isType(QMetaType::QColor)
#endif
        )
    {
        // Show the colour on the button, not just its hex string, so you can read a
        // swatch without opening the dialog. Chequerboard-free: a flat fill over a
        // dark panel reads alpha well enough.
        auto paintSwatch = [](QPushButton* b, const QColor& c)
        {
            b->setText(c.name(QColor::HexArgb));
            b->setStyleSheet(QStringLiteral(
                "text-align:left; padding-left:6px;"
                "background-color: rgba(%1,%2,%3,%4);"
                "color: %5;")
                .arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha())
                .arg(c.lightnessF() > 0.5 && c.alpha() > 96 ? "#111" : "#eee"));
        };

        auto* button = new QPushButton();

        if (mixed)
            button->setText(QStringLiteral("mixed"));
        else
            paintSwatch(button, object->property(prop.name()).value<QColor>());

        QPointer<QObject> obj = object;
        QPointer<QPushButton> btn = button;

        QObject::connect(button, &QPushButton::clicked, this, [this, obj, prop, btn, paintSwatch]()
        {
            if (!obj || !btn)
                return;

            suppressRebuild = true;

            // ShowAlphaChannel is not optional here: without it the dialog always
            // returns alpha 255, so opening the picker on any translucent colour and
            // pressing OK silently flattened it - ModalComponent's rgba(0,0,0,140)
            // overlay, PanelComponent's rgba(50,50,55,200), Minimap, RadialMenu,
            // DragSlot.
            QColor c = QColorDialog::getColor(obj->property(prop.name()).value<QColor>(),
                                              this,
                                              QString(),
                                              QColorDialog::ShowAlphaChannel);

            if (c.isValid() && obj)
            {
                ApplyPropertyChange(obj, prop.name(),c);

                if (btn)
                    paintSwatch(btn, c);
            }

            suppressRebuild = false;

            emit PropertyEdited();
        });

        return button;
    }

    auto* label = new QLabel(QStringLiteral("<%1>").arg(QString::fromLatin1(prop.typeName())));

    return label;
}

void PropertyEditorPanel::Rebuild()
{
    pendingRebuild = false;
    rebuildQueued  = false;

    // Every rebuild re-creates the whole widget tree, which resets the scroll bar
    // to the top - so toggling a checkbox near the bottom of a Button's property
    // list used to throw you back to the top. Restore deferred, because the new
    // layout has no geometry until the event loop has run.
    const int scrollY = scrollArea && scrollArea->verticalScrollBar()
                      ? scrollArea->verticalScrollBar()->value()
                      : 0;

    if (scrollY > 0)
    {
        QTimer::singleShot(0, this, [this, scrollY]()
        {
            if (scrollArea && scrollArea->verticalScrollBar())
                scrollArea->verticalScrollBar()->setValue(scrollY);
        });
    }

    QLayoutItem* child;

    while ((child = layout->takeAt(0)) != nullptr)
    {
        if (auto* w = child->widget())
            w->deleteLater();

        delete child;
    }

    if (!target)
        return;

    const bool isMulti = targets.size() > 1;

    if (isMulti)
    {
        auto* header = new QLabel(QStringLiteral("%1 elements selected — edits apply to all").arg(targets.size()));
        header->setWordWrap(true);
        header->setStyleSheet("QLabel { color: #6cb6ff; padding: 4px 2px; font-weight: bold; }");
        layout->addWidget(header);

        auto* nameRow = new QWidget();
        auto* nameLayout = new QHBoxLayout();
        nameLayout->setContentsMargins(0,0,0,0);

        auto* nameEdit = new QLineEdit(QStringLiteral("<multiple>"));
        nameEdit->setReadOnly(true);
        nameEdit->setStyleSheet("QLineEdit { color: #aaaaaa; font-style: italic; }");

        nameLayout->addWidget(new QLabel("Name"));
        nameLayout->addWidget(nameEdit);
        nameRow->setLayout(nameLayout);
        layout->addWidget(nameRow);

        // Only show component kinds present on EVERY selected element, so an edit
        // broadcast through ApplyPropertyChange lands on all of them.
        QStringList commonKinds;
        for (Component* comp : target->GetComponents())
        {
            const QString kind = comp->GetTypeName();
            bool inAll = true;

            for (UiElement* el : targets)
            {
                if (!ComponentOfKind(el, kind))
                {
                    inAll = false;
                    break;
                }
            }

            if (inAll)
                commonKinds.append(kind);
        }

        for (Component* comp : target->GetComponents())
        {
            const QString kind = comp->GetTypeName();
            if (!commonKinds.contains(kind))
                continue;

            auto* group = new QGroupBox(kind);
            auto* form = new QFormLayout();

            const QMetaObject* mo = comp->metaObject();

            for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
            {
                QMetaProperty prop = mo->property(i);

                if (!prop.isWritable() || !prop.isReadable())
                    continue;

                const bool mixed = !PropertyIsUniform(targets, kind, prop.name());

                QWidget* editor = EditorForProperty(comp, prop, mixed);

                QString label = QString::fromLatin1(prop.name());
                if (mixed && editor)
                    editor->setToolTip(QStringLiteral("Values differ across the selection. Editing sets all to the same value."));

                form->addRow(label, editor);
            }

            group->setLayout(form);
            layout->addWidget(group);

            for (UiElement* el : targets)
            {
                if (Component* c = ComponentOfKind(el, kind))
                    QObject::connect(c, &Component::ComponentChanged, this, &PropertyEditorPanel::OnComponentChanged, Qt::UniqueConnection);
            }
        }

        layout->addStretch(1);
        return;
    }

    if (target->IsSlot())
    {
        auto* nameRow = new QWidget();
        auto* nameLayout = new QHBoxLayout();
        nameLayout->setContentsMargins(0,0,0,0);

        auto* nameEdit = new QLineEdit(target->GetName());
        nameEdit->setReadOnly(true);
        nameEdit->setStyleSheet("QLineEdit { color: #aaaaaa; }");

        nameLayout->addWidget(new QLabel("Name"));
        nameLayout->addWidget(nameEdit);
        nameRow->setLayout(nameLayout);
        layout->addWidget(nameRow);

        auto* notice = new QLabel(QStringLiteral("Locked slot (index %1). Parent other elements to it to populate its contents.").arg(target->GetSlotIndex()));
        notice->setWordWrap(true);
        notice->setStyleSheet("QLabel { color: #aaaaaa; padding: 12px 4px; }");
        layout->addWidget(notice);
        layout->addStretch(1);

        return;
    }

    auto* nameRow = new QWidget();

    auto* nameLayout = new QHBoxLayout();

    nameLayout->setContentsMargins(0,0,0,0);

    auto* nameEdit = new QLineEdit(target->GetName());

    QObject::connect(nameEdit, &QLineEdit::editingFinished, target, [this, nameEdit]()
    {
        if (!target)
            return;

        const QString v = nameEdit->text();

        if (v.isEmpty())
        {
            nameEdit->setText(target->GetName());
            return;
        }

        // Route through ApplyPropertyChange rather than calling SetName
        // directly, so the rename produces a PropertyEditRecord with an empty
        // componentKind - the element-level case PropertyEditCommand::Apply
        // already handles - and lands on the undo stack like every other edit.
        ApplyPropertyChange(target, "name", v);
    });

    QObject::connect(target, &UiElement::NameChanged, nameEdit, [nameEdit](const QString& v)
    {
        if (nameEdit->text() != v)
            nameEdit->setText(v);
    });

    nameLayout->addWidget(new QLabel("Name"));
    nameLayout->addWidget(nameEdit);
    nameRow->setLayout(nameLayout);
    layout->addWidget(nameRow);

    for (auto* comp : target->GetComponents())
    {
        auto* group = new QGroupBox(comp->GetTypeName());

        auto* form = new QFormLayout();

        const QMetaObject* mo = comp->metaObject();

        for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
        {
            QMetaProperty prop = mo->property(i);

            if (!prop.isWritable() || !prop.isReadable())
                continue;

            QWidget* editor = EditorForProperty(comp, prop);

            form->addRow(QString::fromLatin1(prop.name()), editor);
        }

        group->setLayout(form);
        layout->addWidget(group);

        QObject::connect(comp, &Component::ComponentChanged, this, &PropertyEditorPanel::OnComponentChanged, Qt::UniqueConnection);
    }

    layout->addStretch(1);
}
