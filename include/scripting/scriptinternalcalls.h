#pragma once

#include "ecs/world.h"
#include <mono/jit/jit.h>

namespace DabozzEngine {
namespace Scripting {

class ScriptInternalCalls {
public:
    static void registerInternalCalls();
    static void setWorld(ECS::World* world);
    
private:
    static ECS::World* s_world;
    
    // Entity API
    static uint32_t Entity_Create();
    static void Entity_Destroy(uint32_t entityID);
    static bool Entity_IsValid(uint32_t entityID);
    
    // Transform API
    static void Transform_GetPosition(uint32_t entityID, float* outX, float* outY, float* outZ);
    static void Transform_SetPosition(uint32_t entityID, float x, float y, float z);
    static void Transform_GetRotation(uint32_t entityID, float* outX, float* outY, float* outZ, float* outW);
    static void Transform_SetRotation(uint32_t entityID, float x, float y, float z, float w);
    static void Transform_GetScale(uint32_t entityID, float* outX, float* outY, float* outZ);
    static void Transform_SetScale(uint32_t entityID, float x, float y, float z);
    
    // Input API
    static bool Input_GetKey(int keyCode);
    static bool Input_GetKeyDown(int keyCode);
    static bool Input_GetKeyUp(int keyCode);
    static void Input_GetMousePosition(float* outX, float* outY);
    
    // Debug API
    static void Debug_Log(MonoString* message);
};

}
}
