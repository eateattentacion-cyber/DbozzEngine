/////////////////////////////////////////////////////////////////////////////
// logicnode.cpp                                                           //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#include "esquema/nodes/logicnode.h"

namespace DabozzEngine {
namespace Esquema {

BranchNode::BranchNode(int id) : Node(id, NodeType::Flow, "Branch") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Condition", PinType::Bool, false);
    addOutputPin("True", PinType::Exec);
    addOutputPin("False", PinType::Exec);
}

QString BranchNode::generateLuaCode() const {
    return "if Condition then\n    -- True branch\nelse\n    -- False branch\nend";
}

QString BranchNode::generateAngelScriptCode() const {
    return "if (Condition) {\n    // True branch\n} else {\n    // False branch\n}";
}

AndNode::AndNode(int id) : Node(id, NodeType::Logic, "And") {
    addInputPin("A", PinType::Bool, false);
    addInputPin("B", PinType::Bool, false);
    addOutputPin("Result", PinType::Bool);
}

QString AndNode::generateLuaCode() const {
    return "A and B";
}

QString AndNode::generateAngelScriptCode() const {
    return "A && B";
}

OrNode::OrNode(int id) : Node(id, NodeType::Logic, "Or") {
    addInputPin("A", PinType::Bool, false);
    addInputPin("B", PinType::Bool, false);
    addOutputPin("Result", PinType::Bool);
}

QString OrNode::generateLuaCode() const {
    return "A or B";
}

QString OrNode::generateAngelScriptCode() const {
    return "A || B";
}

NotNode::NotNode(int id) : Node(id, NodeType::Logic, "Not") {
    addInputPin("Value", PinType::Bool, false);
    addOutputPin("Result", PinType::Bool);
}

QString NotNode::generateLuaCode() const {
    return "not Value";
}

QString NotNode::generateAngelScriptCode() const {
    return "!Value";
}

CompareNode::CompareNode(int id, CompareType type) 
    : Node(id, NodeType::Logic, "Compare"), m_compareType(type) {
    addInputPin("A", PinType::Float, 0.0f);
    addInputPin("B", PinType::Float, 0.0f);
    addOutputPin("Result", PinType::Bool);
}

QString CompareNode::generateLuaCode() const {
    QString op;
    switch (m_compareType) {
        case CompareType::Equal: op = "=="; break;
        case CompareType::NotEqual: op = "~="; break;
        case CompareType::Greater: op = ">"; break;
        case CompareType::GreaterEqual: op = ">="; break;
        case CompareType::Less: op = "<"; break;
        case CompareType::LessEqual: op = "<="; break;
    }
    return QString("A %1 B").arg(op);
}

QString CompareNode::generateAngelScriptCode() const {
    QString op;
    switch (m_compareType) {
        case CompareType::Equal: op = "=="; break;
        case CompareType::NotEqual: op = "!="; break;
        case CompareType::Greater: op = ">"; break;
        case CompareType::GreaterEqual: op = ">="; break;
        case CompareType::Less: op = "<"; break;
        case CompareType::LessEqual: op = "<="; break;
    }
    return QString("A %1 B").arg(op);
}

ForLoopNode::ForLoopNode(int id) : Node(id, NodeType::Flow, "For Loop") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Start", PinType::Int, 0);
    addInputPin("End", PinType::Int, 10);
    addInputPin("Step", PinType::Int, 1);
    addOutputPin("Loop Body", PinType::Exec);
    addOutputPin("Index", PinType::Int);
    addOutputPin("Completed", PinType::Exec);
}

QString ForLoopNode::generateLuaCode() const {
    return "for i = Start, End, Step do\n    -- Loop body\nend";
}

QString ForLoopNode::generateAngelScriptCode() const {
    return "for (int i = Start; i <= End; i += Step) {\n    // Loop body\n}";
}

WhileLoopNode::WhileLoopNode(int id) : Node(id, NodeType::Flow, "While Loop") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Condition", PinType::Bool, true);
    addOutputPin("Loop Body", PinType::Exec);
    addOutputPin("Completed", PinType::Exec);
}

QString WhileLoopNode::generateLuaCode() const {
    return "while Condition do\n    -- Loop body\nend";
}

QString WhileLoopNode::generateAngelScriptCode() const {
    return "while (Condition) {\n    // Loop body\n}";
}

}
}