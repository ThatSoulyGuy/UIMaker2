#ifndef RENDERPIPELINE_HPP
#define RENDERPIPELINE_HPP

#include <QObject>
#include <QList>
#include <QPainter>

#include "render/RenderPass.hpp"
#include "input/EditorContext.hpp"

class RenderPipeline : public QObject
{
    Q_OBJECT

public:

    explicit RenderPipeline(QObject* parent = nullptr);

    ~RenderPipeline();

    void AddPass(RenderPass* pass);

    void RemovePass(const QString& id);

    RenderPass* GetPass(const QString& id) const;

    QList<RenderPass*> GetPasses() const;

    void Render(QPainter& painter, const EditorContext& ctx);

    void SetLayerEnabled(RenderLayer layer, bool enabled);

private:

    void SortPasses();

    QList<RenderPass*> m_passes;

};

#endif
