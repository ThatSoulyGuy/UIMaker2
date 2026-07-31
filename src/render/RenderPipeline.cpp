#include "render/RenderPipeline.hpp"

#include <algorithm>

#include "render/RenderPass.hpp"
#include "input/EditorContext.hpp"

RenderPipeline::RenderPipeline(QObject* parent) : QObject(parent) { }

RenderPipeline::~RenderPipeline()
{
    qDeleteAll(m_passes);
}

void RenderPipeline::AddPass(RenderPass* pass)
{
    if (!pass)
        return;

    pass->setParent(this);
    m_passes.append(pass);

    SortPasses();
}

void RenderPipeline::RemovePass(const QString& id)
{
    for (int i = 0; i < m_passes.size(); ++i)
    {
        if (m_passes[i]->GetId() == id)
        {
            delete m_passes.takeAt(i);

            return;
        }
    }
}

RenderPass* RenderPipeline::GetPass(const QString& id) const
{
    for (auto* pass : m_passes)
    {
        if (pass->GetId() == id)
            return pass;
    }

    return nullptr;
}

QList<RenderPass*> RenderPipeline::GetPasses() const
{
    return m_passes;
}

void RenderPipeline::Render(QPainter& painter, const EditorContext& ctx)
{
    for (auto* pass : m_passes)
    {
        if (pass->IsEnabled())
        {
            // Isolate painter state so one pass can't leak pen/brush/clip
            // settings into the next.
            painter.save();
            pass->Render(painter, ctx);
            painter.restore();
        }
    }
}

void RenderPipeline::SetLayerEnabled(RenderLayer layer, bool enabled)
{
    for (auto* pass : m_passes)
    {
        if (pass->GetLayer() == layer)
            pass->SetEnabled(enabled);
    }
}

void RenderPipeline::SortPasses()
{
    std::sort(m_passes.begin(), m_passes.end(),
        [](const RenderPass* a, const RenderPass* b) {
            return static_cast<int>(a->GetLayer()) < static_cast<int>(b->GetLayer());
        });
}
