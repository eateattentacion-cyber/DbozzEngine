/////////////////////////////////////////////////////////////////////////////
// graph.h                                                                 //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "esquema/node.h"
#include <QMap>
#include <QList>
#include <QSet>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

namespace DabozzEngine {
namespace Esquema {

struct Connection {
    int fromNodeId;
    int fromPinIndex;
    int toNodeId;
    int toPinIndex;
    
    QJsonObject toJson() const;
    static Connection fromJson(const QJsonObject& json);
};

class Graph {
public:
    Graph();
    ~Graph();

    int addNode(std::shared_ptr<Node> node);
    void removeNode(int nodeId);
    Node* getNode(int nodeId) const;
    
    void addConnection(int fromNode, int fromPin, int toNode, int toPin);
    void removeConnection(int fromNode, int fromPin, int toNode, int toPin);
    bool hasConnection(int fromNode, int fromPin, int toNode, int toPin) const;
    
    const QMap<int, std::shared_ptr<Node>>& getNodes() const { return m_nodes; }
    const QList<Connection>& getConnections() const { return m_connections; }
    
    QString generateLuaCode() const;
    QString generateAngelScriptCode() const;
    
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);
    bool saveToFile(const QString& filePath) const;
    bool loadFromFile(const QString& filePath);
    
    bool validateGraph() const;
    QStringList getValidationErrors() const;
    
    void clear();
    bool isEmpty() const { return m_nodes.isEmpty(); }
    int getNodeCount() const { return m_nodes.size(); }
    int getConnectionCount() const { return m_connections.size(); }

private:
    QString generateLuaExecutionChain(int nodeId, int fromPinIndex, QSet<int>& visitedNodes) const;
    QString generateAngelScriptExecutionChain(int nodeId, int fromPinIndex, QSet<int>& visitedNodes) const;
    
    QString resolveLuaDataFlow(int nodeId, int pinIndex, QSet<int>& visitedNodes) const;
    QString resolveAngelScriptDataFlow(int nodeId, int pinIndex, QSet<int>& visitedNodes) const;
    
    bool hasCircularDependency() const;
    bool hasCircularDependencyHelper(int nodeId, QSet<int>& visited, QSet<int>& recursionStack) const;
    bool areTypesCompatible(PinType outputType, PinType inputType) const;
    
    QMap<int, std::shared_ptr<Node>> m_nodes;
    QList<Connection> m_connections;
    int m_nextNodeId;
};

}
}
