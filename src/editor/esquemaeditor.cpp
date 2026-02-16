#include "editor/esquemaeditor.h"
#include "editor/esquemanode.h"
#include "editor/esquemaconnection.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QMouseEvent>

EsquemaEditor::EsquemaEditor(QWidget* parent)
    : QWidget(parent)
    , m_graph(new DabozzEngine::Esquema::Graph())
    , m_nextNodePos(50, 50)
    , m_isDraggingConnection(false)
    , m_dragFromNode(nullptr)
    , m_dragFromPin(-1)
    , m_tempConnectionLine(nullptr)
{
    setupUI();
    applyDarkTheme();
    m_view->viewport()->installEventFilter(this);
}

EsquemaEditor::~EsquemaEditor()
{
    delete m_graph;
}

void EsquemaEditor::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    setupToolbar();
    mainLayout->addWidget(m_toolbar);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-5000, -5000, 10000, 10000);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    mainLayout->addWidget(m_view);
}

void EsquemaEditor::setupToolbar()
{
    m_toolbar = new QToolBar(this);

    QPushButton* newBtn = new QPushButton("New", this);
    connect(newBtn, &QPushButton::clicked, this, &EsquemaEditor::newGraph);
    m_toolbar->addWidget(newBtn);

    m_toolbar->addSeparator();

    QPushButton* eventBtn = new QPushButton("Event", this);
    connect(eventBtn, &QPushButton::clicked, this, &EsquemaEditor::onAddEventNode);
    m_toolbar->addWidget(eventBtn);

    QPushButton* funcBtn = new QPushButton("Function", this);
    connect(funcBtn, &QPushButton::clicked, this, &EsquemaEditor::onAddFunctionNode);
    m_toolbar->addWidget(funcBtn);

    QPushButton* printBtn = new QPushButton("Print", this);
    connect(printBtn, &QPushButton::clicked, this, &EsquemaEditor::onAddPrintNode);
    m_toolbar->addWidget(printBtn);

    QPushButton* branchBtn = new QPushButton("Branch", this);
    connect(branchBtn, &QPushButton::clicked, this, &EsquemaEditor::onAddBranchNode);
    m_toolbar->addWidget(branchBtn);

    QPushButton* varBtn = new QPushButton("Variable", this);
    connect(varBtn, &QPushButton::clicked, this, &EsquemaEditor::onAddVariableNode);
    m_toolbar->addWidget(varBtn);

    QPushButton* opBtn = new QPushButton("Operator", this);
    connect(opBtn, &QPushButton::clicked, this, &EsquemaEditor::onAddOperatorNode);
    m_toolbar->addWidget(opBtn);

    m_toolbar->addSeparator();

    QPushButton* genBtn = new QPushButton("Generate Code", this);
    connect(genBtn, &QPushButton::clicked, this, &EsquemaEditor::generateCode);
    m_toolbar->addWidget(genBtn);
}

void EsquemaEditor::applyDarkTheme()
{
    setStyleSheet(R"(
        QWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
        }
        QToolBar {
            background-color: #2d2d30;
            border: none;
            padding: 5px;
        }
        QPushButton {
            background-color: #3e3e42;
            border: 1px solid #555;
            padding: 5px 15px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: #505050;
        }
        QGraphicsView {
            background-color: #252526;
            border: none;
        }
    )");
}

void EsquemaEditor::newGraph()
{
    m_graph->clear();
    m_scene->clear();
    m_nodeWidgets.clear();
    m_connectionWidgets.clear();
    m_nextNodePos = QPointF(50, 50);
}

void EsquemaEditor::loadGraph(const QString& filepath)
{
    QMessageBox::information(this, "Load", "Load functionality coming soon!");
}

void EsquemaEditor::saveGraph(const QString& filepath)
{
    QMessageBox::information(this, "Save", "Save functionality coming soon!");
}

void EsquemaEditor::generateCode()
{
    QString luaCode = m_graph->generateLuaCode();
    QString asCode = m_graph->generateAngelScriptCode();
    
    QString combined = "=== Lua Code ===\n" + luaCode + "\n\n=== AngelScript Code ===\n" + asCode;
    QMessageBox::information(this, "Generated Code", combined);
}

void EsquemaEditor::updateConnections()
{
    for (auto* conn : m_connectionWidgets) {
        conn->updatePath();
    }
}

void EsquemaEditor::onAddEventNode()
{
    auto node = std::make_shared<DabozzEngine::Esquema::EventNode>(0, "Start");
    int id = m_graph->addNode(node);
    
    EsquemaNodeWidget* widget = new EsquemaNodeWidget(node);
    widget->setPos(m_nextNodePos);
    m_scene->addItem(widget);
    m_nodeWidgets[id] = widget;
    
    m_nextNodePos += QPointF(0, 120);
}

void EsquemaEditor::onAddFunctionNode()
{
    auto node = std::make_shared<DabozzEngine::Esquema::FunctionNode>(0, "MyFunction");
    int id = m_graph->addNode(node);
    
    EsquemaNodeWidget* widget = new EsquemaNodeWidget(node);
    widget->setPos(m_nextNodePos);
    m_scene->addItem(widget);
    m_nodeWidgets[id] = widget;
    
    m_nextNodePos += QPointF(0, 120);
}

void EsquemaEditor::onAddPrintNode()
{
    auto node = std::make_shared<DabozzEngine::Esquema::FunctionNode>(0, "Print");
    node->addInputPin("Message", DabozzEngine::Esquema::PinType::String);
    int id = m_graph->addNode(node);
    
    EsquemaNodeWidget* widget = new EsquemaNodeWidget(node);
    widget->setPos(m_nextNodePos);
    m_scene->addItem(widget);
    m_nodeWidgets[id] = widget;
    
    m_nextNodePos += QPointF(0, 120);
}

void EsquemaEditor::onAddBranchNode()
{
    auto node = std::make_shared<DabozzEngine::Esquema::FunctionNode>(0, "Branch");
    node->addInputPin("Condition", DabozzEngine::Esquema::PinType::Bool);
    node->addOutputPin("True", DabozzEngine::Esquema::PinType::Exec);
    node->addOutputPin("False", DabozzEngine::Esquema::PinType::Exec);
    int id = m_graph->addNode(node);
    
    EsquemaNodeWidget* widget = new EsquemaNodeWidget(node);
    widget->setPos(m_nextNodePos);
    m_scene->addItem(widget);
    m_nodeWidgets[id] = widget;
    
    m_nextNodePos += QPointF(0, 120);
}

void EsquemaEditor::onAddVariableNode()
{
    auto node = std::make_shared<DabozzEngine::Esquema::VariableNode>(0, "myVar", DabozzEngine::Esquema::PinType::Float);
    int id = m_graph->addNode(node);
    
    EsquemaNodeWidget* widget = new EsquemaNodeWidget(node);
    widget->setPos(m_nextNodePos);
    m_scene->addItem(widget);
    m_nodeWidgets[id] = widget;
    
    m_nextNodePos += QPointF(0, 120);
}

void EsquemaEditor::onAddOperatorNode()
{
    auto node = std::make_shared<DabozzEngine::Esquema::OperatorNode>(0, DabozzEngine::Esquema::OperatorNode::OpType::Add);
    int id = m_graph->addNode(node);
    
    EsquemaNodeWidget* widget = new EsquemaNodeWidget(node);
    widget->setPos(m_nextNodePos);
    m_scene->addItem(widget);
    m_nodeWidgets[id] = widget;
    
    m_nextNodePos += QPointF(0, 120);
}

bool EsquemaEditor::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_view->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
            
            if (mouseEvent->button() == Qt::MiddleButton) {
                m_view->setDragMode(QGraphicsView::ScrollHandDrag);
                return false;
            }
            
            if (mouseEvent->button() == Qt::LeftButton) {
                QList<QGraphicsItem*> items = m_scene->items(scenePos);
                
                for (QGraphicsItem* item : items) {
                    EsquemaNodeWidget* nodeWidget = dynamic_cast<EsquemaNodeWidget*>(item);
                    
                    if (nodeWidget) {
                        QPointF localPos = nodeWidget->mapFromScene(scenePos);
                        int outputPin = nodeWidget->getOutputPinAt(localPos);
                        
                        if (outputPin >= 0) {
                            m_isDraggingConnection = true;
                            m_dragFromNode = nodeWidget;
                            m_dragFromPin = outputPin;
                            
                            QPointF pinPos = nodeWidget->mapToScene(nodeWidget->getOutputPinPos(outputPin));
                            m_tempConnectionLine = m_scene->addLine(QLineF(pinPos, scenePos), 
                                                                    QPen(QColor(100, 200, 255), 3));
                            m_tempConnectionLine->setZValue(1000);
                            return true;
                        }
                        break;
                    }
                }
            }
        }
        else if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
            
            if (m_isDraggingConnection && m_tempConnectionLine) {
                QPointF startPos = m_dragFromNode->mapToScene(
                    m_dragFromNode->getOutputPinPos(m_dragFromPin));
                m_tempConnectionLine->setLine(QLineF(startPos, scenePos));
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            
            if (mouseEvent->button() == Qt::MiddleButton) {
                m_view->setDragMode(QGraphicsView::NoDrag);
                return false;
            }
            
            if (mouseEvent->button() == Qt::LeftButton && m_isDraggingConnection) {
                QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
                QList<QGraphicsItem*> items = m_scene->items(scenePos);
                
                EsquemaNodeWidget* toNode = nullptr;
                for (QGraphicsItem* item : items) {
                    toNode = dynamic_cast<EsquemaNodeWidget*>(item);
                    if (toNode && toNode != m_dragFromNode) {
                        break;
                    }
                    toNode = nullptr;
                }
                
                if (toNode) {
                    QPointF localPos = toNode->mapFromScene(scenePos);
                    int inputPin = toNode->getInputPinAt(localPos);
                    
                    if (inputPin >= 0) {
                        int fromNodeId = -1;
                        int toNodeId = -1;
                        
                        for (auto it = m_nodeWidgets.begin(); it != m_nodeWidgets.end(); ++it) {
                            if (it.value() == m_dragFromNode) fromNodeId = it.key();
                            if (it.value() == toNode) toNodeId = it.key();
                        }
                        
                        if (fromNodeId >= 0 && toNodeId >= 0) {
                            m_graph->addConnection(fromNodeId, m_dragFromPin, toNodeId, inputPin);
                            
                            EsquemaConnectionWidget* conn = new EsquemaConnectionWidget(
                                m_dragFromNode, m_dragFromPin, toNode, inputPin);
                            m_scene->addItem(conn);
                            m_connectionWidgets.append(conn);
                        }
                    }
                }
                
                if (m_tempConnectionLine) {
                    m_scene->removeItem(m_tempConnectionLine);
                    delete m_tempConnectionLine;
                    m_tempConnectionLine = nullptr;
                }
                
                m_isDraggingConnection = false;
                m_dragFromNode = nullptr;
                m_dragFromPin = -1;
                return true;
            } 
        }
    }
    
    return QWidget::eventFilter(obj, event);
}
