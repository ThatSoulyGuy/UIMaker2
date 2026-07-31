#include "render/RenderPass.hpp"

RenderPass::RenderPass(QObject* parent) : QObject(parent) { }

bool RenderPass::IsEnabled() const noexcept
{
    return m_enabled;
}

void RenderPass::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}
