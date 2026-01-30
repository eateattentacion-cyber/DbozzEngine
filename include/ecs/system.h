#pragma once

#include "ecs/world.h"
#include <vector>

namespace DabozzEngine {
namespace ECS {

class System {
public:
    virtual ~System() = default;
    virtual void update(World* world, float deltaTime) = 0;
};

}
}
