#include "PropertyEditorPanel.hpp"
#include <QLabel>
#include <QGroupBox>

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
    target = element;
    Rebuild();
}

void PropertyEditorPanel::OnComponentChanged()
{
    Rebuild();
}

QWidget* PropertyEditorPanel::EditorForProperty(QObject* object, const QMetaProperty& prop)
{
    const QString name = QString::fromLatin1(prop.name());
    const auto type = prop.metaType();

    if (type == QMetaType::fromType<double>())
    {
        auto* box = new QDoubleSpinBox();

        box->setRange(-100000.0, 100000.0);
        box->setDecimals(3);
        box->setValue(object->property(prop.name()).toDouble());

        QObject::connect(box, &QDoubleSpinBox::valueChanged, object, [object, prop](double v)
        {
            object->setProperty(prop.name(), v);
        });

        return box;
    }

    if (type == QMetaType::fromType<int>())
    {
        auto* box = new QSpinBox();

        box->setRange(0, 4096);
        box->setValue(object->property(prop.name()).toInt());

        QObject::connect(box, &QSpinBox::valueChanged, object, [object, prop](int v)
        {
            object->setProperty(prop.name(), v);
        });

        return box;
    }

    if (type == QMetaType::fromType<QString>())
    {
        auto* edit = new QLineEdit(object->property(prop.name()).toString());

        QObject::connect(edit, &QLineEdit::textEdited, object, [object, prop](const QString& v)
        {
            object->setProperty(prop.name(), v);
        });

        return edit;
    }

    if (type == QMetaType::fromType<QPointF>())
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

        QObject::connect(x, &QDoubleSpinBox::valueChanged, object, [object, prop, y](double xv)
        {
            object->setProperty(prop.name(), QPointF(xv, y->value()));
        });

        QObject::connect(y, &QDoubleSpinBox::valueChanged, object, [object, prop, x](double yv)
        {
            object->setProperty(prop.name(), QPointF(x->value(), yv));
        });

        h->addWidget(x);
        h->addWidget(y);
        row->setLayout(h);

        return row;
    }

    if (type == QMetaType::fromType<QColor>())
    {
        auto* button = new QPushButton(object->property(prop.name()).value<QColor>().name(QColor::HexArgb));

        QObject::connect(button, &QPushButton::clicked, this, [object, prop, button]()
        {
            QColor c = QColorDialog::getColor(object->property(prop.name()).value<QColor>());

            if (c.isValid())
            {
                object->setProperty(prop.name(), c);

                button->setText(c.name(QColor::HexArgb));
            }
        });

        return button;
    }

    if (name == "imagePath")
    {
        auto* row = new QWidget();

        auto* h = new QHBoxLayout();

        h->setContentsMargins(0,0,0,0);

        auto* edit = new QLineEdit(object->property(prop.name()).toString());
        auto* browse = new QPushButton("...");
        QObject::connect(edit, &QLineEdit::textEdited, object, [object, prop](const QString& v)
        {
            object->setProperty(prop.name(), v);
        });

        QObject::connect(browse, &QPushButton::clicked, this, [edit, object, prop]()
        {
            auto path = QFileDialog::getOpenFileName(nullptr, "Choose Image", QString(), "Images (*.png *.jpg *.jpeg *.bmp)");

            if (!path.isEmpty())
            {
                edit->setText(path);
                object->setProperty(prop.name(), path);
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

    auto* nameRow = new QWidget(); auto* nameLayout = new QHBoxLayout();

    nameLayout->setContentsMargins(0,0,0,0);

    auto* nameEdit = new QLineEdit(target->GetName());
    QObject::connect(nameEdit, &QLineEdit::textEdited, target, [this](const QString& v){ target->SetName(v); });

    nameLayout->addWidget(new QLabel("Name"));
    nameLayout->addWidget(nameEdit);
    nameRow->setLayout(nameLayout);
    layout->addWidget(nameRow);

    for (auto* comp : target->GetComponents())
    {
        auto* group = new QGroupBox(QString("%1").arg([&]()
        {
            switch (comp->GetKind())
            {
                case ComponentKind::TRANSFORM:
                    return "Transform";

                case ComponentKind::IMAGE:
                    return "Image";

                case ComponentKind::TEXT:
                    return "Text";

                case ComponentKind::BUTTON:
                    return "Button";
            }

            return "Component";
        }()));

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

        QObject::connect(comp, &Component::ComponentChanged, this, &PropertyEditorPanel::OnComponentChanged);
    }

    layout->addStretch(1);
}
