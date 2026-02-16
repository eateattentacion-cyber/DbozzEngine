#pragma once

#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include "esquema/node.h"

class EsquemaNodeWidget : public QGraphicsItem {
public:
    EsquemaNodeWidget(std::shared_ptr<DabozzEngine::Esquema::Node> node);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    QPointF getInputPinPos(int index) const;
    QPointF getOutputPinPos(int index) const;
    
    int getInputPinAt(const QPointF& pos) const;
    int getOutputPinAt(const QPointF& pos) const;
    
    std::shared_ptr<DabozzEngine::Esquema::Node> getNode() const { return m_node; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    std::shared_ptr<DabozzEngine::Esquema::Node> m_node;
    qreal m_width;
    qreal m_height;
    qreal m_pinRadius;
};
