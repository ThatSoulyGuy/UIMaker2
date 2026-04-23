#ifndef RENDERPIPELINE_HPP
#define RENDERPIPELINE_HPP

#include <QObject>
#include <QList>
#include <QPainter>
#include <algorithm>

#include "render/RenderPass.hpp"
#include "input/EditorContext.hpp"

class RenderPipeline : public QObject
{
    Q_OBJECT

public:

    explicit RenderPipeline(QObject* parent = nullptr) : QObject(parent) { }

    ~RenderPipeline()
    {
        qDeleteAll(m_passes);
    }

    void AddPass(RenderPass* pass)
    {
        if (!pass)
            return;

        pass->setParent(this);
        m_passes.append(pass);

        SortPasses();
    }

    void RemovePass(const QString& id)
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

    RenderPass* GetPass(const QString& id) const
    {
        for (auto* pass : m_passes)
        {
            if (pass->GetId() == id)
                return pass;
        }

        return nullptr;
    }

    QList<RenderPass*> GetPasses() const
    {
        return m_passes;
    }

    void Render(QPainter& painter, const EditorContext& ctx)
    {
        for (auto* pass : m_passes)
        {
            if (pass->IsEnabled())
                pass->Render(painter, ctx);
        }
    }

    void SetLayerEnabled(RenderLayer layer, bool enabled)
    {
        for (auto* pass : m_passes)
        {
            if (pass->GetLayer() == layer)
                pass->SetEnabled(enabled);
        }
    }

private:

    void SortPasses()
    {
        std::sort(m_passes.begin(), m_passes.end(),
            [](const RenderPass* a, const RenderPass* b) {
                return static_cast<int>(a->GetLayer()) < static_cast<int>(b->GetLayer());
            });
    }

    QList<RenderPass*> m_passes;

};

#endif
