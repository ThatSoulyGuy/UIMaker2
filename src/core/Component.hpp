#ifndef CORE_COMPONENT_HPP
#define CORE_COMPONENT_HPP

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QHash>
#include <QRectF>
#include <Qt>
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

    // The axes this component sizes from its CHILDREN rather than from its own
    // Transform.
    //
    // A child cannot meaningfully stretch to fill such an axis: "fill the space
    // I am given" is circular when the space is derived from the filler. The
    // child sizes itself to the container, the container re-measures and comes
    // out larger by its padding, and every interaction pushes it further - which
    // is exactly the runaway growth this reports. Stretch resolution walks PAST
    // these axes to the first ancestor that has a size of its own.
    //
    // Per axis, not per component, because a vertical ScrollBox owns its height
    // (from Transform) while fitting its width to its children.
    virtual Qt::Orientations ShrinkWrapAxes() const;

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
