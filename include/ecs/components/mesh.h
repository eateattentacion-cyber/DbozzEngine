#pragma once

#include "ecs/component.h"
#include <vector>
#include <string>

namespace DabozzEngine {
namespace ECS {

struct Mesh : public Component {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<unsigned int> indices;
    
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    unsigned int textureID = 0;
    bool isUploaded = false;
    bool hasTexture = false;
    
    std::string modelPath;
    std::string texturePath;
    std::vector<unsigned char> embeddedTextureData;
    int embeddedTextureWidth = 0;
    int embeddedTextureHeight = 0;

    Mesh() = default;
};

}
}
