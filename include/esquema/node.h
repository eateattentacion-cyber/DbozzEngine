/////////////////////////////////////////////////////////////////////////////
// node.h                                                                  //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QString>
#include <QList>
#include <QVariant>
#include <QPointF>
#include <memory>

namespace DabozzEngine {
namespace Esquema {

enum class PinType {
    Exec,
    Int,
    Float,
    Bool,
    String,
    Vector3,
    Entity
};

struct Pin {
    QString name;
    PinType type;
    QVariant defaultValue;
    bool isInput;
    int nodeId;
    int pinIndex;
};

enum class NodeType {
    Event,
    Function,
    Variable,
    Operator,
    Flow,
    Constant,
    Math,
    Logic,
    Engine
};

class Node {
public:
    Node(int id, NodeType type, const QString& name);
    virtual ~Node() = default;

    int getId() const { return m_id; }
    NodeType getType() const { return m_type; }
    QString getName() const { return m_name; }
    
    void addInputPin(const QString& name, PinType type, const QVariant& defaultValue = QVariant());
    void addOutputPin(const QString& name, PinType type);
    
    const QList<Pin>& getInputPins() const { return m_inputPins; }
    const QList<Pin>& getOutputPins() const { return m_outputPins; }
    
    void setPosition(const QPointF& pos) { m_position = pos; }
    QPointF getPosition() const { return m_position; }
    
    virtual QString generateLuaCode() const = 0;
    virtual QString generateAngelScriptCode() const = 0;
    
    virtual QString generateCode() const { return generateLuaCode(); }

protected:
    int m_id;
    NodeType m_type;
    QString m_name;
    QList<Pin> m_inputPins;
    QList<Pin> m_outputPins;
    QPointF m_position;
};

class EventNode : public Node {
public:
    EventNode(int id, const QString& eventName);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class FunctionNode : public Node {
public:
    FunctionNode(int id, const QString& functionName);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class VariableNode : public Node {
public:
    VariableNode(int id, const QString& varName, PinType varType);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
    
private:
    PinType m_varType;
};

class ConstantNode : public Node {
public:
    ConstantNode(int id, PinType type, const QVariant& value);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
    
    void setValue(const QVariant& value) { m_value = value; }
    QVariant getValue() const { return m_value; }
    
private:
    QVariant m_value;
    PinType m_valueType;
};

class OperatorNode : public Node {
public:
    enum class OpType { Add, Subtract, Multiply, Divide, Equal, NotEqual, Greater, Less };
    
    OperatorNode(int id, OpType op);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
    
private:
    OpType m_opType;
};

}
}
