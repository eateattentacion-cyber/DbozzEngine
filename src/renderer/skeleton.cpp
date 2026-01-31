#include "renderer/skeleton.h"
#include "renderer/animation.h"
#include "debug/logger.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

namespace DabozzEngine::Renderer {

static glm::mat4 convertMatrixToGLM(const aiMatrix4x4& from)
{
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

void Skeleton::loadFromFile(const std::string& filepath)
{
    DEBUG_LOG << "Skeleton::loadFromFile: " << filepath << std::endl;
    
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    const aiScene* scene = importer.ReadFile(filepath, aiProcess_Triangulate);
    
    if (!scene || !scene->mRootNode) {
        DEBUG_LOG << "ERROR: Failed to load scene" << std::endl;
        return;
    }
    
    // Extract bone offset matrices from all meshes
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        if (mesh->HasBones()) {
            for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
                aiBone* bone = mesh->mBones[boneIndex];
                QString boneName = QString::fromStdString(bone->mName.C_Str());
                
                if (m_boneInfoMap.find(boneName) == m_boneInfoMap.end()) {
                    BoneInfo newBoneInfo;
                    newBoneInfo.id = m_boneCounter++;
                    newBoneInfo.offset = convertMatrixToGLM(bone->mOffsetMatrix);
                    m_boneInfoMap[boneName] = newBoneInfo;
                }
            }
        }
    }
    
    DEBUG_LOG << "Skeleton loaded " << m_boneInfoMap.size() << " bones with offset matrices" << std::endl;
}

}
