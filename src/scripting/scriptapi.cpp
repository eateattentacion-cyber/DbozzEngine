/////////////////////////////////////////////////////////////////////////////
// scriptapi.cpp                                                           //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
//                                                                         //
// Scripting powered by:                                                   //
//   - Lua (https://www.lua.org) - MIT License                             //
//   - AngelScript (https://www.angelcode.com/angelscript) - zlib License  //
/////////////////////////////////////////////////////////////////////////////

#include "scripting/scriptapi.h"
#include "input/inputmanager.h"
#include "ecs/world.h"
#include "ecs/components/transform.h"
#include "ecs/components/rigidbody.h"
#include "ecs/components/name.h"
#include "ecs/components/mesh.h"
#include "ecs/components/boxcollider.h"
#include "ecs/components/spherecollider.h"
#include "ecs/components/audiosource.h"
#include "physics/simplephysics.h"
#include "debug/logger.h"
#include <iostream>

namespace DabozzEngine {
namespace Scripting {

ECS::World* ScriptAPI::s_world = nullptr;
float ScriptAPI::s_deltaTime = 0.0f;
std::function<void(const std::string&)> ScriptAPI::s_logCallback = nullptr;

void ScriptAPI::RegisterLuaAPI(lua_State* L, ECS::World* world)
{
    s_world = world;

    lua_register(L, "print", Lua_Print);
    lua_register(L, "CreateEntity", Lua_CreateEntity);
    lua_register(L, "DestroyEntity", Lua_DestroyEntity);
    lua_register(L, "GetEntityPosition", Lua_GetEntityPosition);
    lua_register(L, "SetEntityPosition", Lua_SetEntityPosition);
    lua_register(L, "GetEntityRotation", Lua_GetEntityRotation);
    lua_register(L, "SetEntityRotation", Lua_SetEntityRotation);
    lua_register(L, "GetEntityScale", Lua_GetEntityScale);
    lua_register(L, "SetEntityScale", Lua_SetEntityScale);
    lua_register(L, "GetDeltaTime", Lua_GetDeltaTime);
    lua_register(L, "AddRigidbody", Lua_AddRigidbody);
    lua_register(L, "SetVelocity", Lua_SetVelocity);
    lua_register(L, "GetVelocity", Lua_GetVelocity);
    lua_register(L, "ApplyForce", Lua_ApplyForce);
    lua_register(L, "AddBoxCollider", Lua_AddBoxCollider);
    lua_register(L, "AddSphereCollider", Lua_AddSphereCollider);
    lua_register(L, "LoadMesh", Lua_LoadMesh);
    lua_register(L, "CreateCube", Lua_CreateCube);
    lua_register(L, "PlaySound", Lua_PlaySound);
    lua_register(L, "FindEntityByName", Lua_FindEntityByName);
    lua_register(L, "SetEntityName", Lua_SetEntityName);
    lua_register(L, "GetEntityName", Lua_GetEntityName);
    lua_register(L, "RaycastFromCamera", Lua_RaycastFromCamera);
    lua_register(L, "GetMousePosition", Lua_GetMousePosition);
    
    // New physics API
    lua_register(L, "Raycast", Lua_Raycast);
    lua_register(L, "AddSphereRigidbody", Lua_AddSphereRigidbody);
    
    // New audio API
    lua_register(L, "PlayAudio", Lua_PlayAudio);
    lua_register(L, "StopAudio", Lua_StopAudio);
    lua_register(L, "PauseAudio", Lua_PauseAudio);
    lua_register(L, "SetAudioVolume", Lua_SetAudioVolume);
    lua_register(L, "SetAudioSpatial", Lua_SetAudioSpatial);

    lua_register(L, "IsKeyPressed", Lua_IsKeyPressed);
    lua_register(L, "IsKeyDown", Lua_IsKeyDown);
    lua_register(L, "IsMouseButtonPressed", Lua_IsMouseButtonPressed);
    lua_register(L, "GetAllEntities", Lua_GetAllEntities);
    lua_register(L, "HasComponent", Lua_HasComponent);
    lua_register(L, "SetGravity", Lua_SetGravity);
    lua_register(L, "LoadScene", Lua_LoadScene);
    lua_register(L, "SaveScene", Lua_SaveScene);
    lua_register(L, "InstantiatePrefab", Lua_InstantiatePrefab);
    lua_register(L, "LookAt", Lua_LookAt);
    lua_register(L, "Distance", Lua_Distance);
    lua_register(L, "Lerp", Lua_Lerp);

    DEBUG_LOG << "Lua API registered" << std::endl;
}

void AS_Print(const std::string& msg)
{
    DEBUG_LOG << "[AngelScript] " << msg << std::endl;
    
    if (ScriptAPI::s_logCallback) {
        ScriptAPI::s_logCallback(msg);
    }
}

void ScriptAPI::RegisterAngelScriptAPI(asIScriptEngine* engine, ECS::World* world)
{
    s_world = world;

    int r = 0;

    r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(AS_Print), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS print" << std::endl;

    r = engine->RegisterGlobalFunction("uint CreateEntity()", asFUNCTION(AS_CreateEntity), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS CreateEntity" << std::endl;

    r = engine->RegisterGlobalFunction("void DestroyEntity(uint)", asFUNCTION(AS_DestroyEntity), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS DestroyEntity" << std::endl;

    r = engine->RegisterGlobalFunction("void SetEntityPosition(uint, float, float, float)", 
        asFUNCTION(AS_SetEntityPosition), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS SetEntityPosition" << std::endl;

    r = engine->RegisterGlobalFunction("void SetEntityRotation(uint, float, float, float)", 
        asFUNCTION(AS_SetEntityRotation), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS SetEntityRotation" << std::endl;

    r = engine->RegisterGlobalFunction("void SetEntityScale(uint, float, float, float)", 
        asFUNCTION(AS_SetEntityScale), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS SetEntityScale" << std::endl;

    r = engine->RegisterGlobalFunction("void AddRigidbody(uint, float, bool)", 
        asFUNCTION(AS_AddRigidbody), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS AddRigidbody" << std::endl;

    r = engine->RegisterGlobalFunction("void SetVelocity(uint, float, float, float)", 
        asFUNCTION(AS_SetVelocity), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS SetVelocity" << std::endl;

    r = engine->RegisterGlobalFunction("void ApplyForce(uint, float, float, float)", 
        asFUNCTION(AS_ApplyForce), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS ApplyForce" << std::endl;

    r = engine->RegisterGlobalFunction("void AddBoxCollider(uint, float, float, float)", 
        asFUNCTION(AS_AddBoxCollider), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS AddBoxCollider" << std::endl;

    r = engine->RegisterGlobalFunction("void LoadMesh(uint, const string &in)", 
        asFUNCTION(AS_LoadMesh), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS LoadMesh" << std::endl;

    r = engine->RegisterGlobalFunction("void CreateCube(uint, float)", 
        asFUNCTION(AS_CreateCube), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS CreateCube" << std::endl;

    r = engine->RegisterGlobalFunction("float GetDeltaTime()", 
        asFUNCTION(AS_GetDeltaTime), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS GetDeltaTime" << std::endl;

    r = engine->RegisterGlobalFunction("uint FindEntityByName(const string &in)", 
        asFUNCTION(AS_FindEntityByName), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS FindEntityByName" << std::endl;

    r = engine->RegisterGlobalFunction("void SetEntityName(uint, const string &in)", 
        asFUNCTION(AS_SetEntityName), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS SetEntityName" << std::endl;

    r = engine->RegisterGlobalFunction("bool IsKeyPressed(int)", 
        asFUNCTION(AS_IsKeyPressed), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS IsKeyPressed" << std::endl;

    r = engine->RegisterGlobalFunction("bool HasComponent(uint, const string &in)", 
        asFUNCTION(AS_HasComponent), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS HasComponent" << std::endl;

    r = engine->RegisterGlobalFunction("void SetGravity(float, float, float)", 
        asFUNCTION(AS_SetGravity), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS SetGravity" << std::endl;

    r = engine->RegisterGlobalFunction("float Distance(float, float, float, float, float, float)", 
        asFUNCTION(AS_Distance), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS Distance" << std::endl;

    r = engine->RegisterGlobalFunction("float Lerp(float, float, float)", 
        asFUNCTION(AS_Lerp), asCALL_CDECL);
    if (r < 0) DEBUG_LOG << "Failed to register AS Lerp" << std::endl;

    DEBUG_LOG << "AngelScript API registered" << std::endl;
}

int ScriptAPI::Lua_Print(lua_State* L)
{
    int n = lua_gettop(L);
    std::string output;
    for (int i = 1; i <= n; i++) {
        if (lua_isstring(L, i)) {
            output += lua_tostring(L, i);
        } else if (lua_isnumber(L, i)) {
            output += std::to_string(lua_tonumber(L, i));
        } else if (lua_isboolean(L, i)) {
            output += lua_toboolean(L, i) ? "true" : "false";
        }
        if (i < n) output += "\t";
    }
    DEBUG_LOG << "[Lua] " << output << std::endl;
    
    if (s_logCallback) {
        s_logCallback(output);
    }
    
    return 0;
}

int ScriptAPI::Lua_CreateEntity(lua_State* L)
{
    if (!s_world) {
        lua_pushnil(L);
        return 1;
    }

    ECS::EntityID entity = s_world->createEntity();
    s_world->addComponent<ECS::Transform>(entity);
    
    lua_pushinteger(L, entity);
    return 1;
}

int ScriptAPI::Lua_DestroyEntity(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    s_world->destroyEntity(entity);
    return 0;
}

int ScriptAPI::Lua_GetEntityPosition(lua_State* L)
{
    if (!s_world) {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        return 3;
    }

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);

    if (transform) {
        lua_pushnumber(L, transform->position.x());
        lua_pushnumber(L, transform->position.y());
        lua_pushnumber(L, transform->position.z());
    } else {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
    }
    return 3;
}

int ScriptAPI::Lua_SetEntityPosition(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));

    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);
    if (transform) {
        transform->position = QVector3D(x, y, z);
    }
    return 0;
}

int ScriptAPI::Lua_GetEntityRotation(lua_State* L)
{
    if (!s_world) {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        return 3;
    }

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);

    if (transform) {
        QVector3D euler = transform->rotation.toEulerAngles();
        lua_pushnumber(L, euler.x());
        lua_pushnumber(L, euler.y());
        lua_pushnumber(L, euler.z());
    } else {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
    }
    return 3;
}

int ScriptAPI::Lua_SetEntityRotation(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));

    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);
    if (transform) {
        transform->rotation = QQuaternion::fromEulerAngles(x, y, z);
    }
    return 0;
}

int ScriptAPI::Lua_GetDeltaTime(lua_State* L)
{
    lua_pushnumber(L, s_deltaTime);
    return 1;
}

void ScriptAPI::AS_Print(const std::string& msg)
{
    DEBUG_LOG << "[AngelScript] " << msg << std::endl;
}

DabozzEngine::ECS::EntityID ScriptAPI::AS_CreateEntity()
{
    if (!s_world) return ECS::INVALID_ENTITY;

    ECS::EntityID entity = s_world->createEntity();
    s_world->addComponent<ECS::Transform>(entity);
    return entity;
}

void ScriptAPI::AS_DestroyEntity(DabozzEngine::ECS::EntityID entity)
{
    if (!s_world) return;
    s_world->destroyEntity(entity);
}

void ScriptAPI::AS_SetEntityPosition(DabozzEngine::ECS::EntityID entity, float x, float y, float z)
{
    if (!s_world) return;

    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);
    if (transform) {
        transform->position = QVector3D(x, y, z);
    }
}

void ScriptAPI::AS_GetEntityPosition(DabozzEngine::ECS::EntityID entity, float& x, float& y, float& z)
{
    if (!s_world) {
        x = y = z = 0.0f;
        return;
    }

    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);
    if (transform) {
        x = transform->position.x();
        y = transform->position.y();
        z = transform->position.z();
    } else {
        x = y = z = 0.0f;
    }
}

}
}

namespace DabozzEngine {
namespace Scripting {

int ScriptAPI::Lua_GetEntityScale(lua_State* L)
{
    if (!s_world) {
        lua_pushnumber(L, 1);
        lua_pushnumber(L, 1);
        lua_pushnumber(L, 1);
        return 3;
    }

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);

    if (transform) {
        lua_pushnumber(L, transform->scale.x());
        lua_pushnumber(L, transform->scale.y());
        lua_pushnumber(L, transform->scale.z());
    } else {
        lua_pushnumber(L, 1);
        lua_pushnumber(L, 1);
        lua_pushnumber(L, 1);
    }
    return 3;
}

int ScriptAPI::Lua_SetEntityScale(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));

    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);
    if (transform) {
        transform->scale = QVector3D(x, y, z);
    }
    return 0;
}

int ScriptAPI::Lua_AddRigidbody(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float mass = static_cast<float>(luaL_checknumber(L, 2));
    bool isStatic = lua_toboolean(L, 3);

    s_world->addComponent<ECS::RigidBody>(entity, mass, isStatic);
    return 0;
}

int ScriptAPI::Lua_SetVelocity(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));

    ECS::RigidBody* rb = s_world->getComponent<ECS::RigidBody>(entity);
    if (rb) {
        rb->velocity = QVector3D(x, y, z);
    }
    return 0;
}

int ScriptAPI::Lua_GetVelocity(lua_State* L)
{
    if (!s_world) {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        return 3;
    }

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    ECS::RigidBody* rb = s_world->getComponent<ECS::RigidBody>(entity);

    if (rb) {
        lua_pushnumber(L, rb->velocity.x());
        lua_pushnumber(L, rb->velocity.y());
        lua_pushnumber(L, rb->velocity.z());
    } else {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
    }
    return 3;
}

int ScriptAPI::Lua_ApplyForce(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));

    ECS::RigidBody* rb = s_world->getComponent<ECS::RigidBody>(entity);
    if (rb) {
        rb->velocity += QVector3D(x, y, z);
    }
    return 0;
}

int ScriptAPI::Lua_AddBoxCollider(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float sizeX = static_cast<float>(luaL_checknumber(L, 2));
    float sizeY = static_cast<float>(luaL_checknumber(L, 3));
    float sizeZ = static_cast<float>(luaL_checknumber(L, 4));

    s_world->addComponent<ECS::BoxCollider>(entity, QVector3D(sizeX, sizeY, sizeZ));
    return 0;
}

int ScriptAPI::Lua_AddSphereCollider(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float radius = static_cast<float>(luaL_checknumber(L, 2));

    s_world->addComponent<ECS::SphereCollider>(entity, radius);
    return 0;
}

int ScriptAPI::Lua_LoadMesh(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    const char* path = luaL_checkstring(L, 2);

    ECS::Mesh* mesh = s_world->addComponent<ECS::Mesh>(entity);
    if (mesh) {
        mesh->modelPath = std::string(path);
    }
    return 0;
}

int ScriptAPI::Lua_CreateCube(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float size = static_cast<float>(luaL_checknumber(L, 2));

    ECS::Mesh* mesh = s_world->addComponent<ECS::Mesh>(entity);
    if (mesh) {
        float halfSize = size / 2.0f;
        
        mesh->vertices = {
            -halfSize, -halfSize, -halfSize,  halfSize, -halfSize, -halfSize,  halfSize,  halfSize, -halfSize, -halfSize,  halfSize, -halfSize,
            -halfSize, -halfSize,  halfSize,  halfSize, -halfSize,  halfSize,  halfSize,  halfSize,  halfSize, -halfSize,  halfSize,  halfSize,
            -halfSize,  halfSize,  halfSize, -halfSize,  halfSize, -halfSize, -halfSize, -halfSize, -halfSize, -halfSize, -halfSize,  halfSize,
             halfSize,  halfSize,  halfSize,  halfSize,  halfSize, -halfSize,  halfSize, -halfSize, -halfSize,  halfSize, -halfSize,  halfSize,
            -halfSize, -halfSize, -halfSize,  halfSize, -halfSize, -halfSize,  halfSize, -halfSize,  halfSize, -halfSize, -halfSize,  halfSize,
            -halfSize,  halfSize, -halfSize,  halfSize,  halfSize, -halfSize,  halfSize,  halfSize,  halfSize, -halfSize,  halfSize,  halfSize
        };
        
        mesh->normals = {
            0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
            0,0,1, 0,0,1, 0,0,1, 0,0,1,
            -1,0,0, -1,0,0, -1,0,0, -1,0,0,
            1,0,0, 1,0,0, 1,0,0, 1,0,0,
            0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0,
            0,1,0, 0,1,0, 0,1,0, 0,1,0
        };
        
        mesh->indices = {
            0,1,2, 2,3,0,
            4,5,6, 6,7,4,
            8,9,10, 10,11,8,
            12,13,14, 14,15,12,
            16,17,18, 18,19,16,
            20,21,22, 22,23,20
        };
    }
    return 0;
}

int ScriptAPI::Lua_PlaySound(lua_State* L)
{
    const char* soundPath = luaL_checkstring(L, 1);
    DEBUG_LOG << "[Audio] Playing sound: " << soundPath << std::endl;
    return 0;
}

void ScriptAPI::AS_SetEntityRotation(DabozzEngine::ECS::EntityID entity, float x, float y, float z)
{
    if (!s_world) return;

    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);
    if (transform) {
        transform->rotation = QQuaternion::fromEulerAngles(x, y, z);
    }
}

void ScriptAPI::AS_SetEntityScale(DabozzEngine::ECS::EntityID entity, float x, float y, float z)
{
    if (!s_world) return;

    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);
    if (transform) {
        transform->scale = QVector3D(x, y, z);
    }
}

void ScriptAPI::AS_AddRigidbody(DabozzEngine::ECS::EntityID entity, float mass, bool isStatic)
{
    if (!s_world) return;
    s_world->addComponent<ECS::RigidBody>(entity, mass, isStatic);
}

void ScriptAPI::AS_SetVelocity(DabozzEngine::ECS::EntityID entity, float x, float y, float z)
{
    if (!s_world) return;

    ECS::RigidBody* rb = s_world->getComponent<ECS::RigidBody>(entity);
    if (rb) {
        rb->velocity = QVector3D(x, y, z);
    }
}

void ScriptAPI::AS_ApplyForce(DabozzEngine::ECS::EntityID entity, float x, float y, float z)
{
    if (!s_world) return;

    ECS::RigidBody* rb = s_world->getComponent<ECS::RigidBody>(entity);
    if (rb) {
        rb->velocity += QVector3D(x, y, z);
    }
}

void ScriptAPI::AS_AddBoxCollider(DabozzEngine::ECS::EntityID entity, float sizeX, float sizeY, float sizeZ)
{
    if (!s_world) return;
    s_world->addComponent<ECS::BoxCollider>(entity, QVector3D(sizeX, sizeY, sizeZ));
}

void ScriptAPI::AS_LoadMesh(DabozzEngine::ECS::EntityID entity, const std::string& path)
{
    if (!s_world) return;

    ECS::Mesh* mesh = s_world->addComponent<ECS::Mesh>(entity);
    if (mesh) {
        mesh->modelPath = path;
    }
}

void ScriptAPI::AS_CreateCube(DabozzEngine::ECS::EntityID entity, float size)
{
    if (!s_world) return;

    ECS::Mesh* mesh = s_world->addComponent<ECS::Mesh>(entity);
    if (mesh) {
        float halfSize = size / 2.0f;
        
        mesh->vertices = {
            -halfSize, -halfSize, -halfSize,  halfSize, -halfSize, -halfSize,  halfSize,  halfSize, -halfSize, -halfSize,  halfSize, -halfSize,
            -halfSize, -halfSize,  halfSize,  halfSize, -halfSize,  halfSize,  halfSize,  halfSize,  halfSize, -halfSize,  halfSize,  halfSize,
            -halfSize,  halfSize,  halfSize, -halfSize,  halfSize, -halfSize, -halfSize, -halfSize, -halfSize, -halfSize, -halfSize,  halfSize,
             halfSize,  halfSize,  halfSize,  halfSize,  halfSize, -halfSize,  halfSize, -halfSize, -halfSize,  halfSize, -halfSize,  halfSize,
            -halfSize, -halfSize, -halfSize,  halfSize, -halfSize, -halfSize,  halfSize, -halfSize,  halfSize, -halfSize, -halfSize,  halfSize,
            -halfSize,  halfSize, -halfSize,  halfSize,  halfSize, -halfSize,  halfSize,  halfSize,  halfSize, -halfSize,  halfSize,  halfSize
        };
        
        mesh->normals = {
            0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
            0,0,1, 0,0,1, 0,0,1, 0,0,1,
            -1,0,0, -1,0,0, -1,0,0, -1,0,0,
            1,0,0, 1,0,0, 1,0,0, 1,0,0,
            0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0,
            0,1,0, 0,1,0, 0,1,0, 0,1,0
        };
        
        mesh->indices = {
            0,1,2, 2,3,0,
            4,5,6, 6,7,4,
            8,9,10, 10,11,8,
            12,13,14, 14,15,12,
            16,17,18, 18,19,16,
            20,21,22, 22,23,20
        };
    }
}

float ScriptAPI::AS_GetDeltaTime()
{
    return s_deltaTime;
}

int ScriptAPI::Lua_FindEntityByName(lua_State* L)
{
    if (!s_world) {
        lua_pushnil(L);
        return 1;
    }

    const char* name = luaL_checkstring(L, 1);
    QString qname = QString::fromUtf8(name);

    for (ECS::EntityID entity : s_world->getEntities()) {
        ECS::Name* nameComp = s_world->getComponent<ECS::Name>(entity);
        if (nameComp && nameComp->name == qname) {
            lua_pushinteger(L, entity);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

int ScriptAPI::Lua_SetEntityName(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    const char* name = luaL_checkstring(L, 2);

    ECS::Name* nameComp = s_world->getComponent<ECS::Name>(entity);
    if (!nameComp) {
        nameComp = s_world->addComponent<ECS::Name>(entity);
    }
    if (nameComp) {
        nameComp->name = QString::fromUtf8(name);
    }
    return 0;
}

int ScriptAPI::Lua_GetEntityName(lua_State* L)
{
    if (!s_world) {
        lua_pushstring(L, "");
        return 1;
    }

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    ECS::Name* nameComp = s_world->getComponent<ECS::Name>(entity);

    if (nameComp) {
        lua_pushstring(L, nameComp->name.toUtf8().constData());
    } else {
        lua_pushstring(L, "");
    }
    return 1;
}

int ScriptAPI::Lua_RaycastFromCamera(lua_State* L)
{
    DEBUG_LOG << "[Raycast] Not yet implemented" << std::endl;
    lua_pushnil(L);
    return 1;
}

int ScriptAPI::Lua_GetMousePosition(lua_State* L)
{
    QPoint pos = DabozzEngine::Input::InputManager::getInstance().getMousePosition();
    lua_pushnumber(L, pos.x());
    lua_pushnumber(L, pos.y());
    return 2;
}

int ScriptAPI::Lua_IsKeyPressed(lua_State* L)
{
    int keyCode = static_cast<int>(luaL_checkinteger(L, 1));
    bool pressed = DabozzEngine::Input::InputManager::getInstance().isKeyPressed(keyCode);
    lua_pushboolean(L, pressed);
    return 1;
}

int ScriptAPI::Lua_IsKeyDown(lua_State* L)
{
    int keyCode = static_cast<int>(luaL_checkinteger(L, 1));
    bool down = DabozzEngine::Input::InputManager::getInstance().isKeyDown(keyCode);
    lua_pushboolean(L, down);
    return 1;
}

int ScriptAPI::Lua_IsMouseButtonPressed(lua_State* L)
{
    int button = static_cast<int>(luaL_checkinteger(L, 1));
    Qt::MouseButton qtButton = static_cast<Qt::MouseButton>(button);
    bool pressed = DabozzEngine::Input::InputManager::getInstance().isMouseButtonPressed(qtButton);
    lua_pushboolean(L, pressed);
    return 1;
}

int ScriptAPI::Lua_GetAllEntities(lua_State* L)
{
    if (!s_world) {
        lua_newtable(L);
        return 1;
    }

    const std::vector<ECS::EntityID>& entities = s_world->getEntities();
    lua_createtable(L, entities.size(), 0);

    for (size_t i = 0; i < entities.size(); i++) {
        lua_pushinteger(L, entities[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

int ScriptAPI::Lua_HasComponent(lua_State* L)
{
    if (!s_world) {
        lua_pushboolean(L, false);
        return 1;
    }

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    const char* componentName = luaL_checkstring(L, 2);
    QString compName = QString::fromUtf8(componentName);

    bool hasComp = false;
    if (compName == "Transform") hasComp = s_world->hasComponent<ECS::Transform>(entity);
    else if (compName == "RigidBody") hasComp = s_world->hasComponent<ECS::RigidBody>(entity);
    else if (compName == "Mesh") hasComp = s_world->hasComponent<ECS::Mesh>(entity);
    else if (compName == "BoxCollider") hasComp = s_world->hasComponent<ECS::BoxCollider>(entity);
    else if (compName == "SphereCollider") hasComp = s_world->hasComponent<ECS::SphereCollider>(entity);

    lua_pushboolean(L, hasComp);
    return 1;
}

int ScriptAPI::Lua_SetGravity(lua_State* L)
{
    float x = static_cast<float>(luaL_checknumber(L, 1));
    float y = static_cast<float>(luaL_checknumber(L, 2));
    float z = static_cast<float>(luaL_checknumber(L, 3));

    DEBUG_LOG << "[Physics] Setting gravity to (" << x << ", " << y << ", " << z << ")" << std::endl;
    return 0;
}

int ScriptAPI::Lua_LoadScene(lua_State* L)
{
    const char* scenePath = luaL_checkstring(L, 1);
    DEBUG_LOG << "[Scene] Loading scene: " << scenePath << std::endl;
    return 0;
}

int ScriptAPI::Lua_SaveScene(lua_State* L)
{
    const char* scenePath = luaL_checkstring(L, 1);
    DEBUG_LOG << "[Scene] Saving scene: " << scenePath << std::endl;
    return 0;
}

int ScriptAPI::Lua_InstantiatePrefab(lua_State* L)
{
    const char* prefabPath = luaL_checkstring(L, 1);
    DEBUG_LOG << "[Prefab] Instantiating: " << prefabPath << std::endl;
    lua_pushnil(L);
    return 1;
}

int ScriptAPI::Lua_LookAt(lua_State* L)
{
    if (!s_world) return 0;

    ECS::EntityID entity = static_cast<ECS::EntityID>(luaL_checkinteger(L, 1));
    float targetX = static_cast<float>(luaL_checknumber(L, 2));
    float targetY = static_cast<float>(luaL_checknumber(L, 3));
    float targetZ = static_cast<float>(luaL_checknumber(L, 4));

    ECS::Transform* transform = s_world->getComponent<ECS::Transform>(entity);
    if (transform) {
        QVector3D direction = QVector3D(targetX, targetY, targetZ) - transform->position;
        direction.normalize();
        
        float yaw = atan2(direction.x(), direction.z()) * 180.0f / 3.14159f;
        float pitch = asin(-direction.y()) * 180.0f / 3.14159f;
        
        transform->rotation = QQuaternion::fromEulerAngles(pitch, yaw, 0);
    }
    return 0;
}

int ScriptAPI::Lua_Distance(lua_State* L)
{
    float x1 = static_cast<float>(luaL_checknumber(L, 1));
    float y1 = static_cast<float>(luaL_checknumber(L, 2));
    float z1 = static_cast<float>(luaL_checknumber(L, 3));
    float x2 = static_cast<float>(luaL_checknumber(L, 4));
    float y2 = static_cast<float>(luaL_checknumber(L, 5));
    float z2 = static_cast<float>(luaL_checknumber(L, 6));

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    float dist = (v2 - v1).length();

    lua_pushnumber(L, dist);
    return 1;
}

int ScriptAPI::Lua_Lerp(lua_State* L)
{
    float a = static_cast<float>(luaL_checknumber(L, 1));
    float b = static_cast<float>(luaL_checknumber(L, 2));
    float t = static_cast<float>(luaL_checknumber(L, 3));

    float result = a + (b - a) * t;
    lua_pushnumber(L, result);
    return 1;
}

DabozzEngine::ECS::EntityID ScriptAPI::AS_FindEntityByName(const std::string& name)
{
    if (!s_world) return ECS::INVALID_ENTITY;

    QString qname = QString::fromStdString(name);
    for (ECS::EntityID entity : s_world->getEntities()) {
        ECS::Name* nameComp = s_world->getComponent<ECS::Name>(entity);
        if (nameComp && nameComp->name == qname) {
            return entity;
        }
    }
    return ECS::INVALID_ENTITY;
}

void ScriptAPI::AS_SetEntityName(DabozzEngine::ECS::EntityID entity, const std::string& name)
{
    if (!s_world) return;

    ECS::Name* nameComp = s_world->getComponent<ECS::Name>(entity);
    if (!nameComp) {
        nameComp = s_world->addComponent<ECS::Name>(entity);
    }
    if (nameComp) {
        nameComp->name = QString::fromStdString(name);
    }
}

bool ScriptAPI::AS_IsKeyPressed(int keyCode)
{
    return DabozzEngine::Input::InputManager::getInstance().isKeyPressed(keyCode);
}

bool ScriptAPI::AS_HasComponent(DabozzEngine::ECS::EntityID entity, const std::string& componentName)
{
    if (!s_world) return false;

    QString compName = QString::fromStdString(componentName);
    if (compName == "Transform") return s_world->hasComponent<ECS::Transform>(entity);
    if (compName == "RigidBody") return s_world->hasComponent<ECS::RigidBody>(entity);
    if (compName == "Mesh") return s_world->hasComponent<ECS::Mesh>(entity);
    if (compName == "BoxCollider") return s_world->hasComponent<ECS::BoxCollider>(entity);
    if (compName == "SphereCollider") return s_world->hasComponent<ECS::SphereCollider>(entity);
    return false;
}

void ScriptAPI::AS_SetGravity(float x, float y, float z)
{
    DEBUG_LOG << "[Physics] Setting gravity to (" << x << ", " << y << ", " << z << ")" << std::endl;
}

float ScriptAPI::AS_Distance(float x1, float y1, float z1, float x2, float y2, float z2)
{
    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    return (v2 - v1).length();
}

float ScriptAPI::AS_Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

}
}


// ===== New Physics API =====

int ScriptAPI::Lua_Raycast(lua_State* L)
{
    if (!s_world) return 0;
    
    float ox = luaL_checknumber(L, 1);
    float oy = luaL_checknumber(L, 2);
    float oz = luaL_checknumber(L, 3);
    float dx = luaL_checknumber(L, 4);
    float dy = luaL_checknumber(L, 5);
    float dz = luaL_checknumber(L, 6);
    float maxDist = luaL_optnumber(L, 7, 1000.0f);
    
    Physics::ButsuriEngine* butsuri = Physics::ButsuriEngine::getInstance();
    if (!butsuri) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    QVector3D origin(ox, oy, oz);
    QVector3D direction(dx, dy, dz);
    
    auto hit = butsuri->raycast(origin, direction, maxDist);
    
    if (hit.hit) {
        lua_pushboolean(L, true);
        lua_pushnumber(L, hit.point.x());
        lua_pushnumber(L, hit.point.y());
        lua_pushnumber(L, hit.point.z());
        lua_pushnumber(L, hit.distance);
        lua_pushinteger(L, hit.bodyId);
        return 6;
    } else {
        lua_pushboolean(L, false);
        return 1;
    }
}

int ScriptAPI::Lua_AddSphereRigidbody(lua_State* L)
{
    if (!s_world) return 0;
    
    ECS::EntityID entity = luaL_checkinteger(L, 1);
    float radius = luaL_checknumber(L, 2);
    float mass = luaL_checknumber(L, 3);
    bool isStatic = lua_toboolean(L, 4);
    
    auto* rb = s_world->addComponent<ECS::RigidBody>(entity, mass, isStatic, false);
    if (rb) {
        auto* sphereCol = s_world->addComponent<ECS::SphereCollider>(entity);
        if (sphereCol) {
            sphereCol->radius = radius;
        }
    }
    
    return 0;
}

// ===== New Audio API =====

int ScriptAPI::Lua_PlayAudio(lua_State* L)
{
    if (!s_world) return 0;
    
    ECS::EntityID entity = luaL_checkinteger(L, 1);
    
    auto* audio = s_world->getComponent<ECS::AudioSource>(entity);
    if (audio) {
        audio->playOnStart = true;
    }
    
    return 0;
}

int ScriptAPI::Lua_StopAudio(lua_State* L)
{
    if (!s_world) return 0;
    
    ECS::EntityID entity = luaL_checkinteger(L, 1);
    
    auto* audio = s_world->getComponent<ECS::AudioSource>(entity);
    if (audio) {
        audio->isPlaying = false;
    }
    
    return 0;
}

int ScriptAPI::Lua_PauseAudio(lua_State* L)
{
    if (!s_world) return 0;
    
    ECS::EntityID entity = luaL_checkinteger(L, 1);
    
    auto* audio = s_world->getComponent<ECS::AudioSource>(entity);
    if (audio) {
        audio->isPlaying = false;
    }
    
    return 0;
}

int ScriptAPI::Lua_SetAudioVolume(lua_State* L)
{
    if (!s_world) return 0;
    
    ECS::EntityID entity = luaL_checkinteger(L, 1);
    float volume = luaL_checknumber(L, 2);
    
    auto* audio = s_world->getComponent<ECS::AudioSource>(entity);
    if (audio) {
        audio->volume = volume;
    }
    
    return 0;
}

int ScriptAPI::Lua_SetAudioSpatial(lua_State* L)
{
    if (!s_world) return 0;
    
    ECS::EntityID entity = luaL_checkinteger(L, 1);
    bool spatial = lua_toboolean(L, 2);
    
    auto* audio = s_world->getComponent<ECS::AudioSource>(entity);
    if (audio) {
        audio->spatial = spatial;
    }
    
    return 0;
}
