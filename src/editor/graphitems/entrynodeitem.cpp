#include "editor/graphitems/entrynodeitem.h"

EntryNodeItem::EntryNodeItem(QGraphicsItem* parent)
    : QGraphicsObject(parent)
{
}

QRectF EntryNodeItem::boundingRect() const
{
    return QRectF(-RADIUS - 2, -RADIUS - 2, RADIUS * 2 + 4, RADIUS * 2 + 4);
}

void EntryNodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setBrush(QColor(60, 180, 60));
    painter->setPen(QPen(QColor(80, 220, 80), 2));
    painter->drawEllipse(QPointF(0, 0), RADIUS, RADIUS);

    painter->setPen(Qt::white);
    QFont font = painter->font();
    font.setPointSize(8);
    painter->setFont(font);
    painter->drawText(boundingRect(), Qt::AlignCenter, "Entry");
}

QPointF EntryNodeItem::rightEdge() const
{
    return scenePos() + QPointF(RADIUS, 0);
}
