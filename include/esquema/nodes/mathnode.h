/////////////////////////////////////////////////////////////////////////////
// mathnode.h                                                              //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
//                                                                         //
// Permission is hereby granted, free of charge, to any person obtaining   //
// a copy of this software and associated documentation files (the         //
// "Software"), to deal in the Software without restriction, including     //
// without limitation the rights to use, copy, modify, merge, publish,     //
// distribute, sublicense, and/or sell copies of the Software, and to      //
// permit persons to whom the Software is furnished to do so, subject to   //
// the following conditions:                                               //
//                                                                         //
// The above copyright notice and this permission notice shall be included //
// in all copies or substantial portions of the Software.                  //
//                                                                         //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  //
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    //
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    //
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       //
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  //
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "esquema/node.h"

namespace DabozzEngine {
namespace Esquema {

class AddNode : public Node {
public:
    AddNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class SubtractNode : public Node {
public:
    SubtractNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class MultiplyNode : public Node {
public:
    MultiplyNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class DivideNode : public Node {
public:
    DivideNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class SinNode : public Node {
public:
    SinNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class CosNode : public Node {
public:
    CosNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class SqrtNode : public Node {
public:
    SqrtNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class AbsNode : public Node {
public:
    AbsNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class ClampNode : public Node {
public:
    ClampNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

class LerpNode : public Node {
public:
    LerpNode(int id);
    QString generateLuaCode() const override;
    QString generateAngelScriptCode() const override;
};

}
}