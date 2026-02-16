#include "editor/esquemaconnection.h"
#include "editor/esquemanode.h"
#include <QPainterPath>

EsquemaConnectionWidget::EsquemaConnectionWidget(EsquemaNodeWidget* fromNode, int fromPin,
                                                 EsquemaNodeWidget* toNode, int toPin)
    : m_fromNode(fromNode)
    , m_toNode(toNode)
    , m_fromPin(fromPin)
    , m_toPin(toPin)
{
    setPen(QPen(QColor(200, 200, 200), 2));
    setZValue(-1);
    updatePath();
}

void EsquemaConnectionWidget::updatePath()
{
    if (!m_fromNode || !m_toNode) return;
    
    QPointF start = m_fromNode->mapToScene(m_fromNode->getOutputPinPos(m_fromPin));
    QPointF end = m_toNode->mapToScene(m_toNode->getInputPinPos(m_toPin));
    
    QPainterPath path;
    path.moveTo(start);
    
    qreal dx = end.x() - start.x();
    qreal offset = qAbs(dx) * 0.5;
    if (offset < 50) offset = 50;
    
    QPointF c1(start.x() + offset, start.y());
    QPointF c2(end.x() - offset, end.y());
    
    path.cubicTo(c1, c2, end);
    setPath(path);
}
