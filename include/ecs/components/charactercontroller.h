#pragma once

#include "ecs/component.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace DabozzEngine {
namespace ECS {

struct CharacterController : public Component {
    float maxSpeed = 6.0f;
    float jumpHeight = 1.2f;
    float characterHeight = 1.8f;
    float characterRadius = 0.4f;
    
    JPH::Ref<JPH::CharacterVirtual> character;
    
    bool isGrounded = false;
    
    CharacterController() = default;
};

}
}
