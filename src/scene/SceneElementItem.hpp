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

    // This element together with everything under it.
    //
    // boundingRect() is the element ALONE. That is the right answer for
    // painting and for what a scale handle edits, but it is the wrong answer
    // for any question about the block as a whole: what selecting it should
    // frame, and how much room a layout has to reserve for it. A Text with an
    // image and a button beneath it measured 100x100 - its glyph run - while
    // occupying 120x178, so a layout packed the next sibling straight through
    // the image.
    //
    // A container that clips its children (ScrollBox) is excluded: nothing a
    // child does there can extend the block, because none of it is visible
    // outside the container.
    QRectF BlockRect() const;
    QRectF BlockSceneRect() const;

    // The size a STRETCH resolves against, per axis.
    //
    // Normally this is simply the parent's size. It differs only under a
    // layout: a layout derives its size from its children, so a child that
    // sizes itself to the layout closes a loop - the layout re-measures,
    // comes out larger by its padding, and every subsequent interaction
    // pushes it further out. This walks past any ancestor that shrink-wraps
    // on the axis in question (Component::ShrinkWrapAxes) to the first one
    // that owns a size, falling back to the design canvas.
    QSizeF StretchReferenceSize() const;

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
