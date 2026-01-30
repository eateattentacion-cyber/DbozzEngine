#pragma once

#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/components/transform.h"
#include "ecs/components/name.h"
#include "ecs/components/hierarchy.h"
#include "ecs/components/mesh.h"
#include "ecs/components/firstpersoncontroller.h"
#include "ecs/components/collider.h"
#include "ecs/components/boxcollider.h"
#include "ecs/components/spherecollider.h"
#include "ecs/components/rigidbody.h"
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <vector>
#include <algorithm>

namespace DabozzEngine {
namespace ECS {

class World {
public:
    World();
    ~World();

    EntityID createEntity();
    void destroyEntity(EntityID entity);

    template<typename T, typename... Args>
    T* addComponent(EntityID entity, Args&&... args) {
        if (!hasEntity(entity)) return nullptr;
        
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* componentPtr = component.get();
        
        m_components[entity][typeid(T)] = std::move(component);
        return componentPtr;
    }

    template<typename T>
    T* getComponent(EntityID entity) {
        auto entityIt = m_components.find(entity);
        if (entityIt == m_components.end()) return nullptr;
        
        auto componentIt = entityIt->second.find(typeid(T));
        if (componentIt == entityIt->second.end()) return nullptr;
        
        return static_cast<T*>(componentIt->second.get());
    }

    template<typename T>
    bool hasComponent(EntityID entity) const {
        auto entityIt = m_components.find(entity);
        if (entityIt == m_components.end()) return false;
        
        return entityIt->second.count(typeid(T)) > 0;
    }

    template<typename T>
    void removeComponent(EntityID entity) {
        auto entityIt = m_components.find(entity);
        if (entityIt == m_components.end()) return;
        
        entityIt->second.erase(typeid(T));
    }

    bool hasEntity(EntityID entity) const {
        return std::find(m_entities.begin(), m_entities.end(), entity) != m_entities.end();
    }

    const std::vector<EntityID>& getEntities() const {
        return m_entities;
    }

    std::unordered_map<std::type_index, std::unique_ptr<Component>>* getComponents(EntityID entity) {
        auto entityIt = m_components.find(entity);
        if (entityIt == m_components.end()) return nullptr;
        return &entityIt->second;
    }

private:
    std::vector<EntityID> m_entities;
    EntityID m_nextEntityID;
    std::unordered_map<EntityID, std::unordered_map<std::type_index, std::unique_ptr<Component>>> m_components;
};

}
}
