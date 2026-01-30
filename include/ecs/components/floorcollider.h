#pragma once

#include "ecs/component.h"
#include <QVector3D>

namespace DabozzEngine {
namespace ECS {

struct FloorCollider : public Component {
    QVector3D size;
    
    FloorCollider()
        : size(10.0f, 0.1f, 10.0f)
    {
    }
};

}
}
