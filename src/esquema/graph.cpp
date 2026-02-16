/////////////////////////////////////////////////////////////////////////////
// graph.cpp                                                               //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#include "esquema/graph.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSet>

namespace DabozzEngine {
namespace Esquema {

// Connection serialization
QJsonObject Connection::toJson() const
{
    QJsonObject obj;
    obj["fromNodeId"] = fromNodeId;
    obj["fromPinIndex"] = fromPinIndex;
    obj["toNodeId"] = toNodeId;
    obj["toPinIndex"] = toPinIndex;
    return obj;
}

Connection Connection::fromJson(const QJsonObject& json)
{
    Connection conn;
    conn.fromNodeId = json["fromNodeId"].toInt();
    conn.fromPinIndex = json["fromPinIndex"].toInt();
    conn.toNodeId = json["toNodeId"].toInt();
    conn.toPinIndex = json["toPinIndex"].toInt();
    return conn;
}

Graph::Graph()
    : m_nextNodeId(1)
{
}

Graph::~Graph()
{
}

int Graph::addNode(std::shared_ptr<Node> node)
{
    int id = m_nextNodeId++;
    m_nodes[id] = node;
    return id;
}

void Graph::removeNode(int nodeId)
{
    m_nodes.remove(nodeId);
    
    // Remove all connections involving this node
    for (int i = m_connections.size() - 1; i >= 0; --i) {
        const Connection& conn = m_connections[i];
        if (conn.fromNodeId == nodeId || conn.toNodeId == nodeId) {
            m_connections.removeAt(i);
        }
    }
}

Node* Graph::getNode(int nodeId) const
{
    auto it = m_nodes.find(nodeId);
    return it != m_nodes.end() ? it.value().get() : nullptr;
}

void Graph::addConnection(int fromNode, int fromPin, int toNode, int toPin)
{
    if (hasConnection(fromNode, fromPin, toNode, toPin)) {
        return;
    }
    
    Connection conn;
    conn.fromNodeId = fromNode;
    conn.fromPinIndex = fromPin;
    conn.toNodeId = toNode;
    conn.toPinIndex = toPin;
    m_connections.append(conn);
}

void Graph::removeConnection(int fromNode, int fromPin, int toNode, int toPin)
{
    for (int i = m_connections.size() - 1; i >= 0; --i) {
        const Connection& conn = m_connections[i];
        if (conn.fromNodeId == fromNode && 
            conn.fromPinIndex == fromPin &&
            conn.toNodeId == toNode && 
            conn.toPinIndex == toPin) {
            m_connections.removeAt(i);
            break;
        }
    }
}

bool Graph::hasConnection(int fromNode, int fromPin, int toNode, int toPin) const
{
    for (const Connection& conn : m_connections) {
        if (conn.fromNodeId == fromNode && conn.fromPinIndex == fromPin &&
            conn.toNodeId == toNode && conn.toPinIndex == toPin) {
            return true;
        }
    }
    return false;
}

QString Graph::generateLuaCode() const
{
    QString code;
    QSet<int> visitedNodes;
    
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        Node* node = it.value().get();
        if (node->getType() == NodeType::Event) {
            code += node->generateLuaCode();
            code += generateLuaExecutionChain(it.key(), 0, visitedNodes);
            code += "end\n\n";
            visitedNodes.clear();
        }
    }
    
    return code.isEmpty() ? "-- No event nodes found\n" : code;
}

QString Graph::generateAngelScriptCode() const
{
    QString code;
    QSet<int> visitedNodes;
    
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        Node* node = it.value().get();
        if (node->getType() == NodeType::Event) {
            code += node->generateAngelScriptCode();
            code += generateAngelScriptExecutionChain(it.key(), 0, visitedNodes);
            code += "}\n\n";
            visitedNodes.clear();
        }
    }
    
    return code.isEmpty() ? "// No event nodes found\n" : code;
}

QString Graph::generateLuaExecutionChain(int nodeId, int fromPinIndex, QSet<int>& visitedNodes) const
{
    QString code;
    
    if (visitedNodes.contains(nodeId)) {
        return "    -- Circular reference detected\n";
    }
    visitedNodes.insert(nodeId);
    
    for (const Connection& conn : m_connections) {
        if (conn.fromNodeId == nodeId && conn.fromPinIndex == fromPinIndex) {
            Node* nextNode = getNode(conn.toNodeId);
            if (!nextNode) continue;
            
            if (nextNode->getType() == NodeType::Flow && nextNode->getName() == "Branch") {
                code += "    if " + resolveLuaDataFlow(conn.toNodeId, 1, visitedNodes) + " then\n";
                code += generateLuaExecutionChain(conn.toNodeId, 1, visitedNodes);
                code += "    else\n";
                code += generateLuaExecutionChain(conn.toNodeId, 2, visitedNodes);
                code += "    end\n";
            } else {
                code += "    " + nextNode->generateLuaCode() + "\n";
                
                const auto& outPins = nextNode->getOutputPins();
                for (int i = 0; i < outPins.size(); ++i) {
                    if (outPins[i].type == PinType::Exec) {
                        code += generateLuaExecutionChain(conn.toNodeId, i, visitedNodes);
                    }
                }
            }
        }
    }
    
    visitedNodes.remove(nodeId);
    return code;
}

QString Graph::generateAngelScriptExecutionChain(int nodeId, int fromPinIndex, QSet<int>& visitedNodes) const
{
    QString code;
    
    if (visitedNodes.contains(nodeId)) {
        return "    // Circular reference detected\n";
    }
    visitedNodes.insert(nodeId);
    
    for (const Connection& conn : m_connections) {
        if (conn.fromNodeId == nodeId && conn.fromPinIndex == fromPinIndex) {
            Node* nextNode = getNode(conn.toNodeId);
            if (!nextNode) continue;
            
            if (nextNode->getType() == NodeType::Flow && nextNode->getName() == "Branch") {
                code += "    if (" + resolveAngelScriptDataFlow(conn.toNodeId, 1, visitedNodes) + ") {\n";
                code += generateAngelScriptExecutionChain(conn.toNodeId, 1, visitedNodes);
                code += "    } else {\n";
                code += generateAngelScriptExecutionChain(conn.toNodeId, 2, visitedNodes);
                code += "    }\n";
            } else {
                code += "    " + nextNode->generateAngelScriptCode() + "\n";
                
                const auto& outPins = nextNode->getOutputPins();
                for (int i = 0; i < outPins.size(); ++i) {
                    if (outPins[i].type == PinType::Exec) {
                        code += generateAngelScriptExecutionChain(conn.toNodeId, i, visitedNodes);
                    }
                }
            }
        }
    }
    
    visitedNodes.remove(nodeId);
    return code;
}

QString Graph::resolveLuaDataFlow(int nodeId, int pinIndex, QSet<int>& visitedNodes) const
{
    for (const Connection& conn : m_connections) {
        if (conn.toNodeId == nodeId && conn.toPinIndex == pinIndex) {
            Node* sourceNode = getNode(conn.fromNodeId);
            if (sourceNode) {
                return sourceNode->generateLuaCode();
            }
        }
    }
    
    Node* node = getNode(nodeId);
    if (node && pinIndex < node->getInputPins().size()) {
        const Pin& pin = node->getInputPins()[pinIndex];
        return pin.defaultValue.toString();
    }
    
    return "nil";
}

QString Graph::resolveAngelScriptDataFlow(int nodeId, int pinIndex, QSet<int>& visitedNodes) const
{
    for (const Connection& conn : m_connections) {
        if (conn.toNodeId == nodeId && conn.toPinIndex == pinIndex) {
            Node* sourceNode = getNode(conn.fromNodeId);
            if (sourceNode) {
                return sourceNode->generateAngelScriptCode();
            }
        }
    }
    
    Node* node = getNode(nodeId);
    if (node && pinIndex < node->getInputPins().size()) {
        const Pin& pin = node->getInputPins()[pinIndex];
        return pin.defaultValue.toString();
    }
    
    return "null";
}

QJsonObject Graph::toJson() const
{
    QJsonObject obj;
    
    QJsonArray nodesArray;
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        QJsonObject nodeObj;
        nodeObj["id"] = it.key();
        nodeObj["type"] = static_cast<int>(it.value()->getType());
        nodeObj["name"] = it.value()->getName();
        nodeObj["posX"] = it.value()->getPosition().x();
        nodeObj["posY"] = it.value()->getPosition().y();
        nodesArray.append(nodeObj);
    }
    obj["nodes"] = nodesArray;
    
    QJsonArray connectionsArray;
    for (const Connection& conn : m_connections) {
        connectionsArray.append(conn.toJson());
    }
    obj["connections"] = connectionsArray;
    
    obj["nextNodeId"] = m_nextNodeId;
    return obj;
}

bool Graph::fromJson(const QJsonObject& json)
{
    clear();
    m_nextNodeId = json["nextNodeId"].toInt();
    return true;
}

bool Graph::saveToFile(const QString& filePath) const
{
    QJsonDocument doc(toJson());
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        return true;
    }
    return false;
}

bool Graph::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        return fromJson(doc.object());
    }
    return false;
}

bool Graph::validateGraph() const
{
    return !hasCircularDependency();
}

QStringList Graph::getValidationErrors() const
{
    QStringList errors;
    
    if (hasCircularDependency()) {
        errors << "Circular dependency detected in graph";
    }
    
    for (const Connection& conn : m_connections) {
        Node* fromNode = getNode(conn.fromNodeId);
        Node* toNode = getNode(conn.toNodeId);
        
        if (fromNode && toNode) {
            if (conn.fromPinIndex < fromNode->getOutputPins().size() &&
                conn.toPinIndex < toNode->getInputPins().size()) {
                
                PinType outputType = fromNode->getOutputPins()[conn.fromPinIndex].type;
                PinType inputType = toNode->getInputPins()[conn.toPinIndex].type;
                
                if (!areTypesCompatible(outputType, inputType)) {
                    errors << QString("Type mismatch: %1 -> %2")
                        .arg(static_cast<int>(outputType))
                        .arg(static_cast<int>(inputType));
                }
            }
        }
    }
    
    return errors;
}

void Graph::clear()
{
    m_nodes.clear();
    m_connections.clear();
    m_nextNodeId = 1;
}

bool Graph::hasCircularDependency() const
{
    QSet<int> visited;
    QSet<int> recursionStack;
    
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (!visited.contains(it.key())) {
            if (hasCircularDependencyHelper(it.key(), visited, recursionStack)) {
                return true;
            }
        }
    }
    return false;
}

bool Graph::hasCircularDependencyHelper(int nodeId, QSet<int>& visited, QSet<int>& recursionStack) const
{
    visited.insert(nodeId);
    recursionStack.insert(nodeId);
    
    for (const Connection& conn : m_connections) {
        if (conn.fromNodeId == nodeId) {
            if (!visited.contains(conn.toNodeId)) {
                if (hasCircularDependencyHelper(conn.toNodeId, visited, recursionStack)) {
                    return true;
                }
            } else if (recursionStack.contains(conn.toNodeId)) {
                return true;
            }
        }
    }
    
    recursionStack.remove(nodeId);
    return false;
}

bool Graph::areTypesCompatible(PinType outputType, PinType inputType) const
{
    if (outputType == inputType) return true;
    if (inputType == PinType::Exec || outputType == PinType::Exec) return inputType == outputType;
    
    if ((outputType == PinType::Int && inputType == PinType::Float) ||
        (outputType == PinType::Float && inputType == PinType::Int)) {
        return true;
    }
    
    return false;
}

}
}