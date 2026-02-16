#pragma once

#include <QGraphicsPathItem>
#include <QPainter>

class EsquemaNodeWidget;

class EsquemaConnectionWidget : public QGraphicsPathItem {
public:
    EsquemaConnectionWidget(EsquemaNodeWidget* fromNode, int fromPin, 
                           EsquemaNodeWidget* toNode, int toPin);
    
    void updatePath();
    
    EsquemaNodeWidget* getFromNode() const { return m_fromNode; }
    EsquemaNodeWidget* getToNode() const { return m_toNode; }
    int getFromPin() const { return m_fromPin; }
    int getToPin() const { return m_toPin; }

private:
    EsquemaNodeWidget* m_fromNode;
    EsquemaNodeWidget* m_toNode;
    int m_fromPin;
    int m_toPin;
};
