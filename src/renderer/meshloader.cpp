#include "renderer/meshloader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QFileInfo>
#include <QDir>
#include "debug/logger.h"

namespace DabozzEngine {
namespace Renderer {

std::vector<ECS::Mesh> MeshLoader::LoadMesh(const std::string& filepath) {
    std::vector<ECS::Mesh> meshes;
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(filepath, 
        aiProcess_Triangulate | 
        aiProcess_GenNormals | 
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString();
        return meshes;
    }
    
    if (scene->mNumMeshes == 0) {
        std::cerr << "No meshes found in file: " << filepath;
        return meshes;
    }

    DEBUG_LOG << "Scene has " << scene->mNumTextures << " embedded textures" << std::endl;
    DEBUG_LOG << "Scene has " << scene->mNumMaterials << " materials" << std::endl;

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* aiMesh = scene->mMeshes[meshIndex];
        ECS::Mesh mesh;

        for (unsigned int i = 0; i < aiMesh->mNumVertices; i++) {
            mesh.vertices.push_back(aiMesh->mVertices[i].x);
            mesh.vertices.push_back(aiMesh->mVertices[i].y);
            mesh.vertices.push_back(aiMesh->mVertices[i].z);
            
            if (aiMesh->HasNormals()) {
                mesh.normals.push_back(aiMesh->mNormals[i].x);
                mesh.normals.push_back(aiMesh->mNormals[i].y);
                mesh.normals.push_back(aiMesh->mNormals[i].z);
            }
            
            if (aiMesh->mTextureCoords[0]) {
                mesh.texCoords.push_back(aiMesh->mTextureCoords[0][i].x);
                mesh.texCoords.push_back(aiMesh->mTextureCoords[0][i].y);
            }
        }
        
        for (unsigned int i = 0; i < aiMesh->mNumFaces; i++) {
            aiFace face = aiMesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                mesh.indices.push_back(face.mIndices[j]);
            }
        }
        
        mesh.modelPath = filepath;
        
        DEBUG_LOG << "Mesh loaded: " << mesh.vertices.size()/3 << " vertices, " << mesh.indices.size()/3 << " triangles" << std::endl;
        
        if (scene->mNumMaterials > aiMesh->mMaterialIndex) {
            aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
            aiString texPath;

            DEBUG_LOG << "Checking material " << aiMesh->mMaterialIndex << " for textures..." << std::endl;
            DEBUG_LOG << "Material has " << material->GetTextureCount(aiTextureType_DIFFUSE) << " diffuse textures" << std::endl;
            
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                DEBUG_LOG << "Found texture path: " << texPath.C_Str() << std::endl;
                if (texPath.data[0] == '*') {
                    DEBUG_LOG << "Embedded texture detected" << std::endl;
                    int textureIndex = atoi(texPath.data + 1);
                    if (textureIndex >= 0 && textureIndex < (int)scene->mNumTextures) {
                        aiTexture* embeddedTexture = scene->mTextures[textureIndex];
                        if (embeddedTexture->mHeight == 0) {
                            mesh.embeddedTextureData.assign(
                                reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
                                reinterpret_cast<const unsigned char*>(embeddedTexture->pcData) + embeddedTexture->mWidth
                            );
                            mesh.embeddedTextureWidth = embeddedTexture->mWidth;
                            mesh.hasTexture = true;
                            mesh.texturePath = "embedded_compressed";
                            DEBUG_LOG << "Loaded compressed embedded texture: " << mesh.embeddedTextureData.size() << " bytes" << std::endl;
                        } else {
                            int pixelCount = embeddedTexture->mWidth * embeddedTexture->mHeight;
                            mesh.embeddedTextureData.resize(pixelCount * 4);
                            const aiTexel* texels = reinterpret_cast<const aiTexel*>(embeddedTexture->pcData);
                            for (int i = 0; i < pixelCount; i++) {
                                mesh.embeddedTextureData[i * 4 + 0] = texels[i].r;
                                mesh.embeddedTextureData[i * 4 + 1] = texels[i].g;
                                mesh.embeddedTextureData[i * 4 + 2] = texels[i].b;
                                mesh.embeddedTextureData[i * 4 + 3] = texels[i].a;
                            }
                            mesh.embeddedTextureWidth = embeddedTexture->mWidth;
                            mesh.embeddedTextureHeight = embeddedTexture->mHeight;
                            mesh.hasTexture = true;
                            mesh.texturePath = "embedded_raw";
                            DEBUG_LOG << "Loaded raw embedded texture: " << mesh.embeddedTextureWidth << "x" << mesh.embeddedTextureHeight << std::endl;
                        }
                    }
                } else {
                    QFileInfo modelFile(QString::fromStdString(filepath));
                    QDir modelDir = modelFile.absoluteDir();
                    QString textureFile = modelDir.absoluteFilePath(QString::fromUtf8(texPath.C_Str()));
                    if (QFileInfo::exists(textureFile)) {
                        mesh.hasTexture = true;
                        mesh.texturePath = textureFile.toStdString();
                        DEBUG_LOG << "Found external texture: " << mesh.texturePath << std::endl;
                    } else {
                        DEBUG_LOG << "External texture not found: " << textureFile.toStdString() << std::endl;
                    }
                }
            } else {
                DEBUG_LOG << "No diffuse texture found in material" << std::endl;
            }
        } else {
            DEBUG_LOG << "No material for this mesh" << std::endl;
        }
        meshes.push_back(mesh);
    }
    
    return meshes;
}

}
}
