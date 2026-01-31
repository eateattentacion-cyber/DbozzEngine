#pragma once

#include "ecs/components/mesh.h"
#include <string>
#include <vector>

namespace DabozzEngine {
namespace Renderer {

class Skeleton; // Forward declaration

class MeshLoader {
public:
    static std::vector<ECS::Mesh> LoadMesh(const std::string& filepath, Skeleton* skeleton = nullptr);
};

}
}
