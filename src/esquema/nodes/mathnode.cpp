/////////////////////////////////////////////////////////////////////////////
// mathnode.cpp                                                            //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#include "esquema/nodes/mathnode.h"

namespace DabozzEngine {
namespace Esquema {

AddNode::AddNode(int id) : Node(id, NodeType::Math, "Add") {
    addInputPin("A", PinType::Float, 0.0f);
    addInputPin("B", PinType::Float, 0.0f);
    addOutputPin("Result", PinType::Float);
}

QString AddNode::generateLuaCode() const {
    return "A + B";
}

QString AddNode::generateAngelScriptCode() const {
    return "A + B";
}

SubtractNode::SubtractNode(int id) : Node(id, NodeType::Math, "Subtract") {
    addInputPin("A", PinType::Float, 0.0f);
    addInputPin("B", PinType::Float, 0.0f);
    addOutputPin("Result", PinType::Float);
}

QString SubtractNode::generateLuaCode() const {
    return "A - B";
}

QString SubtractNode::generateAngelScriptCode() const {
    return "A - B";
}

MultiplyNode::MultiplyNode(int id) : Node(id, NodeType::Math, "Multiply") {
    addInputPin("A", PinType::Float, 1.0f);
    addInputPin("B", PinType::Float, 1.0f);
    addOutputPin("Result", PinType::Float);
}

QString MultiplyNode::generateLuaCode() const {
    return "A * B";
}

QString MultiplyNode::generateAngelScriptCode() const {
    return "A * B";
}

DivideNode::DivideNode(int id) : Node(id, NodeType::Math, "Divide") {
    addInputPin("A", PinType::Float, 1.0f);
    addInputPin("B", PinType::Float, 1.0f);
    addOutputPin("Result", PinType::Float);
}

QString DivideNode::generateLuaCode() const {
    return "A / B";
}

QString DivideNode::generateAngelScriptCode() const {
    return "A / B";
}

SinNode::SinNode(int id) : Node(id, NodeType::Math, "Sin") {
    addInputPin("Value", PinType::Float, 0.0f);
    addOutputPin("Result", PinType::Float);
}

QString SinNode::generateLuaCode() const {
    return "math.sin(Value)";
}

QString SinNode::generateAngelScriptCode() const {
    return "sin(Value)";
}

CosNode::CosNode(int id) : Node(id, NodeType::Math, "Cos") {
    addInputPin("Value", PinType::Float, 0.0f);
    addOutputPin("Result", PinType::Float);
}

QString CosNode::generateLuaCode() const {
    return "math.cos(Value)";
}

QString CosNode::generateAngelScriptCode() const {
    return "cos(Value)";
}

SqrtNode::SqrtNode(int id) : Node(id, NodeType::Math, "Sqrt") {
    addInputPin("Value", PinType::Float, 0.0f);
    addOutputPin("Result", PinType::Float);
}

QString SqrtNode::generateLuaCode() const {
    return "math.sqrt(Value)";
}

QString SqrtNode::generateAngelScriptCode() const {
    return "sqrt(Value)";
}

AbsNode::AbsNode(int id) : Node(id, NodeType::Math, "Abs") {
    addInputPin("Value", PinType::Float, 0.0f);
    addOutputPin("Result", PinType::Float);
}

QString AbsNode::generateLuaCode() const {
    return "math.abs(Value)";
}

QString AbsNode::generateAngelScriptCode() const {
    return "abs(Value)";
}

ClampNode::ClampNode(int id) : Node(id, NodeType::Math, "Clamp") {
    addInputPin("Value", PinType::Float, 0.0f);
    addInputPin("Min", PinType::Float, 0.0f);
    addInputPin("Max", PinType::Float, 1.0f);
    addOutputPin("Result", PinType::Float);
}

QString ClampNode::generateLuaCode() const {
    return "math.max(Min, math.min(Max, Value))";
}

QString ClampNode::generateAngelScriptCode() const {
    return "clamp(Value, Min, Max)";
}

LerpNode::LerpNode(int id) : Node(id, NodeType::Math, "Lerp") {
    addInputPin("A", PinType::Float, 0.0f);
    addInputPin("B", PinType::Float, 1.0f);
    addInputPin("T", PinType::Float, 0.5f);
    addOutputPin("Result", PinType::Float);
}

QString LerpNode::generateLuaCode() const {
    return "A + (B - A) * T";
}

QString LerpNode::generateAngelScriptCode() const {
    return "lerp(A, B, T)";
}

}
}