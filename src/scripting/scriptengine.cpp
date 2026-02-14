/////////////////////////////////////////////////////////////////////////////
// scriptengine.cpp                                                        //
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

#include "scripting/scriptengine.h"
#include "scripting/scriptapi.h"
#include "debug/logger.h"
#include <fstream>
#include <sstream>

namespace DabozzEngine {
namespace Scripting {

static void MessageCallback(const asSMessageInfo* msg, void*)
{
    const char* type = "INFO";
    if (msg->type == asMSGTYPE_WARNING) type = "WARN";
    else if (msg->type == asMSGTYPE_ERROR) type = "ERROR";
    DEBUG_LOG << "[AS " << type << "] " << msg->section << " (" << msg->row << ", " << msg->col << "): " << msg->message << std::endl;
}

ScriptEngine::ScriptEngine()
    : m_luaState(nullptr)
    , m_asEngine(nullptr)
    , m_asContext(nullptr)
{
}

ScriptEngine::~ScriptEngine()
{
    shutdown();
}

bool ScriptEngine::initialize()
{
    return initialize(nullptr);
}

bool ScriptEngine::initialize(ECS::World* world)
{
    DEBUG_LOG << "Initializing Lua..." << std::endl;
    m_luaState = luaL_newstate();
    if (!m_luaState) {
        DEBUG_LOG << "ERROR: Failed to create Lua state" << std::endl;
        return false;
    }
    luaL_openlibs(m_luaState);

    if (world) {
        ScriptAPI::RegisterLuaAPI(m_luaState, world);
    }

    DEBUG_LOG << "Initializing AngelScript..." << std::endl;
    m_asEngine = asCreateScriptEngine();
    if (!m_asEngine) {
        DEBUG_LOG << "ERROR: Failed to create AngelScript engine" << std::endl;
        return false;
    }

    m_asEngine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);

    m_asContext = m_asEngine->CreateContext();
    if (!m_asContext) {
        DEBUG_LOG << "ERROR: Failed to create AngelScript context" << std::endl;
        return false;
    }

    if (world) {
        ScriptAPI::RegisterAngelScriptAPI(m_asEngine, world);
    }

    DEBUG_LOG << "Script engines initialized" << std::endl;
    return true;
}

void ScriptEngine::shutdown()
{
    if (m_asContext) {
        m_asContext->Release();
        m_asContext = nullptr;
    }

    if (m_asEngine) {
        m_asEngine->ShutDownAndRelease();
        m_asEngine = nullptr;
    }

    if (m_luaState) {
        lua_close(m_luaState);
        m_luaState = nullptr;
    }
}

bool ScriptEngine::loadLuaScript(const std::string& filepath)
{
    if (!m_luaState) return false;

    if (luaL_dofile(m_luaState, filepath.c_str()) != LUA_OK) {
        DEBUG_LOG << "Lua error: " << lua_tostring(m_luaState, -1) << std::endl;
        lua_pop(m_luaState, 1);
        return false;
    }

    return true;
}

bool ScriptEngine::executeLuaString(const std::string& code)
{
    if (!m_luaState) return false;

    if (luaL_dostring(m_luaState, code.c_str()) != LUA_OK) {
        DEBUG_LOG << "Lua error: " << lua_tostring(m_luaState, -1) << std::endl;
        lua_pop(m_luaState, 1);
        return false;
    }

    return true;
}

void ScriptEngine::callLuaStart()
{
    if (!m_luaState) return;
    
    lua_getglobal(m_luaState, "Start");
    if (lua_isfunction(m_luaState, -1)) {
        if (lua_pcall(m_luaState, 0, 0, 0) != LUA_OK) {
            DEBUG_LOG << "Lua Start() error: " << lua_tostring(m_luaState, -1) << std::endl;
            lua_pop(m_luaState, 1);
        }
    } else {
        lua_pop(m_luaState, 1);
    }
}

void ScriptEngine::callLuaUpdate(float deltaTime)
{
    if (!m_luaState) return;
    
    lua_getglobal(m_luaState, "Update");
    if (lua_isfunction(m_luaState, -1)) {
        lua_pushnumber(m_luaState, deltaTime);
        if (lua_pcall(m_luaState, 1, 0, 0) != LUA_OK) {
            DEBUG_LOG << "Lua Update() error: " << lua_tostring(m_luaState, -1) << std::endl;
            lua_pop(m_luaState, 1);
        }
    } else {
        lua_pop(m_luaState, 1);
    }
}

bool ScriptEngine::loadAngelScript(const std::string& filepath)
{
    if (!m_asEngine) return false;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        DEBUG_LOG << "ERROR: Could not open AngelScript file: " << filepath << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    asIScriptModule* mod = m_asEngine->GetModule("main", asGM_ALWAYS_CREATE);
    if (mod->AddScriptSection(filepath.c_str(), code.c_str(), code.length()) < 0) {
        DEBUG_LOG << "ERROR: Failed to add AngelScript section" << std::endl;
        return false;
    }

    if (mod->Build() < 0) {
        DEBUG_LOG << "ERROR: Failed to build AngelScript module" << std::endl;
        return false;
    }

    return true;
}

bool ScriptEngine::executeAngelScriptString(const std::string& code)
{
    if (!m_asEngine || !m_asContext) return false;

    asIScriptModule* mod = m_asEngine->GetModule("main", asGM_ALWAYS_CREATE);
    if (mod->AddScriptSection("exec", code.c_str(), code.length()) < 0) {
        return false;
    }

    if (mod->Build() < 0) {
        return false;
    }

    asIScriptFunction* func = mod->GetFunctionByDecl("void main()");
    if (!func) {
        DEBUG_LOG << "ERROR: No main() function found" << std::endl;
        return false;
    }

    m_asContext->Prepare(func);
    int r = m_asContext->Execute();
    if (r != asEXECUTION_FINISHED) {
        DEBUG_LOG << "ERROR: AngelScript execution failed" << std::endl;
        return false;
    }

    return true;
}

void ScriptEngine::callAngelScriptStart()
{
    if (!m_asEngine || !m_asContext) return;
    
    asIScriptModule* mod = m_asEngine->GetModule("main");
    if (!mod) return;
    
    asIScriptFunction* func = mod->GetFunctionByDecl("void Start()");
    if (!func) return;
    
    m_asContext->Prepare(func);
    int r = m_asContext->Execute();
    if (r != asEXECUTION_FINISHED) {
        DEBUG_LOG << "ERROR: AngelScript Start() execution failed" << std::endl;
    }
}

void ScriptEngine::callAngelScriptUpdate(float deltaTime)
{
    if (!m_asEngine || !m_asContext) return;
    
    asIScriptModule* mod = m_asEngine->GetModule("main");
    if (!mod) return;
    
    asIScriptFunction* func = mod->GetFunctionByDecl("void Update(float)");
    if (!func) return;
    
    m_asContext->Prepare(func);
    m_asContext->SetArgFloat(0, deltaTime);
    int r = m_asContext->Execute();
    if (r != asEXECUTION_FINISHED) {
        DEBUG_LOG << "ERROR: AngelScript Update() execution failed" << std::endl;
    }
}

}
}
