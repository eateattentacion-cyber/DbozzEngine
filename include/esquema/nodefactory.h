/////////////////////////////////////////////////////////////////////////////
// nodefactory.h                                                           //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "esquema/node.h"
#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <memory>

namespace DabozzEngine {
namespace Esquema {

class NodeFactory {
public:
    // Node creation
    static std::shared_ptr<Node> createNode(int id, const QString& nodeType);
    static std::shared_ptr<Node> createEventNode(int id, const QString& eventName);
    static std::shared_ptr<Node> createConstantNode(int id, PinType type, const QVariant& value);
    
    // Node type queries
    static QStringList getAvailableNodeTypes();
    static QStringList getEventNodeTypes();
    static QStringList getMathNodeTypes();
    static QStringList getLogicNodeTypes();
    static QStringList getEngineNodeTypes();
    
    // Node categories
    static QString getNodeCategory(const QString& nodeType);
    static QStringList getNodeTypesInCategory(const QString& category);
    
    // Node information
    static QString getNodeDescription(const QString& nodeType);
    static NodeType getNodeTypeEnum(const QString& nodeType);
    
    // Serialization support
    static std::shared_ptr<Node> createNodeFromJson(const QJsonObject& json);

private:
    NodeFactory() = default;
    
    static void initializeNodeTypes();
    static bool s_initialized;
    static QMap<QString, QString> s_nodeCategories;
    static QMap<QString, QString> s_nodeDescriptions;
};

}
}