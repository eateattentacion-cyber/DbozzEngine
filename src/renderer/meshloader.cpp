#include "renderer/meshloader.h"
#include "renderer/skeleton.h"
#include "renderer/animation.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <iostream>
#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QFileInfo>
#include <QDir>
#include "debug/logger.h"

namespace DabozzEngine {
namespace Renderer {

std::vector<ECS::Mesh> MeshLoader::LoadMesh(const std::string& filepath, Skeleton* skeleton) {
    std::vector<ECS::Mesh> meshes;
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

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

    DEBUG_LOG << "=== SCENE INFO ===" << std::endl;
    DEBUG_LOG << "Embedded textures: " << scene->mNumTextures << std::endl;
    DEBUG_LOG << "Materials: " << scene->mNumMaterials << std::endl;
    DEBUG_LOG << "Meshes: " << scene->mNumMeshes << std::endl;
    
    // List all embedded textures
    for (unsigned int i = 0; i < scene->mNumTextures; i++) {
        aiTexture* tex = scene->mTextures[i];
        DEBUG_LOG << "  Texture " << i << ": " 
                  << (tex->mHeight == 0 ? "compressed" : "raw") 
                  << " format=" << tex->achFormatHint 
                  << " size=" << tex->mWidth << "x" << tex->mHeight << std::endl;
    }

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
        
        // Extract bone data if mesh has bones
        if (aiMesh->HasBones()) {
            mesh.hasAnimation = true;
            const int maxBoneInfluence = 4;
            mesh.boneIds.resize(aiMesh->mNumVertices * maxBoneInfluence, -1);
            mesh.boneWeights.resize(aiMesh->mNumVertices * maxBoneInfluence, 0.0f);
            
            // Build bone name to ID map from skeleton if available
            std::map<std::string, int> boneNameToId;
            if (skeleton) {
                DEBUG_LOG << "=== MESH LOADER: Using skeleton for bone IDs ===" << std::endl;
                auto& boneInfoMap = skeleton->getBoneInfoMap();
                for (auto& pair : boneInfoMap) {
                    boneNameToId[pair.first.toStdString()] = pair.second.id;
                    DEBUG_LOG << "  Skeleton bone: " << pair.first.toStdString() << " -> ID " << pair.second.id << std::endl;
                }
            } else {
                DEBUG_LOG << "=== MESH LOADER: No skeleton provided, using local bone indices ===" << std::endl;
            }
            
            for (unsigned int boneIndex = 0; boneIndex < aiMesh->mNumBones; ++boneIndex) {
                aiBone* bone = aiMesh->mBones[boneIndex];
                std::string boneName = bone->mName.C_Str();
                
                // Get global bone ID from skeleton, or use local index as fallback
                int globalBoneId = boneIndex;
                if (skeleton && boneNameToId.find(boneName) != boneNameToId.end()) {
                    globalBoneId = boneNameToId[boneName];
                    DEBUG_LOG << "Mesh bone " << boneIndex << " '" << boneName << "' -> global ID " << globalBoneId << std::endl;
                } else {
                    DEBUG_LOG << "Mesh bone " << boneIndex << " '" << boneName << "' -> local ID " << globalBoneId << " (NOT IN SKELETON)" << std::endl;
                }
                
                for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                    unsigned int vertexId = bone->mWeights[weightIndex].mVertexId;
                    float weight = bone->mWeights[weightIndex].mWeight;
                    
                    // Find empty slot for this bone influence
                    for (int i = 0; i < maxBoneInfluence; i++) {
                        if (mesh.boneIds[vertexId * maxBoneInfluence + i] == -1) {
                            mesh.boneIds[vertexId * maxBoneInfluence + i] = globalBoneId;
                            mesh.boneWeights[vertexId * maxBoneInfluence + i] = weight;
                            break;
                        }
                    }
                }
            }
            DEBUG_LOG << "Mesh has " << aiMesh->mNumBones << " bones" << std::endl;
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

            DEBUG_LOG << "=== MATERIAL " << aiMesh->mMaterialIndex << " TEXTURE CHECK ===" << std::endl;
            DEBUG_LOG << "Diffuse textures: " << material->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
            DEBUG_LOG << "Base color textures: " << material->GetTextureCount(aiTextureType_BASE_COLOR) << std::endl;
            DEBUG_LOG << "Specular textures: " << material->GetTextureCount(aiTextureType_SPECULAR) << std::endl;
            DEBUG_LOG << "Emissive textures: " << material->GetTextureCount(aiTextureType_EMISSIVE) << std::endl;
            
            // Debug: Print all material properties
            DEBUG_LOG << "Material properties count: " << material->mNumProperties << std::endl;
            for (unsigned int i = 0; i < material->mNumProperties; i++) {
                aiMaterialProperty* prop = material->mProperties[i];
                DEBUG_LOG << "  Property: " << prop->mKey.C_Str() << " type=" << prop->mType << " dataLen=" << prop->mDataLength << std::endl;
            }
            
            // Try diffuse first, then base color (for PBR materials)
            bool foundTexture = false;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                foundTexture = true;
                DEBUG_LOG << "Found DIFFUSE texture: " << texPath.C_Str() << std::endl;
            } else if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS) {
                foundTexture = true;
                DEBUG_LOG << "Found BASE_COLOR texture: " << texPath.C_Str() << std::endl;
            }
            
            if (foundTexture) {
                if (texPath.data[0] == '*') {
                    DEBUG_LOG << "=== EMBEDDED TEXTURE ===" << std::endl;
                    int textureIndex = atoi(texPath.data + 1);
                    DEBUG_LOG << "Texture index: " << textureIndex << std::endl;
                    if (textureIndex >= 0 && textureIndex < (int)scene->mNumTextures) {
                        aiTexture* embeddedTexture = scene->mTextures[textureIndex];
                        DEBUG_LOG << "Texture format hint: " << (embeddedTexture->achFormatHint[0] ? embeddedTexture->achFormatHint : "none") << std::endl;
                        DEBUG_LOG << "Texture dimensions: " << embeddedTexture->mWidth << "x" << embeddedTexture->mHeight << std::endl;
                        
                        if (embeddedTexture->mHeight == 0) {
                            // Compressed format (PNG, JPG, etc)
                            mesh.embeddedTextureData.assign(
                                reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
                                reinterpret_cast<const unsigned char*>(embeddedTexture->pcData) + embeddedTexture->mWidth
                            );
                            mesh.embeddedTextureWidth = embeddedTexture->mWidth;
                            mesh.hasTexture = true;
                            mesh.texturePath = "embedded_compressed";
                            DEBUG_LOG << "Loaded compressed embedded texture (" << embeddedTexture->achFormatHint << "): " << mesh.embeddedTextureData.size() << " bytes" << std::endl;
                        } else {
                            // Raw RGBA format
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
                    } else {
                        DEBUG_LOG << "ERROR: Invalid texture index " << textureIndex << " (scene has " << scene->mNumTextures << " textures)" << std::endl;
                    }
                } else {
                    // External texture file
                    DEBUG_LOG << "=== EXTERNAL TEXTURE ===" << std::endl;
                    DEBUG_LOG << "Texture path from material: " << texPath.C_Str() << std::endl;
                    
                    QFileInfo modelFile(QString::fromStdString(filepath));
                    QDir modelDir = modelFile.absoluteDir();
                    QString texPathStr = QString::fromUtf8(texPath.C_Str());
                    
                    // Extract just the filename from the path
                    QFileInfo texFileInfo(texPathStr);
                    QString texFilename = texFileInfo.fileName();
                    
                    DEBUG_LOG << "Model directory: " << modelDir.absolutePath().toStdString() << std::endl;
                    DEBUG_LOG << "Texture filename: " << texFilename.toStdString() << std::endl;
                    
                    // Try multiple search locations
                    QStringList searchPaths;
                    searchPaths << modelDir.absoluteFilePath(texPathStr);           // Original path
                    searchPaths << modelDir.absoluteFilePath(texFilename);          // Same dir as model
                    searchPaths << modelDir.absoluteFilePath("textures/" + texFilename);  // textures subfolder
                    searchPaths << modelDir.absoluteFilePath("Textures/" + texFilename);  // Textures subfolder
                    searchPaths << modelDir.absoluteFilePath("../textures/" + texFilename); // parent/textures
                    searchPaths << modelDir.absoluteFilePath("../" + texFilename);  // parent dir
                    
                    bool found = false;
                    for (const QString& searchPath : searchPaths) {
                        DEBUG_LOG << "Trying: " << searchPath.toStdString() << std::endl;
                        if (QFileInfo::exists(searchPath)) {
                            mesh.hasTexture = true;
                            mesh.texturePath = searchPath.toStdString();
                            DEBUG_LOG << "SUCCESS: Found texture at " << searchPath.toStdString() << std::endl;
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found) {
                        DEBUG_LOG << "ERROR: External texture not found!" << std::endl;
                        
                        // FALLBACK: If scene has embedded textures, use the first one
                        if (scene->mNumTextures > 0) {
                            DEBUG_LOG << "FALLBACK: Using first embedded texture instead" << std::endl;
                            aiTexture* embeddedTexture = scene->mTextures[0];
                            
                            if (embeddedTexture->mHeight == 0) {
                                mesh.embeddedTextureData.assign(
                                    reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
                                    reinterpret_cast<const unsigned char*>(embeddedTexture->pcData) + embeddedTexture->mWidth
                                );
                                mesh.embeddedTextureWidth = embeddedTexture->mWidth;
                                mesh.hasTexture = true;
                                mesh.texturePath = "embedded_compressed";
                                DEBUG_LOG << "Using embedded texture 0: " << mesh.embeddedTextureData.size() << " bytes" << std::endl;
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
                                DEBUG_LOG << "Using embedded texture 0: " << mesh.embeddedTextureWidth << "x" << mesh.embeddedTextureHeight << std::endl;
                            }
                        } else {
                            DEBUG_LOG << "No fallback available - no embedded textures in scene" << std::endl;
                        }
                    }
                }
            } else {
                DEBUG_LOG << "No texture found in material (checked DIFFUSE and BASE_COLOR)" << std::endl;
                
                // FALLBACK: If no texture reference but scene has embedded textures, use first one
                if (scene->mNumTextures > 0) {
                    DEBUG_LOG << "FALLBACK: Material has no texture but scene has embedded textures, using first one" << std::endl;
                    aiTexture* embeddedTexture = scene->mTextures[0];
                    
                    if (embeddedTexture->mHeight == 0) {
                        mesh.embeddedTextureData.assign(
                            reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
                            reinterpret_cast<const unsigned char*>(embeddedTexture->pcData) + embeddedTexture->mWidth
                        );
                        mesh.embeddedTextureWidth = embeddedTexture->mWidth;
                        mesh.hasTexture = true;
                        mesh.texturePath = "embedded_compressed";
                        DEBUG_LOG << "Using embedded texture 0: " << mesh.embeddedTextureData.size() << " bytes" << std::endl;
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
                        DEBUG_LOG << "Using embedded texture 0: " << mesh.embeddedTextureWidth << "x" << mesh.embeddedTextureHeight << std::endl;
                    }
                }
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
