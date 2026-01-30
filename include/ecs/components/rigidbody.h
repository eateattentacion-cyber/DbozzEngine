#pragma once
#include <QVector3D>
#include "ecs/component.h"

namespace DabozzEngine::ECS {

struct RigidBody : public Component {
    float mass;
    bool isStatic;
    bool useGravity;
    QVector3D velocity;
    QVector3D angularVelocity;
    float drag;
    float angularDrag;
    int bodyId; // Butsuri body ID
    
    RigidBody(float m = 1.0f, bool stat = false, bool grav = true)
        : mass(m), isStatic(stat), useGravity(grav), velocity(0, 0, 0), angularVelocity(0, 0, 0), drag(0.0f), angularDrag(0.05f), bodyId(-1) {}
};

}
