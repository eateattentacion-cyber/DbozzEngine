#pragma once

#include <QGraphicsObject>
#include <QPainter>

class StateNodeItem;

class TransitionArrowItem : public QGraphicsObject
{
    Q_OBJECT

public:
    TransitionArrowItem(int transitionId, StateNodeItem* source, StateNodeItem* dest, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    int transitionId() const { return m_transitionId; }
    void setActive(bool active);
    void updatePosition();

signals:
    void selected(int transitionId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    int m_transitionId;
    StateNodeItem* m_source;
    StateNodeItem* m_dest;
    bool m_isActive = false;

    QPointF m_sourcePoint;
    QPointF m_destPoint;
    QPointF m_controlPoint;

    void recalculate();
    void drawArrowHead(QPainter* painter, const QPointF& tip, const QPointF& from);
};
