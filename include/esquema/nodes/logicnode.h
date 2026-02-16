/////////////////////////////////////////////////////////////////////////////
// logicnode.h                                                             //
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

class BranchNode : public Node {
public:
    BranchNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class AndNode : public Node {
public:
    AndNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class OrNode : public Node {
public:
    OrNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class NotNode : public Node {
public:
    NotNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class CompareNode : public Node {
public:
    enum class CompareType { Equal, NotEqual, Greater, GreaterEqual, Less, LessEqual };
    
    CompareNode(int id, CompareType type);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
    
private:
    CompareType m_compareType;
};

class ForLoopNode : public Node {
public:
    ForLoopNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class WhileLoopNode : public Node {
public:
    WhileLoopNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

}
}