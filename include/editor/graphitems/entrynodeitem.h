#pragma once

#include <QGraphicsObject>
#include <QPainter>

class EntryNodeItem : public QGraphicsObject
{
    Q_OBJECT

public:
    EntryNodeItem(QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QPointF rightEdge() const;

private:
    static constexpr float RADIUS = 15.0f;
};
