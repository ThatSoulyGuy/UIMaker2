#ifndef SCENEELEMENTITEM_HPP
#define SCENEELEMENTITEM_HPP

#include <QGraphicsObject>
#include <QPainter>

class UiElement;

class SceneElementItem : public QGraphicsObject
{
    Q_OBJECT

public:

    explicit SceneElementItem(UiElement* element);
    ~SceneElementItem() override = default;

    QRectF boundingRect() const override
    {
        return localRect;
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    UiElement* GetElement() const noexcept
    {
        return element;
    }

protected:

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private slots:

    void OnComponentChanged();
    void RefreshFromComponents();

private:

    UiElement* element;
    QRectF localRect;
};

#endif
