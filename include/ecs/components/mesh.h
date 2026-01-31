#pragma once

#include "ecs/component.h"
#include <vector>
#include <string>

namespace DabozzEngine {
namespace ECS {

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    float position[3];
    float normal[3];
    float texCoords[2];
    int boneIDs[MAX_BONE_INFLUENCE];
    float weights[MAX_BONE_INFLUENCE];
};

struct Mesh : public Component {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<unsigned int> indices;
    
    // Bone data for skeletal animation
    std::vector<int> boneIds;
    std::vector<float> boneWeights;
    bool hasAnimation = false;
    
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    unsigned int boneVBO = 0;
    unsigned int weightVBO = 0;
    unsigned int textureID = 0;
    bool isUploaded = false;
    bool hasTexture = false;
    
    std::string modelPath;
    std::string texturePath;
    std::vector<unsigned char> embeddedTextureData;
    int embeddedTextureWidth = 0;
    int embeddedTextureHeight = 0;

    Mesh() {
        // Initialize bone data with -1 (no bone)
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            boneIds.push_back(-1);
            boneWeights.push_back(0.0f);
        }
    }
};

}
}
