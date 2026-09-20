#include "components/DocumentComponent.hpp"
#include "core/PixelModel.hpp"

#include <QJsonObject>

REGISTER_COMPONENT(DocumentComponent, "Document")

DocumentComponent::DocumentComponent(QObject* parent) : Component(parent) { }

QString DocumentComponent::GetTypeName() const
{
    return QStringLiteral("Document");
}

int DocumentComponent::GetRenderModel() const noexcept
{
    return m_renderModel;
}

void DocumentComponent::SetRenderModel(int v)
{
    const int c = (v == 1) ? 1 : 0;

    if (m_renderModel == c)
        return;

    m_renderModel = c;
    NotifyChanged();
}

double DocumentComponent::GetPixelUnit() const noexcept
{
    return m_pixelUnit;
}

void DocumentComponent::SetPixelUnit(double v)
{
    // A non-positive unit would make every virtual-pixel division degenerate.
    const double c = (v > 0.0) ? v : 1.0;

    if (qFuzzyCompare(m_pixelUnit, c))
        return;

    m_pixelUnit = c;
    NotifyChanged();
}

void DocumentComponent::CaptureFromPixelModel()
{
    SetRenderModel(PixelModel::GetMode() == PixelModel::Mode::PixelGrid ? 1 : 0);
    SetPixelUnit(PixelModel::GetUnit());
}

void DocumentComponent::ApplyToPixelModel() const
{
    PixelModel::SetMode(m_renderModel == 1 ? PixelModel::Mode::PixelGrid
                                           : PixelModel::Mode::Continuous);
    PixelModel::SetUnit(m_pixelUnit);
}

void DocumentComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Document";
    out["renderModel"] = m_renderModel;
    out["pixelUnit"] = m_pixelUnit;
}

void DocumentComponent::FromJson(const QJsonObject& in)
{
    SetRenderModel(in["renderModel"].toInt(0));
    SetPixelUnit(in["pixelUnit"].toDouble(1.0));
}
