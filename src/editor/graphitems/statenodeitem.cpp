#include "editor/graphitems/statenodeitem.h"
#include <QMenu>
#include <QAction>

StateNodeItem::StateNodeItem(int stateId, const QString& name, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_stateId(stateId)
    , m_name(name)
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
    setCacheMode(DeviceCoordinateCache);
}

QRectF StateNodeItem::boundingRect() const
{
    return QRectF(-WIDTH / 2 - 2, -HEIGHT / 2 - 2, WIDTH + 4, HEIGHT + 4);
}

void StateNodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    QRectF rect(-WIDTH / 2, -HEIGHT / 2, WIDTH, HEIGHT);

    // Background
    QColor fillColor(60, 60, 60);
    if (m_isEntry) fillColor = QColor(40, 70, 40);

    painter->setBrush(fillColor);

    // Border
    QPen pen;
    if (m_isActive) {
        pen = QPen(QColor(255, 160, 0), 3);
    } else if (m_isBlending) {
        int alpha = static_cast<int>(255 * (1.0f - m_blendProgress));
        pen = QPen(QColor(255, 160, 0, alpha), 2);
    } else if (isSelected()) {
        pen = QPen(QColor(100, 150, 255), 2);
    } else if (m_isEntry) {
        pen = QPen(QColor(80, 200, 80), 2);
    } else {
        pen = QPen(QColor(120, 120, 120), 1);
    }

    painter->setPen(pen);
    painter->drawRoundedRect(rect, RADIUS, RADIUS);

    // Text
    painter->setPen(Qt::white);
    painter->drawText(rect, Qt::AlignCenter, m_name);
}

void StateNodeItem::setName(const QString& name)
{
    m_name = name;
    update();
}

void StateNodeItem::setActive(bool active)
{
    m_isActive = active;
    update();
}

void StateNodeItem::setEntryState(bool entry)
{
    m_isEntry = entry;
    update();
}

void StateNodeItem::setBlending(bool blending, float progress)
{
    m_isBlending = blending;
    m_blendProgress = progress;
    update();
}

QPointF StateNodeItem::centerPos() const
{
    return scenePos();
}

QPointF StateNodeItem::rightEdge() const
{
    return scenePos() + QPointF(WIDTH / 2, 0);
}

QPointF StateNodeItem::leftEdge() const
{
    return scenePos() + QPointF(-WIDTH / 2, 0);
}

void StateNodeItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;
    QAction* transitionAction = menu.addAction("Make Transition");
    QAction* entryAction = menu.addAction("Set as Entry State");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("Delete State");

    QAction* selected = menu.exec(event->screenPos());

    if (selected == transitionAction) {
        emit makeTransitionRequested(m_stateId);
    } else if (selected == entryAction) {
        emit setEntryRequested(m_stateId);
    } else if (selected == deleteAction) {
        emit deleteRequested(m_stateId);
    }
}

QVariant StateNodeItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged) {
        emit positionChanged(m_stateId, value.toPointF());
    }
    return QGraphicsObject::itemChange(change, value);
}
