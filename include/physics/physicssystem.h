#pragma once
#include "ecs/world.h"

namespace DabozzEngine {
namespace Physics {
    class ButsuriEngine;
}
namespace Systems {

class PhysicsSystem {
public:
    PhysicsSystem(ECS::World* world);
    ~PhysicsSystem();
    
    void initialize();
    void shutdown();
    void update(float deltaTime);
    
private:
    void createPhysicsBodies();
    void syncTransforms();
    
    ECS::World* m_world;
    Physics::ButsuriEngine* m_butsuri;
};

}
}
