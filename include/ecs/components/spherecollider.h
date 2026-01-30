#pragma once
#include "collider.h"

namespace DabozzEngine::ECS {

struct SphereCollider : public Collider {
    float radius;
    
    SphereCollider(float r = 0.5f, bool trigger = false)
        : Collider(ColliderType::Sphere, trigger), radius(r) {}
};

}
