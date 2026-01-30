#include "physics/physicssystem.h"
#include "ecs/components/transform.h"
#include "ecs/components/rigidbody.h"
#include "ecs/components/collider.h"
#include "ecs/components/boxcollider.h"
#include "ecs/components/spherecollider.h"
#include "physics/simplephysics.h"
#include "debug/logger.h"

namespace DabozzEngine::Systems {

PhysicsSystem::PhysicsSystem(ECS::World* world)
    : m_world(world)
    , m_butsuri(nullptr)
{
}

PhysicsSystem::~PhysicsSystem()
{
    shutdown();
}

void PhysicsSystem::initialize()
{
    m_butsuri = Physics::ButsuriEngine::getInstance();
}

void PhysicsSystem::shutdown()
{
    // Don't shutdown the engine here - MainWindow owns it
}

void PhysicsSystem::update(float deltaTime)
{
    if (!m_world || !m_butsuri) {
        DEBUG_LOG << "Physics update: world or butsuri is null" << std::endl;
        return;
    }
    
    createPhysicsBodies();
    m_butsuri->update(deltaTime);
    syncTransforms();
}

void PhysicsSystem::createPhysicsBodies()
{
    if (!m_world || !m_butsuri) return;
    
    for (ECS::EntityID entity : m_world->getEntities()) {
        ECS::Transform* transform = m_world->getComponent<ECS::Transform>(entity);
        ECS::RigidBody* rigidBody = m_world->getComponent<ECS::RigidBody>(entity);
        ECS::BoxCollider* boxCollider = m_world->getComponent<ECS::BoxCollider>(entity);
        
        if (!transform || !rigidBody || !boxCollider) continue;
        
        // Only create body if it doesn't exist yet
        if (rigidBody->bodyId < 0) {
            QVector3D size(boxCollider->size.x(), boxCollider->size.y(), boxCollider->size.z());
            QVector3D pos(transform->position.x(), transform->position.y(), transform->position.z());
            
            rigidBody->bodyId = m_butsuri->createBody(pos, size, rigidBody->mass, rigidBody->isStatic);
        }
    }
}

void PhysicsSystem::syncTransforms()
{
    if (!m_world || !m_butsuri) return;
    
    for (ECS::EntityID entity : m_world->getEntities()) {
        ECS::Transform* transform = m_world->getComponent<ECS::Transform>(entity);
        ECS::RigidBody* rigidBody = m_world->getComponent<ECS::RigidBody>(entity);
        
        if (!transform || !rigidBody || rigidBody->bodyId < 0) continue;
        
        Physics::RigidBodyState* body = m_butsuri->getBody(rigidBody->bodyId);
        if (body) {
            transform->position.setX(body->position.x());
            transform->position.setY(body->position.y());
            transform->position.setZ(body->position.z());
        }
    }
}

}
