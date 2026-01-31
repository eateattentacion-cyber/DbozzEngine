#include "editor/graphitems/transitionarrowitem.h"
#include "editor/graphitems/statenodeitem.h"
#include <QGraphicsSceneMouseEvent>
#include <cmath>

TransitionArrowItem::TransitionArrowItem(int transitionId, StateNodeItem* source, StateNodeItem* dest, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_transitionId(transitionId)
    , m_source(source)
    , m_dest(dest)
{
    setFlag(ItemIsSelectable);
    setZValue(-1);
    recalculate();
}

void TransitionArrowItem::recalculate()
{
    prepareGeometryChange();
    m_sourcePoint = m_source->rightEdge();
    m_destPoint = m_dest->leftEdge();

    // Offset for bidirectional: shift control point perpendicular
    QPointF mid = (m_sourcePoint + m_destPoint) / 2.0;
    QPointF diff = m_destPoint - m_sourcePoint;
    QPointF perp(-diff.y(), diff.x());
    float len = std::sqrt(perp.x() * perp.x() + perp.y() * perp.y());
    if (len > 0) perp /= len;

    m_controlPoint = mid + perp * 30.0;
}

QRectF TransitionArrowItem::boundingRect() const
{
    float minX = std::min({m_sourcePoint.x(), m_destPoint.x(), m_controlPoint.x()}) - 15;
    float minY = std::min({m_sourcePoint.y(), m_destPoint.y(), m_controlPoint.y()}) - 15;
    float maxX = std::max({m_sourcePoint.x(), m_destPoint.x(), m_controlPoint.x()}) + 15;
    float maxY = std::max({m_sourcePoint.y(), m_destPoint.y(), m_controlPoint.y()}) + 15;
    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

QPainterPath TransitionArrowItem::shape() const
{
    QPainterPath path;
    path.moveTo(m_sourcePoint);
    path.quadTo(m_controlPoint, m_destPoint);

    QPainterPathStroker stroker;
    stroker.setWidth(12);
    return stroker.createStroke(path);
}

void TransitionArrowItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    recalculate();

    QPen pen;
    if (m_isActive) {
        pen = QPen(QColor(255, 160, 0), 3);
    } else if (isSelected()) {
        pen = QPen(QColor(100, 150, 255), 2.5);
    } else {
        pen = QPen(QColor(180, 180, 180), 2);
    }

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    QPainterPath path;
    path.moveTo(m_sourcePoint);
    path.quadTo(m_controlPoint, m_destPoint);
    painter->drawPath(path);

    // Arrow head
    drawArrowHead(painter, m_destPoint, m_controlPoint);
}

void TransitionArrowItem::drawArrowHead(QPainter* painter, const QPointF& tip, const QPointF& from)
{
    QPointF dir = tip - from;
    float len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (len < 0.001f) return;
    dir /= len;

    float arrowSize = 10.0f;
    QPointF perp(-dir.y(), dir.x());

    QPointF p1 = tip - dir * arrowSize + perp * (arrowSize * 0.5f);
    QPointF p2 = tip - dir * arrowSize - perp * (arrowSize * 0.5f);

    painter->setBrush(painter->pen().color());
    QPolygonF arrowHead;
    arrowHead << tip << p1 << p2;
    painter->drawPolygon(arrowHead);
}

void TransitionArrowItem::setActive(bool active)
{
    m_isActive = active;
    update();
}

void TransitionArrowItem::updatePosition()
{
    recalculate();
    update();
}

void TransitionArrowItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsObject::mousePressEvent(event);
    emit selected(m_transitionId);
}
