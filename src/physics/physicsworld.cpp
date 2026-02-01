#include "physics/physicsworld.h"

namespace DabozzEngine::Physics {

static PhysicsWorld* g_instance = nullptr;

PhysicsWorld::PhysicsWorld()
{
    g_instance = this;
}

PhysicsWorld::~PhysicsWorld()
{
    shutdown();
    g_instance = nullptr;
}

void PhysicsWorld::initialize()
{
    // Custom physics system initialization
    // TODO: Initialize your custom physics implementation
}

void PhysicsWorld::shutdown()
{
    // Custom physics system cleanup
    // TODO: Cleanup your custom physics implementation
}

void PhysicsWorld::update(float deltaTime)
{
    // Custom physics update
    // TODO: Update your custom physics implementation
}

PhysicsWorld* PhysicsWorld::getInstance()
{
    return g_instance;
}

}
