/////////////////////////////////////////////////////////////////////////////
// node.cpp                                                                //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#include "esquema/node.h"

namespace DabozzEngine {
namespace Esquema {

Node::Node(int id, NodeType type, const QString& name)
    : m_id(id)
    , m_type(type)
    , m_name(name)
    , m_position(0, 0)
{
}

void Node::addInputPin(const QString& name, PinType type, const QVariant& defaultValue)
{
    Pin pin;
    pin.name = name;
    pin.type = type;
    pin.defaultValue = defaultValue;
    pin.isInput = true;
    pin.nodeId = m_id;
    pin.pinIndex = m_inputPins.size();
    m_inputPins.append(pin);
}

void Node::addOutputPin(const QString& name, PinType type)
{
    Pin pin;
    pin.name = name;
    pin.type = type;
    pin.isInput = false;
    pin.nodeId = m_id;
    pin.pinIndex = m_outputPins.size();
    m_outputPins.append(pin);
}

// Event Node Implementation
EventNode::EventNode(int id, const QString& eventName)
    : Node(id, NodeType::Event, eventName)
{
    addOutputPin("Exec", PinType::Exec);
}

QString EventNode::generateLuaCode() const
{
    return QString("function %1()\n").arg(m_name);
}

QString EventNode::generateAngelScriptCode() const
{
    return QString("void %1()\n{\n").arg(m_name);
}

// Function Node Implementation
FunctionNode::FunctionNode(int id, const QString& functionName)
    : Node(id, NodeType::Function, functionName)
{
    addInputPin("Exec", PinType::Exec);
    addOutputPin("Exec", PinType::Exec);
}

QString FunctionNode::generateLuaCode() const
{
    return QString("    %1()\n").arg(m_name);
}

QString FunctionNode::generateAngelScriptCode() const
{
    return QString("    %1();\n").arg(m_name);
}

// Variable Node Implementation
VariableNode::VariableNode(int id, const QString& varName, PinType varType)
    : Node(id, NodeType::Variable, varName)
    , m_varType(varType)
{
    addOutputPin(varName, varType);
}

QString VariableNode::generateLuaCode() const
{
    return QString("local %1").arg(m_name);
}

QString VariableNode::generateAngelScriptCode() const
{
    QString typeStr;
    switch (m_varType) {
        case PinType::Int: typeStr = "int"; break;
        case PinType::Float: typeStr = "float"; break;
        case PinType::Bool: typeStr = "bool"; break;
        case PinType::String: typeStr = "string"; break;
        case PinType::Vector3: typeStr = "Vector3"; break;
        case PinType::Entity: typeStr = "Entity@"; break;
        default: typeStr = "auto"; break;
    }
    return QString("%1 %2;").arg(typeStr, m_name);
}

// Constant Node Implementation
ConstantNode::ConstantNode(int id, PinType type, const QVariant& value)
    : Node(id, NodeType::Constant, "Constant")
    , m_value(value)
    , m_valueType(type)
{
    addOutputPin("Value", type);
}

QString ConstantNode::generateLuaCode() const
{
    switch (m_valueType) {
        case PinType::String:
            return QString("\"%1\"").arg(m_value.toString());
        case PinType::Bool:
            return m_value.toBool() ? "true" : "false";
        default:
            return m_value.toString();
    }
}

QString ConstantNode::generateAngelScriptCode() const
{
    switch (m_valueType) {
        case PinType::String:
            return QString("\"%1\"").arg(m_value.toString());
        case PinType::Bool:
            return m_value.toBool() ? "true" : "false";
        case PinType::Float:
            return QString("%1f").arg(m_value.toFloat());
        default:
            return m_value.toString();
    }
}

// Legacy Operator Node Implementation
OperatorNode::OperatorNode(int id, OpType op)
    : Node(id, NodeType::Operator, "Operator")
    , m_opType(op)
{
    addInputPin("A", PinType::Float);
    addInputPin("B", PinType::Float);
    addOutputPin("Result", PinType::Float);
}

QString OperatorNode::generateLuaCode() const
{
    QString opStr;
    switch (m_opType) {
        case OpType::Add: opStr = "+"; break;
        case OpType::Subtract: opStr = "-"; break;
        case OpType::Multiply: opStr = "*"; break;
        case OpType::Divide: opStr = "/"; break;
        case OpType::Equal: opStr = "=="; break;
        case OpType::NotEqual: opStr = "~="; break;
        case OpType::Greater: opStr = ">"; break;
        case OpType::Less: opStr = "<"; break;
    }
    return QString("A %1 B").arg(opStr);
}

QString OperatorNode::generateAngelScriptCode() const
{
    QString opStr;
    switch (m_opType) {
        case OpType::Add: opStr = "+"; break;
        case OpType::Subtract: opStr = "-"; break;
        case OpType::Multiply: opStr = "*"; break;
        case OpType::Divide: opStr = "/"; break;
        case OpType::Equal: opStr = "=="; break;
        case OpType::NotEqual: opStr = "!="; break;
        case OpType::Greater: opStr = ">"; break;
        case OpType::Less: opStr = "<"; break;
    }
    return QString("A %1 B").arg(opStr);
}

}
}
