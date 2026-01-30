#pragma once
#include "ecs/component.h"

namespace DabozzEngine::ECS {

enum class ColliderType {
    Box,
    Sphere,
    Capsule,
    Mesh
};

struct Collider : public Component {
    ColliderType type;
    bool isTrigger;
    
    Collider(ColliderType t = ColliderType::Box, bool trigger = false)
        : type(t), isTrigger(trigger) {}
};

}
