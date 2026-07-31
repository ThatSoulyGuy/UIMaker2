#ifndef RENDERPASS_HPP
#define RENDERPASS_HPP

#include <QObject>
#include <QString>
#include <QPainter>

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

    explicit RenderPass(QObject* parent = nullptr);

    virtual ~RenderPass() = default;

    virtual QString GetId() const = 0;
    virtual RenderLayer GetLayer() const = 0;

    virtual void Render(QPainter& painter, const EditorContext& ctx) = 0;

    bool IsEnabled() const noexcept;

    void SetEnabled(bool enabled);

protected:

    bool m_enabled = true;

};

#endif
