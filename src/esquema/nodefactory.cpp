/////////////////////////////////////////////////////////////////////////////
// nodefactory.cpp                                                         //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#include "esquema/nodefactory.h"
#include "esquema/nodes/mathnode.h"
#include "esquema/nodes/logicnode.h"
#include "esquema/nodes/enginenode.h"
#include <QJsonObject>

namespace DabozzEngine {
namespace Esquema {

bool NodeFactory::s_initialized = false;
QMap<QString, QString> NodeFactory::s_nodeCategories;
QMap<QString, QString> NodeFactory::s_nodeDescriptions;

std::shared_ptr<Node> NodeFactory::createNode(int id, const QString& nodeType)
{
    initializeNodeTypes();
    
    // Math nodes
    if (nodeType == "Add") return std::make_shared<AddNode>(id);
    if (nodeType == "Subtract") return std::make_shared<SubtractNode>(id);
    if (nodeType == "Multiply") return std::make_shared<MultiplyNode>(id);
    if (nodeType == "Divide") return std::make_shared<DivideNode>(id);
    if (nodeType == "Sin") return std::make_shared<SinNode>(id);
    if (nodeType == "Cos") return std::make_shared<CosNode>(id);
    if (nodeType == "Sqrt") return std::make_shared<SqrtNode>(id);
    if (nodeType == "Abs") return std::make_shared<AbsNode>(id);
    if (nodeType == "Clamp") return std::make_shared<ClampNode>(id);
    if (nodeType == "Lerp") return std::make_shared<LerpNode>(id);
    
    // Logic nodes
    if (nodeType == "Branch") return std::make_shared<BranchNode>(id);
    if (nodeType == "And") return std::make_shared<AndNode>(id);
    if (nodeType == "Or") return std::make_shared<OrNode>(id);
    if (nodeType == "Not") return std::make_shared<NotNode>(id);
    if (nodeType == "Equal") return std::make_shared<CompareNode>(id, CompareNode::CompareType::Equal);
    if (nodeType == "NotEqual") return std::make_shared<CompareNode>(id, CompareNode::CompareType::NotEqual);
    if (nodeType == "Greater") return std::make_shared<CompareNode>(id, CompareNode::CompareType::Greater);
    if (nodeType == "Less") return std::make_shared<CompareNode>(id, CompareNode::CompareType::Less);
    if (nodeType == "ForLoop") return std::make_shared<ForLoopNode>(id);
    if (nodeType == "WhileLoop") return std::make_shared<WhileLoopNode>(id);
    
    // Engine nodes
    if (nodeType == "CreateEntity") return std::make_shared<CreateEntityNode>(id);
    if (nodeType == "DestroyEntity") return std::make_shared<DestroyEntityNode>(id);
    if (nodeType == "GetEntity") return std::make_shared<GetEntityNode>(id);
    if (nodeType == "GetPosition") return std::make_shared<GetPositionNode>(id);
    if (nodeType == "SetPosition") return std::make_shared<SetPositionNode>(id);
    if (nodeType == "GetRotation") return std::make_shared<GetRotationNode>(id);
    if (nodeType == "SetRotation") return std::make_shared<SetRotationNode>(id);
    if (nodeType == "GetKeyPressed") return std::make_shared<GetKeyPressedNode>(id);
    if (nodeType == "GetMousePosition") return std::make_shared<GetMousePositionNode>(id);
    if (nodeType == "Print") return std::make_shared<PrintNode>(id);
    if (nodeType == "Log") return std::make_shared<LogNode>(id);
    if (nodeType == "GetDeltaTime") return std::make_shared<GetDeltaTimeNode>(id);
    if (nodeType == "Delay") return std::make_shared<DelayNode>(id);
    
    // Basic nodes
    if (nodeType == "Function") return std::make_shared<FunctionNode>(id, "CustomFunction");
    if (nodeType == "Variable") return std::make_shared<VariableNode>(id, "Variable", PinType::Float);
    
    return nullptr;
}

std::shared_ptr<Node> NodeFactory::createEventNode(int id, const QString& eventName)
{
    return std::make_shared<EventNode>(id, eventName);
}

std::shared_ptr<Node> NodeFactory::createConstantNode(int id, PinType type, const QVariant& value)
{
    return std::make_shared<ConstantNode>(id, type, value);
}

QStringList NodeFactory::getAvailableNodeTypes()
{
    initializeNodeTypes();
    return s_nodeCategories.keys();
}

QStringList NodeFactory::getEventNodeTypes()
{
    return QStringList() << "Start" << "Update" << "OnCollision" << "OnKeyPress" << "OnMouseClick";
}

QStringList NodeFactory::getMathNodeTypes()
{
    return QStringList() << "Add" << "Subtract" << "Multiply" << "Divide" 
                        << "Sin" << "Cos" << "Sqrt" << "Abs" << "Clamp" << "Lerp";
}

QStringList NodeFactory::getLogicNodeTypes()
{
    return QStringList() << "Branch" << "And" << "Or" << "Not" 
                        << "Equal" << "NotEqual" << "Greater" << "Less"
                        << "ForLoop" << "WhileLoop";
}

QStringList NodeFactory::getEngineNodeTypes()
{
    return QStringList() << "CreateEntity" << "DestroyEntity" << "GetEntity"
                        << "GetPosition" << "SetPosition" << "GetRotation" << "SetRotation"
                        << "GetKeyPressed" << "GetMousePosition" << "Print" << "Log"
                        << "GetDeltaTime" << "Delay";
}

QString NodeFactory::getNodeCategory(const QString& nodeType)
{
    initializeNodeTypes();
    return s_nodeCategories.value(nodeType, "Unknown");
}

QStringList NodeFactory::getNodeTypesInCategory(const QString& category)
{
    initializeNodeTypes();
    QStringList types;
    for (auto it = s_nodeCategories.begin(); it != s_nodeCategories.end(); ++it) {
        if (it.value() == category) {
            types << it.key();
        }
    }
    return types;
}

QString NodeFactory::getNodeDescription(const QString& nodeType)
{
    initializeNodeTypes();
    return s_nodeDescriptions.value(nodeType, "No description available");
}

NodeType NodeFactory::getNodeTypeEnum(const QString& nodeType)
{
    QString category = getNodeCategory(nodeType);
    if (category == "Math") return NodeType::Math;
    if (category == "Logic") return NodeType::Logic;
    if (category == "Engine") return NodeType::Engine;
    if (category == "Flow") return NodeType::Flow;
    if (category == "Event") return NodeType::Event;
    if (category == "Function") return NodeType::Function;
    if (category == "Variable") return NodeType::Variable;
    if (category == "Constant") return NodeType::Constant;
    return NodeType::Function;
}

std::shared_ptr<Node> NodeFactory::createNodeFromJson(const QJsonObject& json)
{
    QString nodeType = json["name"].toString();
    int id = json["id"].toInt();
    
    auto node = createNode(id, nodeType);
    if (node) {
        node->setPosition(QPointF(json["posX"].toDouble(), json["posY"].toDouble()));
    }
    
    return node;
}

void NodeFactory::initializeNodeTypes()
{
    if (s_initialized) return;
    
    // Math nodes
    s_nodeCategories["Add"] = "Math";
    s_nodeCategories["Subtract"] = "Math";
    s_nodeCategories["Multiply"] = "Math";
    s_nodeCategories["Divide"] = "Math";
    s_nodeCategories["Sin"] = "Math";
    s_nodeCategories["Cos"] = "Math";
    s_nodeCategories["Sqrt"] = "Math";
    s_nodeCategories["Abs"] = "Math";
    s_nodeCategories["Clamp"] = "Math";
    s_nodeCategories["Lerp"] = "Math";
    
    // Logic nodes
    s_nodeCategories["Branch"] = "Flow";
    s_nodeCategories["And"] = "Logic";
    s_nodeCategories["Or"] = "Logic";
    s_nodeCategories["Not"] = "Logic";
    s_nodeCategories["Equal"] = "Logic";
    s_nodeCategories["NotEqual"] = "Logic";
    s_nodeCategories["Greater"] = "Logic";
    s_nodeCategories["Less"] = "Logic";
    s_nodeCategories["ForLoop"] = "Flow";
    s_nodeCategories["WhileLoop"] = "Flow";
    
    // Engine nodes
    s_nodeCategories["CreateEntity"] = "Engine";
    s_nodeCategories["DestroyEntity"] = "Engine";
    s_nodeCategories["GetEntity"] = "Engine";
    s_nodeCategories["GetPosition"] = "Engine";
    s_nodeCategories["SetPosition"] = "Engine";
    s_nodeCategories["GetRotation"] = "Engine";
    s_nodeCategories["SetRotation"] = "Engine";
    s_nodeCategories["GetKeyPressed"] = "Engine";
    s_nodeCategories["GetMousePosition"] = "Engine";
    s_nodeCategories["Print"] = "Engine";
    s_nodeCategories["Log"] = "Engine";
    s_nodeCategories["GetDeltaTime"] = "Engine";
    s_nodeCategories["Delay"] = "Engine";
    
    // Descriptions
    s_nodeDescriptions["Add"] = "Adds two numbers together";
    s_nodeDescriptions["Subtract"] = "Subtracts B from A";
    s_nodeDescriptions["Multiply"] = "Multiplies two numbers";
    s_nodeDescriptions["Divide"] = "Divides A by B";
    s_nodeDescriptions["Sin"] = "Calculates sine of input value";
    s_nodeDescriptions["Cos"] = "Calculates cosine of input value";
    s_nodeDescriptions["Branch"] = "Conditional execution based on boolean input";
    s_nodeDescriptions["Print"] = "Prints text to console";
    s_nodeDescriptions["CreateEntity"] = "Creates a new entity in the world";
    s_nodeDescriptions["GetPosition"] = "Gets the position of an entity";
    
    s_initialized = true;
}

}
}