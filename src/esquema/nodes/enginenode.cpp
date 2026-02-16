/////////////////////////////////////////////////////////////////////////////
// enginenode.cpp                                                          //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#include "esquema/nodes/enginenode.h"

namespace DabozzEngine {
namespace Esquema {

CreateEntityNode::CreateEntityNode(int id) : Node(id, NodeType::Engine, "Create Entity") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Name", PinType::String, "Entity");
    addOutputPin("Exec", PinType::Exec);
    addOutputPin("Entity", PinType::Entity);
}

QString CreateEntityNode::generateLuaCode() const {
    return "local entity = World:createEntity(Name)";
}

QString CreateEntityNode::generateAngelScriptCode() const {
    return "Entity@ entity = World.createEntity(Name);";
}

DestroyEntityNode::DestroyEntityNode(int id) : Node(id, NodeType::Engine, "Destroy Entity") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Entity", PinType::Entity);
    addOutputPin("Exec", PinType::Exec);
}

QString DestroyEntityNode::generateLuaCode() const {
    return "World:destroyEntity(Entity)";
}

QString DestroyEntityNode::generateAngelScriptCode() const {
    return "World.destroyEntity(Entity);";
}

GetEntityNode::GetEntityNode(int id) : Node(id, NodeType::Engine, "Get Entity") {
    addInputPin("Name", PinType::String, "");
    addOutputPin("Entity", PinType::Entity);
}

QString GetEntityNode::generateLuaCode() const {
    return "World:getEntityByName(Name)";
}

QString GetEntityNode::generateAngelScriptCode() const {
    return "World.getEntityByName(Name)";
}

GetPositionNode::GetPositionNode(int id) : Node(id, NodeType::Engine, "Get Position") {
    addInputPin("Entity", PinType::Entity);
    addOutputPin("Position", PinType::Vector3);
}

QString GetPositionNode::generateLuaCode() const {
    return "Entity:getComponent('Transform').position";
}

QString GetPositionNode::generateAngelScriptCode() const {
    return "Entity.getComponent<Transform>().position";
}

SetPositionNode::SetPositionNode(int id) : Node(id, NodeType::Engine, "Set Position") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Entity", PinType::Entity);
    addInputPin("Position", PinType::Vector3);
    addOutputPin("Exec", PinType::Exec);
}

QString SetPositionNode::generateLuaCode() const {
    return "Entity:getComponent('Transform').position = Position";
}

QString SetPositionNode::generateAngelScriptCode() const {
    return "Entity.getComponent<Transform>().position = Position;";
}

GetRotationNode::GetRotationNode(int id) : Node(id, NodeType::Engine, "Get Rotation") {
    addInputPin("Entity", PinType::Entity);
    addOutputPin("Rotation", PinType::Vector3);
}

QString GetRotationNode::generateLuaCode() const {
    return "Entity:getComponent('Transform').rotation";
}

QString GetRotationNode::generateAngelScriptCode() const {
    return "Entity.getComponent<Transform>().rotation";
}

SetRotationNode::SetRotationNode(int id) : Node(id, NodeType::Engine, "Set Rotation") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Entity", PinType::Entity);
    addInputPin("Rotation", PinType::Vector3);
    addOutputPin("Exec", PinType::Exec);
}

QString SetRotationNode::generateLuaCode() const {
    return "Entity:getComponent('Transform').rotation = Rotation";
}

QString SetRotationNode::generateAngelScriptCode() const {
    return "Entity.getComponent<Transform>().rotation = Rotation;";
}

GetKeyPressedNode::GetKeyPressedNode(int id) : Node(id, NodeType::Engine, "Get Key Pressed") {
    addInputPin("Key", PinType::String, "Space");
    addOutputPin("Pressed", PinType::Bool);
}

QString GetKeyPressedNode::generateLuaCode() const {
    return "Input:isKeyPressed(Key)";
}

QString GetKeyPressedNode::generateAngelScriptCode() const {
    return "Input.isKeyPressed(Key)";
}

GetMousePositionNode::GetMousePositionNode(int id) : Node(id, NodeType::Engine, "Get Mouse Position") {
    addOutputPin("Position", PinType::Vector3);
}

QString GetMousePositionNode::generateLuaCode() const {
    return "Input:getMousePosition()";
}

QString GetMousePositionNode::generateAngelScriptCode() const {
    return "Input.getMousePosition()";
}

PrintNode::PrintNode(int id) : Node(id, NodeType::Engine, "Print") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Text", PinType::String, "Hello World");
    addOutputPin("Exec", PinType::Exec);
}

QString PrintNode::generateLuaCode() const {
    return "print(Text)";
}

QString PrintNode::generateAngelScriptCode() const {
    return "print(Text);";
}

LogNode::LogNode(int id) : Node(id, NodeType::Engine, "Log") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Message", PinType::String, "Log message");
    addInputPin("Level", PinType::String, "Info");
    addOutputPin("Exec", PinType::Exec);
}

QString LogNode::generateLuaCode() const {
    return "Log:write(Level, Message)";
}

QString LogNode::generateAngelScriptCode() const {
    return "Log.write(Level, Message);";
}

GetDeltaTimeNode::GetDeltaTimeNode(int id) : Node(id, NodeType::Engine, "Get Delta Time") {
    addOutputPin("Delta Time", PinType::Float);
}

QString GetDeltaTimeNode::generateLuaCode() const {
    return "Time:getDeltaTime()";
}

QString GetDeltaTimeNode::generateAngelScriptCode() const {
    return "Time.getDeltaTime()";
}

DelayNode::DelayNode(int id) : Node(id, NodeType::Engine, "Delay") {
    addInputPin("Exec", PinType::Exec);
    addInputPin("Duration", PinType::Float, 1.0f);
    addOutputPin("Exec", PinType::Exec);
}

QString DelayNode::generateLuaCode() const {
    return "Timer:delay(Duration, function() end)";
}

QString DelayNode::generateAngelScriptCode() const {
    return "Timer.delay(Duration, @() {});";
}

}
}