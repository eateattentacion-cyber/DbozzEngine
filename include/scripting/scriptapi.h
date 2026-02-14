/////////////////////////////////////////////////////////////////////////////
// scriptapi.h                                                             //
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

#pragma once

#include "scripting/scriptengine.h"
#include "ecs/entity.h"
#include <QVector3D>

namespace DabozzEngine {

namespace ECS {
    class World;
}

namespace Scripting {

class ScriptAPI {
public:
    static void SetDeltaTime(float dt) { s_deltaTime = dt; }
    
    static void RegisterLuaAPI(lua_State* L, ECS::World* world);
    static void RegisterAngelScriptAPI(asIScriptEngine* engine, ECS::World* world);

private:
    static int Lua_Print(lua_State* L);
    static int Lua_CreateEntity(lua_State* L);
    static int Lua_DestroyEntity(lua_State* L);
    static int Lua_GetEntityPosition(lua_State* L);
    static int Lua_SetEntityPosition(lua_State* L);
    static int Lua_GetEntityRotation(lua_State* L);
    static int Lua_SetEntityRotation(lua_State* L);
    static int Lua_GetEntityScale(lua_State* L);
    static int Lua_SetEntityScale(lua_State* L);
    static int Lua_GetDeltaTime(lua_State* L);
    static int Lua_AddRigidbody(lua_State* L);
    static int Lua_SetVelocity(lua_State* L);
    static int Lua_GetVelocity(lua_State* L);
    static int Lua_ApplyForce(lua_State* L);
    static int Lua_AddBoxCollider(lua_State* L);
    static int Lua_AddSphereCollider(lua_State* L);
    static int Lua_LoadMesh(lua_State* L);
    static int Lua_CreateCube(lua_State* L);
    static int Lua_PlaySound(lua_State* L);
    static int Lua_FindEntityByName(lua_State* L);
    static int Lua_SetEntityName(lua_State* L);
    static int Lua_GetEntityName(lua_State* L);
    static int Lua_RaycastFromCamera(lua_State* L);
    static int Lua_GetMousePosition(lua_State* L);
    static int Lua_IsKeyPressed(lua_State* L);
    static int Lua_IsKeyDown(lua_State* L);
    static int Lua_IsMouseButtonPressed(lua_State* L);
    static int Lua_GetAllEntities(lua_State* L);
    static int Lua_HasComponent(lua_State* L);
    static int Lua_SetGravity(lua_State* L);
    static int Lua_LoadScene(lua_State* L);
    static int Lua_SaveScene(lua_State* L);
    static int Lua_InstantiatePrefab(lua_State* L);
    static int Lua_LookAt(lua_State* L);
    static int Lua_Distance(lua_State* L);
    static int Lua_Lerp(lua_State* L);

    static void AS_Print(const std::string& msg);
    static DabozzEngine::ECS::EntityID AS_CreateEntity();
    static void AS_DestroyEntity(DabozzEngine::ECS::EntityID entity);
    static void AS_SetEntityPosition(DabozzEngine::ECS::EntityID entity, float x, float y, float z);
    static void AS_GetEntityPosition(DabozzEngine::ECS::EntityID entity, float& x, float& y, float& z);
    static void AS_SetEntityRotation(DabozzEngine::ECS::EntityID entity, float x, float y, float z);
    static void AS_SetEntityScale(DabozzEngine::ECS::EntityID entity, float x, float y, float z);
    static void AS_AddRigidbody(DabozzEngine::ECS::EntityID entity, float mass, bool isStatic);
    static void AS_SetVelocity(DabozzEngine::ECS::EntityID entity, float x, float y, float z);
    static void AS_ApplyForce(DabozzEngine::ECS::EntityID entity, float x, float y, float z);
    static void AS_AddBoxCollider(DabozzEngine::ECS::EntityID entity, float sizeX, float sizeY, float sizeZ);
    static void AS_LoadMesh(DabozzEngine::ECS::EntityID entity, const std::string& path);
    static void AS_CreateCube(DabozzEngine::ECS::EntityID entity, float size);
    static float AS_GetDeltaTime();
    static DabozzEngine::ECS::EntityID AS_FindEntityByName(const std::string& name);
    static void AS_SetEntityName(DabozzEngine::ECS::EntityID entity, const std::string& name);
    static bool AS_IsKeyPressed(int keyCode);
    static bool AS_HasComponent(DabozzEngine::ECS::EntityID entity, const std::string& componentName);
    static void AS_SetGravity(float x, float y, float z);
    static float AS_Distance(float x1, float y1, float z1, float x2, float y2, float z2);
    static float AS_Lerp(float a, float b, float t);

    static ECS::World* s_world;
    static float s_deltaTime;
};

}
}
