#include "physics/physicsworld.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace DabozzEngine::Physics {

// Broad phase layer interface
class BPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface
{
public:
    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return 2;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < 2);
        return JPH::BroadPhaseLayer(inLayer);
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)0: return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)1: return "MOVING";
        default: JPH_ASSERT(false); return "INVALID";
        }
    }
#endif
};

// Object vs broad phase layer filter
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case Layers::NON_MOVING:
            return inLayer2 == JPH::BroadPhaseLayer(Layers::MOVING);
        case Layers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

// Object layer pair filter
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case Layers::NON_MOVING:
            return inObject2 == Layers::MOVING;
        case Layers::MOVING:
            return inObject2 == Layers::NON_MOVING || inObject2 == Layers::MOVING;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

static BPLayerInterfaceImpl s_broadPhaseLayerInterface;
static ObjectVsBroadPhaseLayerFilterImpl s_objectVsBroadPhaseLayerFilter;
static ObjectLayerPairFilterImpl s_objectLayerPairFilter;

static PhysicsWorld* g_instance = nullptr;

PhysicsWorld::PhysicsWorld()
    : m_physicsSystem(nullptr)
    , m_bodyInterface(nullptr)
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
    // Jolt allocator and factory are initialized in main()
    // Just create our physics system
    
    m_tempAllocator = new JPH::TempAllocatorImpl(16 * 1024 * 1024);
    m_jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 0);
    
    m_physicsSystem = new JPH::PhysicsSystem();
    m_physicsSystem->Init(1024, 0, 1024, 1024, s_broadPhaseLayerInterface, s_objectVsBroadPhaseLayerFilter, s_objectLayerPairFilter);
    
    m_bodyInterface = &m_physicsSystem->GetBodyInterface();
}

void PhysicsWorld::shutdown()
{
    if (m_physicsSystem) {
        delete m_physicsSystem;
        m_physicsSystem = nullptr;
    }
    
    if (m_jobSystem) {
        delete m_jobSystem;
        m_jobSystem = nullptr;
    }
    
    if (m_tempAllocator) {
        delete m_tempAllocator;
        m_tempAllocator = nullptr;
    }
    
    if (JPH::Factory::sInstance) {
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }
}

void PhysicsWorld::update(float deltaTime)
{
    if (!m_physicsSystem) return;
    
    m_physicsSystem->Update(deltaTime, 1, m_tempAllocator, m_jobSystem);
}

PhysicsWorld* PhysicsWorld::getInstance()
{
    return g_instance;
}

}
