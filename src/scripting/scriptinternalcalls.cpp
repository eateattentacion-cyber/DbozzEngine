#include "scripting/scriptinternalcalls.h"
#include "ecs/components/transform.h"
#include "debug/logger.h"
#include <mono/metadata/object.h>
#include <QGuiApplication>
#include <QCursor>

namespace DabozzEngine {
namespace Scripting {

ECS::World* ScriptInternalCalls::s_world = nullptr;

void ScriptInternalCalls::setWorld(ECS::World* world)
{
    s_world = world;
}

void ScriptInternalCalls::registerInternalCalls()
{
    // Entity API
    mono_add_internal_call("DabozzEngine.Entity::Internal_Create", (void*)Entity_Create);
    mono_add_internal_call("DabozzEngine.Entity::Internal_Destroy", (void*)Entity_Destroy);
    mono_add_internal_call("DabozzEngine.Entity::Internal_IsValid", (void*)Entity_IsValid);
    
    // Transform API
    mono_add_internal_call("DabozzEngine.Transform::Internal_GetPosition", (void*)Transform_GetPosition);
    mono_add_internal_call("DabozzEngine.Transform::Internal_SetPosition", (void*)Transform_SetPosition);
    mono_add_internal_call("DabozzEngine.Transform::Internal_GetRotation", (void*)Transform_GetRotation);
    mono_add_internal_call("DabozzEngine.Transform::Internal_SetRotation", (void*)Transform_SetRotation);
    mono_add_internal_call("DabozzEngine.Transform::Internal_GetScale", (void*)Transform_GetScale);
    mono_add_internal_call("DabozzEngine.Transform::Internal_SetScale", (void*)Transform_SetScale);
    
    // Input API
    mono_add_internal_call("DabozzEngine.Input::Internal_GetKey", (void*)Input_GetKey);
    mono_add_internal_call("DabozzEngine.Input::Internal_GetKeyDown", (void*)Input_GetKeyDown);
    mono_add_internal_call("DabozzEngine.Input::Internal_GetKeyUp", (void*)Input_GetKeyUp);
    mono_add_internal_call("DabozzEngine.Input::Internal_GetMousePosition", (void*)Input_GetMousePosition);
    
    // Debug API
    mono_add_internal_call("DabozzEngine.Debug::Internal_Log", (void*)Debug_Log);
}

// Entity API Implementation
uint32_t ScriptInternalCalls::Entity_Create()
{
    if (!s_world) return 0;
    return s_world->createEntity();
}

void ScriptInternalCalls::Entity_Destroy(uint32_t entityID)
{
    if (!s_world) return;
    s_world->destroyEntity(entityID);
}

bool ScriptInternalCalls::Entity_IsValid(uint32_t entityID)
{
    if (!s_world) return false;
    return s_world->hasEntity(entityID);
}

// Transform API Implementation
void ScriptInternalCalls::Transform_GetPosition(uint32_t entityID, float* outX, float* outY, float* outZ)
{
    if (!s_world) return;
    auto* transform = s_world->getComponent<ECS::Transform>(entityID);
    if (transform) {
        *outX = transform->position.x();
        *outY = transform->position.y();
        *outZ = transform->position.z();
    }
}

void ScriptInternalCalls::Transform_SetPosition(uint32_t entityID, float x, float y, float z)
{
    if (!s_world) return;
    auto* transform = s_world->getComponent<ECS::Transform>(entityID);
    if (transform) {
        transform->position = QVector3D(x, y, z);
    }
}

void ScriptInternalCalls::Transform_GetRotation(uint32_t entityID, float* outX, float* outY, float* outZ, float* outW)
{
    if (!s_world) return;
    auto* transform = s_world->getComponent<ECS::Transform>(entityID);
    if (transform) {
        *outX = transform->rotation.x();
        *outY = transform->rotation.y();
        *outZ = transform->rotation.z();
        *outW = transform->rotation.scalar();
    }
}

void ScriptInternalCalls::Transform_SetRotation(uint32_t entityID, float x, float y, float z, float w)
{
    if (!s_world) return;
    auto* transform = s_world->getComponent<ECS::Transform>(entityID);
    if (transform) {
        transform->rotation = QQuaternion(w, x, y, z);
    }
}

void ScriptInternalCalls::Transform_GetScale(uint32_t entityID, float* outX, float* outY, float* outZ)
{
    if (!s_world) return;
    auto* transform = s_world->getComponent<ECS::Transform>(entityID);
    if (transform) {
        *outX = transform->scale.x();
        *outY = transform->scale.y();
        *outZ = transform->scale.z();
    }
}

void ScriptInternalCalls::Transform_SetScale(uint32_t entityID, float x, float y, float z)
{
    if (!s_world) return;
    auto* transform = s_world->getComponent<ECS::Transform>(entityID);
    if (transform) {
        transform->scale = QVector3D(x, y, z);
    }
}

// Input API Implementation (stub - you'll need to implement proper input system)
bool ScriptInternalCalls::Input_GetKey(int keyCode)
{
    // TODO: Implement input system
    return false;
}

bool ScriptInternalCalls::Input_GetKeyDown(int keyCode)
{
    // TODO: Implement input system
    return false;
}

bool ScriptInternalCalls::Input_GetKeyUp(int keyCode)
{
    // TODO: Implement input system
    return false;
}

void ScriptInternalCalls::Input_GetMousePosition(float* outX, float* outY)
{
    QPoint pos = QCursor::pos();
    *outX = pos.x();
    *outY = pos.y();
}

// Debug API Implementation
void ScriptInternalCalls::Debug_Log(MonoString* message)
{
    char* cstr = mono_string_to_utf8(message);
    DEBUG_LOG << "[C# Script] " << cstr << std::endl;
    mono_free(cstr);
}

}
}
