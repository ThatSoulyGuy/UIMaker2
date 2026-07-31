#include "core/Component.hpp"

#include <QMetaObject>

Component::Component(QObject* parent) : QObject(parent) { }

int Component::UpdateOrder() const
{
    return 0;
}

bool Component::IsLayout() const
{
    return false;
}

void Component::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);
    Q_UNUSED(rect);
    Q_UNUSED(parentRect);
}

bool Component::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    Q_UNUSED(painter);
    Q_UNUSED(rect);
    Q_UNUSED(selected);

    return false;
}

QHash<QString, Component::ComponentFactory>& Component::Registry()
{
    static QHash<QString, ComponentFactory> registry;

    return registry;
}

void Component::Register(const QString& name, ComponentFactory factory)
{
    Registry().insert(name, factory);
}

Component* Component::Create(const QString& name, QObject* parent)
{
    auto it = Registry().find(name);
    return it != Registry().end() ? it.value()(parent) : nullptr;
}

void Component::EmitComponentChanged()
{
    emit ComponentChanged();
}

void Component::NotifyChanged()
{
    QMetaObject::invokeMethod(this, "EmitComponentChanged", Qt::QueuedConnection);
}
