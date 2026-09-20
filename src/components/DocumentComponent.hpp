#ifndef COMPONENTS_DOCUMENTCOMPONENT_HPP
#define COMPONENTS_DOCUMENTCOMPONENT_HPP

#include "core/Component.hpp"

// Document-level settings, carried as a component on the ROOT element.
//
// Why a component rather than a header field: the .uibin header is a fixed 32
// bytes and extending it would force a version bump, which every existing
// consumer would reject. A component rides the machinery that is already
// there - it interns, encodes and length-prefixes like any other, and a
// consumer that does not recognise the type name skips it by payloadLen with
// no special handling. Backward AND forward compatible, no format change.
//
// It is the carrier for the pixel-perfect rendering model, which an engine
// cannot otherwise reproduce: without the unit it does not know how large a
// virtual pixel is, so it cannot lay a texel on one.
class DocumentComponent : public Component
{
    Q_OBJECT

    // 0 = Continuous (exact float layout), 1 = PixelGrid (geometry rounds to
    // whole virtual pixels and textures draw one texel per virtual pixel).
    Q_PROPERTY(int renderModel READ GetRenderModel WRITE SetRenderModel NOTIFY ComponentChanged)

    // Scene units per virtual pixel. Square by definition. Only meaningful
    // when renderModel is 1.
    Q_PROPERTY(double pixelUnit READ GetPixelUnit WRITE SetPixelUnit NOTIFY ComponentChanged)

public:

    explicit DocumentComponent(QObject* parent = nullptr);

    QString GetTypeName() const override;

    int GetRenderModel() const noexcept;
    void SetRenderModel(int v);

    double GetPixelUnit() const noexcept;
    void SetPixelUnit(double v);

    // Copy the live PixelModel globals into this component, and back out.
    // SceneDocument calls these at save and load so there is exactly one
    // source of truth at any moment.
    void CaptureFromPixelModel();
    void ApplyToPixelModel() const;

    void ToJson(QJsonObject& out) const override;
    void FromJson(const QJsonObject& in) override;

private:

    int m_renderModel = 0;
    double m_pixelUnit = 1.0;
};

#endif
