/////////////////////////////////////////////////////////////////////////////
// enginenode.h                                                            //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "esquema/node.h"

namespace DabozzEngine {
namespace Esquema {

// Entity management nodes
class CreateEntityNode : public Node {
public:
    CreateEntityNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class DestroyEntityNode : public Node {
public:
    DestroyEntityNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class GetEntityNode : public Node {
public:
    GetEntityNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

// Transform nodes
class GetPositionNode : public Node {
public:
    GetPositionNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class SetPositionNode : public Node {
public:
    SetPositionNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class GetRotationNode : public Node {
public:
    GetRotationNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class SetRotationNode : public Node {
public:
    SetRotationNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

// Input nodes
class GetKeyPressedNode : public Node {
public:
    GetKeyPressedNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class GetMousePositionNode : public Node {
public:
    GetMousePositionNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

// Debug nodes
class PrintNode : public Node {
public:
    PrintNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class LogNode : public Node {
public:
    LogNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

// Time nodes
class GetDeltaTimeNode : public Node {
public:
    GetDeltaTimeNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class DelayNode : public Node {
public:
    DelayNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

}
}