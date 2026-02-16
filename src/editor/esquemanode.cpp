#include "editor/esquemanode.h"
#include "editor/esquemaconnection.h"
#include <QGraphicsScene>

EsquemaNodeWidget::EsquemaNodeWidget(std::shared_ptr<DabozzEngine::Esquema::Node> node)
    : m_node(node)
    , m_width(180)
    , m_height(80)
    , m_pinRadius(6)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    
    int maxPins = qMax(m_node->getInputPins().size(), m_node->getOutputPins().size());
    if (maxPins > 2) {
        m_height = 40 + maxPins * 25;
    }
}

QRectF EsquemaNodeWidget::boundingRect() const
{
    return QRectF(-10, -10, m_width + 20, m_height + 20);
}

void EsquemaNodeWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    QColor bgColor;
    switch (m_node->getType()) {
        case DabozzEngine::Esquema::NodeType::Event:
            bgColor = QColor(80, 60, 100);
            break;
        case DabozzEngine::Esquema::NodeType::Function:
            bgColor = QColor(60, 80, 100);
            break;
        case DabozzEngine::Esquema::NodeType::Variable:
            bgColor = QColor(60, 100, 80);
            break;
        case DabozzEngine::Esquema::NodeType::Operator:
            bgColor = QColor(100, 80, 60);
            break;
        default:
            bgColor = QColor(70, 70, 70);
    }
    
    if (isSelected()) {
        painter->setPen(QPen(QColor(100, 150, 255), 3));
    } else {
        painter->setPen(QPen(QColor(200, 200, 200), 2));
    }
    
    painter->setBrush(bgColor);
    painter->drawRoundedRect(0, 0, m_width, m_height, 5, 5);
    
    painter->setPen(Qt::white);
    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    painter->drawText(QRectF(10, 10, m_width - 20, 25), Qt::AlignCenter, m_node->getName());
    
    font.setBold(false);
    font.setPointSize(9);
    painter->setFont(font);
    
    const auto& inputPins = m_node->getInputPins();
    for (int i = 0; i < inputPins.size(); ++i) {
        QPointF pinPos = getInputPinPos(i);
        
        QColor pinColor = (inputPins[i].type == DabozzEngine::Esquema::PinType::Exec) 
            ? QColor(255, 255, 255) : QColor(150, 200, 255);
        
        painter->setBrush(pinColor);
        painter->setPen(Qt::white);
        painter->drawEllipse(pinPos, m_pinRadius, m_pinRadius);
        
        painter->setPen(Qt::white);
        painter->drawText(QRectF(pinPos.x() + 15, pinPos.y() - 10, 100, 20), 
                         Qt::AlignLeft | Qt::AlignVCenter, inputPins[i].name);
    }
    
    const auto& outputPins = m_node->getOutputPins();
    for (int i = 0; i < outputPins.size(); ++i) {
        QPointF pinPos = getOutputPinPos(i);
        
        QColor pinColor = (outputPins[i].type == DabozzEngine::Esquema::PinType::Exec) 
            ? QColor(255, 255, 255) : QColor(255, 200, 150);
        
        painter->setBrush(pinColor);
        painter->setPen(Qt::white);
        painter->drawEllipse(pinPos, m_pinRadius, m_pinRadius);
        
        painter->setPen(Qt::white);
        painter->drawText(QRectF(pinPos.x() - 115, pinPos.y() - 10, 100, 20), 
                         Qt::AlignRight | Qt::AlignVCenter, outputPins[i].name);
    }
}

QPointF EsquemaNodeWidget::getInputPinPos(int index) const
{
    qreal y = 40 + index * 25;
    return QPointF(0, y);
}

QPointF EsquemaNodeWidget::getOutputPinPos(int index) const
{
    qreal y = 40 + index * 25;
    return QPointF(m_width, y);
}

int EsquemaNodeWidget::getInputPinAt(const QPointF& pos) const
{
    const auto& pins = m_node->getInputPins();
    for (int i = 0; i < pins.size(); ++i) {
        QPointF pinPos = getInputPinPos(i);
        qreal dist = QLineF(pos, pinPos).length();
        if (dist < m_pinRadius + 10) {
            return i;
        }
    }
    return -1;
}

int EsquemaNodeWidget::getOutputPinAt(const QPointF& pos) const
{
    const auto& pins = m_node->getOutputPins();
    for (int i = 0; i < pins.size(); ++i) {
        QPointF pinPos = getOutputPinPos(i);
        qreal dist = QLineF(pos, pinPos).length();
        if (dist < m_pinRadius + 10) {
            return i;
        }
    }
    return -1;
}

void EsquemaNodeWidget::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mousePressEvent(event);
}

void EsquemaNodeWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseMoveEvent(event);
    
    if (scene()) {
        QList<QGraphicsItem*> items = scene()->items();
        for (QGraphicsItem* item : items) {
            EsquemaConnectionWidget* conn = dynamic_cast<EsquemaConnectionWidget*>(item);
            if (conn && (conn->getFromNode() == this || conn->getToNode() == this)) {
                conn->updatePath();
            }
        }
    }
}

void EsquemaNodeWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
}
