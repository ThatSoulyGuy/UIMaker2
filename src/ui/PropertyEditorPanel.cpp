#include "ui/PropertyEditorPanel.hpp"
#include <QMetaProperty>
#include <QApplication>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPointer>
#include <QTimer>
#include <QFile>

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
    if (target)
    {
        QObject::disconnect(target, nullptr, this, nullptr);

        for (Component* comp : target->GetComponents())
            QObject::disconnect(comp, nullptr, this, nullptr);
    }

    target = element;

    if (target)
    {
        QObject::connect(target, &UiElement::ComponentListChanged, this, [this](UiElement*) { Rebuild(); });
        QObject::connect(target, &QObject::destroyed, this, [this]()
        {
            target = nullptr;
            pendingRebuild = false;
            Rebuild();
        });
    }

    Rebuild();
}

void PropertyEditorPanel::OnComponentChanged()
{
    if (suppressRebuild)
        return;

    QWidget* fw = QApplication::focusWidget();

    const bool editingInPanel = fw && (this->isAncestorOf(fw)) && (qobject_cast<QLineEdit*>(fw) || qobject_cast<QAbstractSpinBox*>(fw) || qobject_cast<QComboBox*>(fw));

    if (editingInPanel)
    {
        pendingRebuild = true;
        return;
    }

    QTimer::singleShot(0, this, [this](){ Rebuild(); });
}

QWidget* PropertyEditorPanel::EditorForProperty(QObject* object, const QMetaProperty& prop)
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
            obj->setProperty(prop.name(), index);
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
            obj->setProperty(prop.name(), index);
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
        cb->setChecked(object->property(prop.name()).toBool());

        QPointer<QObject> obj = object;
        QObject::connect(cb, &QCheckBox::toggled, this, [this, obj, prop](bool checked)
        {
            if (!obj)
                return;
            suppressRebuild = true;
            obj->setProperty(prop.name(), checked);
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

        box->setRange(-100000.0, 100000.0);
        box->setDecimals(3);
        box->setValue(object->property(prop.name()).toDouble());


        QPointer<QObject> obj = object;
        QObject::connect(box, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, obj, prop](double v)
        {
            if (!obj)
                return;
            suppressRebuild = true;
            obj->setProperty(prop.name(), v);
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

        box->setRange(0, 4096);
        box->setValue(object->property(prop.name()).toInt());

        QPointer<QObject> obj = object;

        QObject::connect(box, qOverload<int>(&QSpinBox::valueChanged), this, [this, obj, prop](int v)
        {
            if (!obj)
                return;

            suppressRebuild = true;
            obj->setProperty(prop.name(), v);
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

        auto* edit = new QLineEdit(object->property(prop.name()).toString());
        auto* browse = new QPushButton("...");
        browse->setFixedWidth(30);

        QPointer<QObject> obj = object;

        QObject::connect(edit, &QLineEdit::editingFinished, this, [this, edit, obj, prop]()
        {
            if (!obj)
                return;

            QString v = edit->text().trimmed();

            if (!v.isEmpty() && !QFile::exists(v))
                edit->setStyleSheet("QLineEdit { border: 1px solid #cc4444; }");
            else
                edit->setStyleSheet(QString());

            suppressRebuild = true;
            obj->setProperty(prop.name(), v);
            suppressRebuild = false;

            emit PropertyEdited();
        });

        QObject::connect(browse, &QPushButton::clicked, this, [this, edit, obj, prop, name]()
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

            auto path = QFileDialog::getOpenFileName(nullptr, title, QString(), filter);

            if (!path.isEmpty() && obj)
            {
                edit->setText(path);
                edit->setStyleSheet(QString());

                suppressRebuild = true;
                obj->setProperty(prop.name(), path);
                suppressRebuild = false;

                emit PropertyEdited();
            }
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
        auto* edit = new QLineEdit(object->property(prop.name()).toString());

        QPointer<QObject> obj = object;

        QObject::connect(edit, &QLineEdit::textEdited, this, [this, obj, prop](const QString& v)
        {
            if (!obj)
                return;
            suppressRebuild = true;
            obj->setProperty(prop.name(), v);
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

            obj->setProperty(prop.name(), v);
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

            obj->setProperty(prop.name(), v);
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

            obj->setProperty(prop.name(), v);
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
            obj->setProperty(prop.name(), e.value(index));
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

                obj->setProperty(prop.name(), v);
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

        x->setRange(-100000.0, 100000.0);
        y->setRange(-100000.0, 100000.0);
        x->setDecimals(3);
        y->setDecimals(3);

        auto p = object->property(prop.name()).toPointF();
        x->setValue(p.x());
        y->setValue(p.y());

        QPointer<QObject> obj = object;

        QObject::connect(x, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, obj, prop, y](double xv)
        {
            if (!obj)
                return;

            suppressRebuild = true;
            obj->setProperty(prop.name(), QPointF(xv, y->value()));
            suppressRebuild = false;

            emit PropertyEdited();
        });

        QObject::connect(y, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, obj, prop, x](double yv)
        {
            if (!obj)
                return;

            suppressRebuild = true;
            obj->setProperty(prop.name(), QPointF(x->value(), yv));
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
        auto* button = new QPushButton(object->property(prop.name()).value<QColor>().name(QColor::HexArgb));

        QPointer<QObject> obj = object;
        QPointer<QPushButton> btn = button;

        QObject::connect(button, &QPushButton::clicked, this, [this, obj, prop, btn]()
        {
            if (!obj || !btn)
                return;

            suppressRebuild = true;
            QColor c = QColorDialog::getColor(obj->property(prop.name()).value<QColor>());

            if (c.isValid())
            {
                obj->setProperty(prop.name(), c);

                if (btn)
                    btn->setText(c.name(QColor::HexArgb));
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

    QLayoutItem* child;

    while ((child = layout->takeAt(0)) != nullptr)
    {
        if (auto* w = child->widget())
            w->deleteLater();

        delete child;
    }

    if (!target)
        return;

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

        target->SetName(v);
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
