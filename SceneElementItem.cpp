#include "SceneElementItem.hpp"
#include <QFontMetrics>
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

    setPos(xform->GetPosition());
    setRotation(xform->GetRotationDegrees());
    setScale(std::max(0.0001, xform->GetScale().x()));

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
        QFontMetrics fm(font);

        const QSize size(fm.horizontalAdvance(text->GetText()), fm.height());

        newRect = QRectF(QPointF(0.0, 0.0), size);
    }
    else if (element->GetComponent<ButtonComponent>())
        newRect = QRectF(0.0, 0.0, 160.0, 48.0);
    else
        newRect = QRectF(0.0, 0.0, 100.0, 50.0);

    if (newRect != localRect)
    {
        prepareGeometryChange();
        localRect = newRect;
    }

    pixmap = newPixmap;
}

QVariant SceneElementItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged)
    {
        if (auto* xform = element->GetComponent<TransformComponent>())
            xform->SetPosition(pos());
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
