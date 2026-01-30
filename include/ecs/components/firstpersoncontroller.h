#pragma once

#include "ecs/component.h"
#include <QVector3D>

namespace DabozzEngine {
namespace ECS {

struct FirstPersonController : public Component {
    float moveSpeed;
    float lookSpeed;
    float pitch;
    float yaw;
    
    bool moveForward;
    bool moveBackward;
    bool moveLeft;
    bool moveRight;
    bool moveUp;
    bool moveDown;
    
    FirstPersonController()
        : moveSpeed(5.0f)
        , lookSpeed(0.1f)
        , pitch(0.0f)
        , yaw(0.0f)
        , moveForward(false)
        , moveBackward(false)
        , moveLeft(false)
        , moveRight(false)
        , moveUp(false)
        , moveDown(false)
    {
    }
};

}
}
