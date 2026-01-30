#pragma once

#include "ecs/component.h"
#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>

namespace DabozzEngine {
namespace ECS {

struct Transform : public Component {
    QVector3D position;
    QQuaternion rotation;
    QVector3D scale;

    Transform()
        : position(0.0f, 0.0f, 0.0f)
        , rotation(1.0f, 0.0f, 0.0f, 0.0f)
        , scale(1.0f, 1.0f, 1.0f)
    {
    }

    QMatrix4x4 getModelMatrix() const {
        QMatrix4x4 model;
        model.translate(position);
        model.rotate(rotation);
        model.scale(scale);
        return model;
    }
};

}
}
