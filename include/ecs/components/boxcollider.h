#pragma once
#include "collider.h"
#include <QVector3D>

namespace DabozzEngine::ECS {

struct BoxCollider : public Collider {
    QVector3D size;
    
    BoxCollider(const QVector3D& s = QVector3D(1.0f, 1.0f, 1.0f), bool trigger = false)
        : Collider(ColliderType::Box, trigger), size(s) {}
};

}
