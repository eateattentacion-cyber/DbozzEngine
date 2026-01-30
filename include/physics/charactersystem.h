#pragma once

#include "ecs/system.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace JPH {
    class PhysicsSystem;
}

namespace DabozzEngine {
namespace Physics {
    class PhysicsWorld;
}

namespace Systems {

class CharacterSystem : public ECS::System {
public:
    CharacterSystem(ECS::World* world);
    ~CharacterSystem();

    void initialize() override;
    void shutdown() override;
    void update(ECS::World* world, float deltaTime) override;

private:
    Physics::PhysicsWorld* m_physicsWorld;
    
    // We need to keep track of created characters to update/destroy them
    // For now, we'll just iterate entities and check components
    void updateCharacters(float deltaTime);
    void createCharacter(ECS::EntityID entity);
};

}
}
