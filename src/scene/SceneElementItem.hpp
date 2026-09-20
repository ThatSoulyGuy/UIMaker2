#ifndef SCENEELEMENTITEM_HPP
#define SCENEELEMENTITEM_HPP

#include <QGraphicsObject>
#include <QPainter>

#include "core/Anchor.hpp"

class UiElement;

class SceneElementItem : public QGraphicsObject
{
    Q_OBJECT

public:

    // Forward/inverse pair for anchor-based positioning, exposed so the checks
    // target can assert they really are inverses. AnchorToItemPos maps a
    // component-space position to an item pos inside parentRect;
    // ItemPosToComponent maps it back. They must change together.
    static QPointF AnchorToItemPos(const QPointF& pos, AnchorFlags anchors,
                                   const QRectF& parentRect, double w, double h);

    static QPointF ItemPosToComponent(const QPointF& itemPos, AnchorFlags anchors,
                                      const QRectF& parentRect, double w, double h);


    explicit SceneElementItem(UiElement* element);
    ~SceneElementItem() override = default;

    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    UiElement* GetElement() const noexcept;

    void setPosFromComponent(const QPointF& p);
    void setRotationFromComponent(double deg);

    // The rect a TOP-LEVEL element anchors against (the design canvas / screen).
    // Distinct from the scene rect, which is a large pasteboard for panning;
    // anchoring against the scene rect would offset every top-level element by
    // the pasteboard margin.
    void SetScreenRect(const QRectF& r);

protected:

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

public slots:

    void RefreshFromComponents();

private slots:

    void OnComponentChanged();

private:

    UiElement* element;
    QRectF localRect;
    QRectF screenRect;
    bool pendingRefresh = false;
    bool inLayoutRefresh = false;
    bool inDownwardCascade = false;

    bool ignorePositionFeedback = false;
    bool ignoreRotationFeedback = false;
};

#endif
