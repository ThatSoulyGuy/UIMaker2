#include "SceneElementItem.hpp"
#include <QFontMetrics>
#include <QGraphicsScene>
#include <cmath>

SceneElementItem::SceneElementItem(UiElement* element) : QGraphicsObject(nullptr), element(element), localRect(-50.0, -25.0, 100.0, 50.0)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    for (auto* comp : element->GetComponents())
        QObject::connect(comp, &Component::ComponentChanged, this, &SceneElementItem::OnComponentChanged);

    QObject::connect(element, &UiElement::ComponentListChanged, this, &SceneElementItem::RefreshFromComponents);

    RefreshFromComponents();
}

void SceneElementItem::OnComponentChanged()
{
    RefreshFromComponents();
    update();
}

void SceneElementItem::RefreshFromComponents()
{
    auto* xform = element->GetComponent<TransformComponent>();

    if (xform == nullptr)
        xform = element->AddComponent<TransformComponent>();

    QRectF newRect = localRect;
    QPixmap newPixmap;

    if (auto* img = element->GetComponent<ImageComponent>())
    {
        if (!img->GetImagePath().isEmpty())
        {
            QPixmap loaded(img->GetImagePath());
            if (!loaded.isNull())
            {
                newPixmap = loaded;
                newRect = QRectF(QPointF(0.0, 0.0), loaded.size());
            }
        }
    }
    else if (auto* text = element->GetComponent<TextComponent>())
    {
        QFont font(text->GetFontFamily(), text->GetPixelSize());
        QFontMetrics fontMetrics(font);

        const QSize size(fontMetrics.horizontalAdvance(text->GetText()), fontMetrics.height());

        newRect = QRectF(QPointF(0.0, 0.0), size);
    }
    else if (auto* button = element->GetComponent<ButtonComponent>())
    {
        QFont font(button->GetFontFamily(), button->GetPixelSize());
        QFontMetrics fm(font);

        const QSize size(fm.horizontalAdvance(button->GetText()) + 40, fm.height() + 20);

        newRect = QRectF(QPointF(0.0, 0.0), size);
    }
    else
        newRect = QRectF(0.0, 0.0, 100.0, 50.0);

    QRectF parentRect;

    if (auto* p = parentItem())
        parentRect = p->boundingRect();
    else if (scene())
        parentRect = scene()->sceneRect();

    QPointF pos = xform->GetPosition();
    auto stretch = xform->GetStretch();

    if (stretch.testFlag(Anchor::LEFT) && stretch.testFlag(Anchor::RIGHT))
        newRect.setWidth(parentRect.width() - pos.x() * 2.0);
    if (stretch.testFlag(Anchor::TOP) && stretch.testFlag(Anchor::BOTTOM))
        newRect.setHeight(parentRect.height() - pos.y() * 2.0);

    if (newRect != localRect)
    {
        prepareGeometryChange();
        localRect = newRect;
    }

    pixmap = newPixmap;

    auto anchors = xform->GetAnchors();

    double x = pos.x();
    double y = pos.y();

    if (anchors.testFlag(Anchor::RIGHT))
        x = parentRect.width() - localRect.width() - pos.x();
    else if (anchors.testFlag(Anchor::CENTER_X))
        x = (parentRect.width() - localRect.width()) * 0.5 + pos.x();

    if (anchors.testFlag(Anchor::BOTTOM))
        y = parentRect.height() - localRect.height() - pos.y();
    else if (anchors.testFlag(Anchor::CENTER_Y))
        y = (parentRect.height() - localRect.height()) * 0.5 + pos.y();

    setPos(parentRect.topLeft() + QPointF(x, y));
    setRotation(xform->GetRotationDegrees());
    setScale(std::max(0.0001, xform->GetScale().x()));
}


QVariant SceneElementItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged)
    {
        if (auto* xform = element->GetComponent<TransformComponent>())
        {
            QRectF parentRect;

            if (auto* p = parentItem())
                parentRect = p->boundingRect();
            else if (scene())
                parentRect = scene()->sceneRect();

            auto anchors = xform->GetAnchors();
            QPointF p = pos() - parentRect.topLeft();

            if (anchors.testFlag(Anchor::RIGHT))
                p.setX(parentRect.width() - localRect.width() - p.x());
            else if (anchors.testFlag(Anchor::CENTER_X))
                p.setX(p.x() - (parentRect.width() - localRect.width()) * 0.5);

            if (anchors.testFlag(Anchor::BOTTOM))
                p.setY(parentRect.height() - localRect.height() - p.y());
            else if (anchors.testFlag(Anchor::CENTER_Y))
                p.setY(p.y() - (parentRect.height() - localRect.height()) * 0.5);

            xform->SetPosition(p);
        }
    }

    if (change == ItemRotationHasChanged)
    {
        if (auto* xform = element->GetComponent<TransformComponent>())
            xform->SetRotationDegrees(rotation());
    }

    if (change == ItemScaleHasChanged)
    {
        if (auto* xform = element->GetComponent<TransformComponent>())
            xform->SetScale(QPointF(scale(), scale()));
    }

    return QGraphicsObject::itemChange(change, value);
}


void SceneElementItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();

    if (!pixmap.isNull())
    {
        painter->setOpacity(1.0);
        painter->drawPixmap(localRect.topLeft(), pixmap);

        if (isSelected())
        {
            painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(localRect);
        }

        painter->restore();

        return;
    }

    if (auto* text = element->GetComponent<TextComponent>())
    {
        QFont font(text->GetFontFamily(), text->GetPixelSize());

        painter->setFont(font);
        painter->setPen(text->GetColor());

        painter->drawText(localRect.topLeft() + QPointF(0.0, localRect.height() - 4.0), text->GetText());

        if (isSelected())
        {
            painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(localRect);
        }

        painter->restore();

        return;
    }

    if (auto* button = element->GetComponent<ButtonComponent>())
    {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(button->GetBackgroundColor());
        painter->drawRoundedRect(localRect, 6.0, 6.0);
        painter->setPen(button->GetTextColor());
        QFont font(button->GetFontFamily(), button->GetPixelSize());
        painter->setFont(font);
        painter->drawText(localRect, Qt::AlignCenter, button->GetText());

        if (isSelected())
        {
            painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(localRect, 6.0, 6.0);
        }

        painter->restore();

        return;
    }

    painter->setBrush(QColor(80, 80, 80));
    painter->setPen(Qt::NoPen);
    painter->drawRect(localRect);

    if (isSelected())
    {
        painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(localRect);
    }

    painter->restore();
}
