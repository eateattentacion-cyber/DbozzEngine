/////////////////////////////////////////////////////////////////////////////
// scriptengine.h                                                          //
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
//                                                                         //
// Scripting powered by:                                                   //
//   - Lua (https://www.lua.org) - MIT License                             //
//   - AngelScript (https://www.angelcode.com/angelscript) - zlib License  //
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <memory>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "angelscript.h"

namespace DabozzEngine {

namespace ECS {
    class World;
}

namespace Scripting {

enum class ScriptLanguage {
    Lua,
    AngelScript
};

class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    bool initialize();
    bool initialize(ECS::World* world);
    void shutdown();

    bool loadLuaScript(const std::string& filepath);
    bool executeLuaString(const std::string& code);
    void callLuaStart();
    void callLuaUpdate(float deltaTime);
    lua_State* getLuaState() { return m_luaState; }

    bool loadAngelScript(const std::string& filepath);
    bool executeAngelScriptString(const std::string& code);
    void callAngelScriptStart();
    void callAngelScriptUpdate(float deltaTime);
    asIScriptEngine* getAngelScriptEngine() { return m_asEngine; }

private:
    lua_State* m_luaState;
    asIScriptEngine* m_asEngine;
    asIScriptContext* m_asContext;
};

}
}
