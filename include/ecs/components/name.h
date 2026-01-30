#pragma once

#include "ecs/component.h"
#include <QString>

namespace DabozzEngine {
namespace ECS {

struct Name : public Component {
    QString name;

    Name() = default;
    Name(const QString& n) : name(n) {}
};

}
}
