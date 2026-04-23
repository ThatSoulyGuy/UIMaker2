#ifndef CORE_COMPONENT_HPP
#define CORE_COMPONENT_HPP

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QHash>
#include <QRectF>
#include <functional>

class SceneElementItem;
class QPainter;

class Component : public QObject
{
    Q_OBJECT

public:

    explicit Component(QObject* parent = nullptr) : QObject(parent) { }

    virtual ~Component() = default;
    virtual QString GetTypeName() const = 0;

    virtual int UpdateOrder() const
    {
        return 0;
    }

    virtual bool IsLayout() const
    {
        return false;
    }

    virtual void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
    {
        Q_UNUSED(item);
        Q_UNUSED(rect);
        Q_UNUSED(parentRect);
    }

    virtual bool Paint(QPainter* painter, const QRectF& rect, bool selected)
    {
        Q_UNUSED(painter);
        Q_UNUSED(rect);
        Q_UNUSED(selected);

        return false;
    }

    virtual void ToJson(QJsonObject& out) const = 0;
    virtual void FromJson(const QJsonObject& in) = 0;

    using ComponentFactory = std::function<Component*(QObject*)>;
    static QHash<QString, ComponentFactory>& Registry()
    {
        static QHash<QString, ComponentFactory> registry;

        return registry;
    }

    static void Register(const QString& name, ComponentFactory factory)
    {
        Registry().insert(name, factory);
    }

    static Component* Create(const QString& name, QObject* parent)
    {
        auto it = Registry().find(name);
        return it != Registry().end() ? it.value()(parent) : nullptr;
    }

public slots:

    void EmitComponentChanged()
    {
        emit ComponentChanged();
    }

protected:

    void NotifyChanged()
    {
        QMetaObject::invokeMethod(this, "EmitComponentChanged", Qt::QueuedConnection);
    }

signals:

    void ComponentChanged();

};

#define REGISTER_COMPONENT(ClassName, ComponentName) \
    static const bool ClassName##_component_registered = [](){ \
        Component::Register(ComponentName, [](QObject* p){ return new ClassName(p); }); \
        return true; \
    }();

#endif
