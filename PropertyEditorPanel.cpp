#include "PropertyEditorPanel.hpp"
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPointer>
#include <QTimer>

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
        QObject::connect(target, &UiElement::ComponentListChanged, this, [this](UiElement*)
        {
            Rebuild();
        });
    }

    Rebuild();
}
void PropertyEditorPanel::OnComponentChanged()
{
    if (suppressRebuild)
        return;

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

    if (name == "anchors")
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

    if (name == "imagePath" || name == "fontPath")
    {
        auto* row = new QWidget();

        auto* h = new QHBoxLayout();

        h->setContentsMargins(0,0,0,0);

        auto* edit = new QLineEdit(object->property(prop.name()).toString());
        auto* browse = new QPushButton("...");

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

        QObject::connect(browse, &QPushButton::clicked, this, [this, edit, obj, prop, name]()
        {
            QString filter = name == "imagePath" ? "Images (*.png *.jpg *.jpeg *.bmp)" : "Fonts (*.ttf *.otf)";
            auto path = QFileDialog::getOpenFileName(nullptr, name == "imagePath" ? "Choose Image" : "Choose Font", QString(), filter);

            if (!path.isEmpty() && obj)
            {
                edit->setText(path);

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

    auto* label = new QLabel(QStringLiteral("<%1>").arg(QString::fromLatin1(prop.typeName())));

    return label;
}

void PropertyEditorPanel::Rebuild()
{
    QLayoutItem* child;

    while ((child = layout->takeAt(0)) != nullptr)
    {
        if (auto* w = child->widget())
            w->deleteLater();

        delete child;
    }

    if (!target)
        return;

    auto* nameRow = new QWidget();

    auto* nameLayout = new QHBoxLayout();

    nameLayout->setContentsMargins(0,0,0,0);

    auto* nameEdit = new QLineEdit(target->GetName());
    QObject::connect(nameEdit, &QLineEdit::textEdited, target, [this](const QString& v){ target->SetName(v); });

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
