#pragma once
#include <memory>

namespace JPH {
class PhysicsSystem;
class BodyInterface;
class TempAllocator;
class JobSystemThreadPool;
}

namespace DabozzEngine::Physics {

// Define layers
namespace Layers {
    static constexpr unsigned int NON_MOVING = 0;
    static constexpr unsigned int MOVING = 1;
}

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();
    
    void initialize();
    void shutdown();
    void update(float deltaTime);
    
    JPH::PhysicsSystem* getPhysicsSystem() { return m_physicsSystem; }
    JPH::BodyInterface* getBodyInterface() { return m_bodyInterface; }
    
    static PhysicsWorld* getInstance();
    
private:
    JPH::PhysicsSystem* m_physicsSystem;
    JPH::BodyInterface* m_bodyInterface;
    JPH::TempAllocator* m_tempAllocator;
    JPH::JobSystemThreadPool* m_jobSystem;
};

}
