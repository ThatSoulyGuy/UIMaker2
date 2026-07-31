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

    explicit Component(QObject* parent = nullptr);

    virtual ~Component() = default;
    virtual QString GetTypeName() const = 0;

    virtual int UpdateOrder() const;

    virtual bool IsLayout() const;

    virtual void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect);

    virtual bool Paint(QPainter* painter, const QRectF& rect, bool selected);

    virtual void ToJson(QJsonObject& out) const = 0;
    virtual void FromJson(const QJsonObject& in) = 0;

    using ComponentFactory = std::function<Component*(QObject*)>;
    static QHash<QString, ComponentFactory>& Registry();

    static void Register(const QString& name, ComponentFactory factory);

    static Component* Create(const QString& name, QObject* parent);

public slots:

    void EmitComponentChanged();

protected:

    void NotifyChanged();

signals:

    void ComponentChanged();

};

#define REGISTER_COMPONENT(ClassName, ComponentName) \
    static const bool ClassName##_component_registered = [](){ \
        Component::Register(ComponentName, [](QObject* p){ return new ClassName(p); }); \
        return true; \
    }();

#endif
