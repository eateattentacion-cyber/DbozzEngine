#pragma once

#include "ecs/component.h"
#include <vector>

namespace DabozzEngine {
namespace ECS {

struct Hierarchy : public Component {
    EntityID parent;
    std::vector<EntityID> children;

    Hierarchy() : parent(0) {}
};

}
}
