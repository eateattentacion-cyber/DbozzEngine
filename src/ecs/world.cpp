#include "ecs/world.h"

namespace DabozzEngine {
namespace ECS {

World::World()
    : m_nextEntityID(1)
{
}

World::~World()
{
    for (EntityID entity : m_entities) {
        destroyEntity(entity);
    }
}

EntityID World::createEntity()
{
    EntityID entity = m_nextEntityID++;
    m_entities.push_back(entity);
    return entity;
}

void World::destroyEntity(EntityID entity)
{
    auto it = std::find(m_entities.begin(), m_entities.end(), entity);
    if (it != m_entities.end()) {
        m_entities.erase(it);
        m_components.erase(entity);
    }
}

}
}
