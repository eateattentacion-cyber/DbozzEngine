#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QToolBar>
#include <QMap>
#include "esquema/graph.h"

class EsquemaNodeWidget;
class EsquemaConnectionWidget;

class EsquemaEditor : public QWidget {
    Q_OBJECT

public:
    explicit EsquemaEditor(QWidget* parent = nullptr);
    ~EsquemaEditor();

    void newGraph();
    void loadGraph(const QString& filepath);
    void saveGraph(const QString& filepath);
    void generateCode();

private slots:
    void onAddEventNode();
    void onAddFunctionNode();
    void onAddVariableNode();
    void onAddOperatorNode();
    void onAddBranchNode();
    void onAddPrintNode();

private:
    void setupUI();
    void setupToolbar();
    void applyDarkTheme();
    void updateConnections();
    
    bool eventFilter(QObject* obj, QEvent* event) override;

    QGraphicsView* m_view;
    QGraphicsScene* m_scene;
    QToolBar* m_toolbar;
    
    DabozzEngine::Esquema::Graph* m_graph;
    QMap<int, EsquemaNodeWidget*> m_nodeWidgets;
    QVector<EsquemaConnectionWidget*> m_connectionWidgets;
    
    QPointF m_nextNodePos;
    
    bool m_isDraggingConnection;
    EsquemaNodeWidget* m_dragFromNode;
    int m_dragFromPin;
    QGraphicsLineItem* m_tempConnectionLine;
};
