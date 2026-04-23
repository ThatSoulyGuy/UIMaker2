#ifndef RENDERPASS_HPP
#define RENDERPASS_HPP

#include <QObject>
#include <QString>
#include <QPainter>
#include <QHash>
#include <functional>

#include "input/EditorContext.hpp"

enum class RenderLayer
{
    Background = 0,
    Scene = 100,
    Selection = 200,
    Guides = 300,
    Gizmos = 400,
    Overlay = 500,
    Debug = 1000
};

class RenderPass : public QObject
{
    Q_OBJECT

public:

    explicit RenderPass(QObject* parent = nullptr) : QObject(parent) { }

    virtual ~RenderPass() = default;

    virtual QString GetId() const = 0;
    virtual RenderLayer GetLayer() const = 0;

    virtual void Render(QPainter& painter, const EditorContext& ctx) = 0;

    bool IsEnabled() const noexcept
    {
        return m_enabled;
    }

    void SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    using RenderPassFactory = std::function<RenderPass*(QObject*)>;

    static QHash<QString, RenderPassFactory>& Registry()
    {
        static QHash<QString, RenderPassFactory> registry;

        return registry;
    }

    static void Register(const QString& id, RenderPassFactory factory)
    {
        Registry().insert(id, factory);
    }

    static RenderPass* Create(const QString& id, QObject* parent)
    {
        auto it = Registry().find(id);

        return it != Registry().end() ? it.value()(parent) : nullptr;
    }

    static QStringList GetRegisteredIds()
    {
        return Registry().keys();
    }

protected:

    bool m_enabled = true;

};

#define REGISTER_RENDER_PASS(ClassName, PassId) \
    static const bool ClassName##_pass_registered = [](){ \
        RenderPass::Register(PassId, [](QObject* p){ return new ClassName(p); }); \
        return true; \
    }();

#endif
