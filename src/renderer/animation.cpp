#include "renderer/animation.h"
#include "renderer/skeleton.h"
#include "debug/logger.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/config.h>

namespace DabozzEngine::Renderer {

// Helper to convert assimp matrix to GLM
static glm::mat4 convertMatrixToGLM(const aiMatrix4x4& from)
{
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

// Animation implementation
Animation::Animation(const QString& animationPath, Skeleton* skeleton)
{
    DEBUG_LOG << "Animation constructor called with path: " << animationPath.toStdString() << std::endl;
    
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    const aiScene* scene = importer.ReadFile(animationPath.toStdString(), aiProcess_Triangulate);
    
    if (!scene || !scene->mRootNode) {
        DEBUG_LOG << "ERROR: Failed to load scene or no root node" << std::endl;
        return;
    }
    
    DEBUG_LOG << "Scene loaded, checking for animations..." << std::endl;
    DEBUG_LOG << "Number of animations in scene: " << scene->mNumAnimations << std::endl;
    
    if (!scene->mAnimations || scene->mAnimations[0] == nullptr) {
        DEBUG_LOG << "ERROR: No animations found in file" << std::endl;
        return;
    }
    
    auto animation = scene->mAnimations[0];
    m_duration = animation->mDuration;
    m_ticksPerSecond = animation->mTicksPerSecond;
    
    DEBUG_LOG << "Animation found: duration=" << m_duration << " ticks=" << m_ticksPerSecond << std::endl;
    DEBUG_LOG << "Animation has " << animation->mNumChannels << " channels" << std::endl;
    
    readHierarchyData(m_rootNode, scene->mRootNode);
    readMissingBones(animation, *skeleton);
    
    DEBUG_LOG << "Animation constructor complete, loaded " << m_bones.size() << " bones" << std::endl;
}

Bone* Animation::findBone(const QString& name)
{
    auto iter = std::find_if(m_bones.begin(), m_bones.end(),
        [&](const Bone& bone) { return bone.getName() == name; });
    
    if (iter == m_bones.end())
        return nullptr;
    return &(*iter);
}

void Animation::readMissingBones(const aiAnimation* animation, Skeleton& skeleton)
{
    int size = animation->mNumChannels;
    auto& boneInfoMap = skeleton.getBoneInfoMap();
    int& boneCount = skeleton.getBoneCount();
    
    DEBUG_LOG << "readMissingBones: Processing " << size << " channels" << std::endl;
    DEBUG_LOG << "Skeleton already has " << boneInfoMap.size() << " bones" << std::endl;
    
    // Track which bones have animation channels
    std::set<QString> animatedBones;
    
    for (int i = 0; i < size; i++) {
        auto channel = animation->mChannels[i];
        QString boneName = QString::fromStdString(channel->mNodeName.data);
        
        animatedBones.insert(boneName);
        
        // Only add if not already in map (offset matrices should already be there from mesh loading)
        if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
            DEBUG_LOG << "WARNING: Bone " << boneName.toStdString() << " not found in skeleton, adding with identity offset" << std::endl;
            boneInfoMap[boneName].id = boneCount;
            boneInfoMap[boneName].offset = glm::mat4(1.0f);
            boneCount++;
        }
        
        std::vector<KeyPosition> positions;
        std::vector<KeyRotation> rotations;
        std::vector<KeyScale> scales;
        
        for (unsigned int positionIndex = 0; positionIndex < channel->mNumPositionKeys; ++positionIndex) {
            aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
            float timeStamp = channel->mPositionKeys[positionIndex].mTime;
            KeyPosition data;
            data.position = glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z);
            data.timeStamp = timeStamp;
            positions.push_back(data);
        }
        
        for (unsigned int rotationIndex = 0; rotationIndex < channel->mNumRotationKeys; ++rotationIndex) {
            aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
            float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
            KeyRotation data;
            data.rotation = glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z);
            data.timeStamp = timeStamp;
            rotations.push_back(data);
        }
        
        for (unsigned int keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex) {
            aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
            float timeStamp = channel->mScalingKeys[keyIndex].mTime;
            KeyScale data;
            data.scale = glm::vec3(scale.x, scale.y, scale.z);
            data.timeStamp = timeStamp;
            scales.push_back(data);
        }
        
        m_bones.push_back(Bone(boneName, boneInfoMap[boneName].id, positions, rotations, scales));
    }
    
    // Log bones that don't have animation channels
    DEBUG_LOG << "Bones WITHOUT animation channels:" << std::endl;
    for (auto& pair : boneInfoMap) {
        if (animatedBones.find(pair.first) == animatedBones.end()) {
            DEBUG_LOG << "  " << pair.first.toStdString() << " (ID: " << pair.second.id << ")" << std::endl;
        }
    }
    
    m_boneInfoMap = boneInfoMap;
    DEBUG_LOG << "Final bone info map size: " << m_boneInfoMap.size() << std::endl;
}

void Animation::readHierarchyData(AssimpNodeData& dest, const aiNode* src)
{
    dest.name = QString::fromStdString(src->mName.data);
    dest.transformation = convertMatrixToGLM(src->mTransformation);
    dest.childrenCount = src->mNumChildren;
    
    for (unsigned int i = 0; i < src->mNumChildren; i++) {
        AssimpNodeData newData;
        readHierarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}

// Bone implementation
Bone::Bone(const QString& name, int id, const std::vector<KeyPosition>& positions,
           const std::vector<KeyRotation>& rotations, const std::vector<KeyScale>& scales)
    : m_name(name)
    , m_id(id)
    , m_positions(positions)
    , m_rotations(rotations)
    , m_scales(scales)
{
    m_localTransform = glm::mat4(1.0f);
}

void Bone::update(float animationTime)
{
    glm::mat4 translation = interpolatePosition(animationTime);
    glm::mat4 rotation = interpolateRotation(animationTime);
    glm::mat4 scale = interpolateScale(animationTime);
    m_localTransform = translation * rotation * scale;
}

int Bone::getPositionIndex(float animationTime)
{
    for (size_t index = 0; index < m_positions.size() - 1; ++index) {
        if (animationTime < m_positions[index + 1].timeStamp)
            return index;
    }
    return 0;
}

int Bone::getRotationIndex(float animationTime)
{
    for (size_t index = 0; index < m_rotations.size() - 1; ++index) {
        if (animationTime < m_rotations[index + 1].timeStamp)
            return index;
    }
    return 0;
}

int Bone::getScaleIndex(float animationTime)
{
    for (size_t index = 0; index < m_scales.size() - 1; ++index) {
        if (animationTime < m_scales[index + 1].timeStamp)
            return index;
    }
    return 0;
}

float Bone::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
    float scaleFactor = 0.0f;
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}

glm::mat4 Bone::interpolatePosition(float animationTime)
{
    if (m_positions.size() == 1) {
        return glm::translate(glm::mat4(1.0f), m_positions[0].position);
    }
    
    int p0Index = getPositionIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(m_positions[p0Index].timeStamp,
                                       m_positions[p1Index].timeStamp, animationTime);
    glm::vec3 finalPosition = glm::mix(m_positions[p0Index].position, m_positions[p1Index].position, scaleFactor);
    return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 Bone::interpolateRotation(float animationTime)
{
    if (m_rotations.size() == 1) {
        return glm::mat4_cast(glm::normalize(m_rotations[0].rotation));
    }
    
    int p0Index = getRotationIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(m_rotations[p0Index].timeStamp,
                                       m_rotations[p1Index].timeStamp, animationTime);
    glm::quat finalRotation = glm::slerp(m_rotations[p0Index].rotation, m_rotations[p1Index].rotation, scaleFactor);
    return glm::mat4_cast(glm::normalize(finalRotation));
}

glm::mat4 Bone::interpolateScale(float animationTime)
{
    if (m_scales.size() == 1) {
        return glm::scale(glm::mat4(1.0f), m_scales[0].scale);
    }
    
    int p0Index = getScaleIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(m_scales[p0Index].timeStamp,
                                       m_scales[p1Index].timeStamp, animationTime);
    glm::vec3 finalScale = glm::mix(m_scales[p0Index].scale, m_scales[p1Index].scale, scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}

void Animation::updateBoneTransforms(float timeInSeconds, std::vector<glm::mat4>& outBoneMatrices)
{
    float timeInTicks = timeInSeconds * m_ticksPerSecond;
    float animationTime = fmod(timeInTicks, m_duration);
    
    // Initialize all bone matrices to identity
    for (auto& mat : outBoneMatrices) {
        mat = glm::mat4(1.0f);
    }
    
    // Update all bones
    for (auto& bone : m_bones) {
        bone.update(animationTime);
    }
    
    // Calculate final transforms with global inverse root transform
    glm::mat4 globalInverseTransform = glm::inverse(m_rootNode.transformation);
    
    static bool logged = false;
    if (!logged) {
        DEBUG_LOG << "=== ANIMATION TRANSFORM DEBUG ===" << std::endl;
        DEBUG_LOG << "Root node name: " << m_rootNode.name.toStdString() << std::endl;
        DEBUG_LOG << "Root transformation matrix:" << std::endl;
        for (int i = 0; i < 4; i++) {
            DEBUG_LOG << "  [" << m_rootNode.transformation[i][0] << ", " 
                      << m_rootNode.transformation[i][1] << ", "
                      << m_rootNode.transformation[i][2] << ", "
                      << m_rootNode.transformation[i][3] << "]" << std::endl;
        }
        logged = true;
    }
    
    calculateBoneTransform(&m_rootNode, globalInverseTransform, animationTime, outBoneMatrices);
}

void Animation::calculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform,
                                       float animationTime, std::vector<glm::mat4>& outBoneMatrices)
{
    QString nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;
    
    // Check if this bone has animation data
    Bone* bone = findBone(nodeName);
    if (bone) {
        // Use animated transform
        nodeTransform = bone->getLocalTransform();
    }
    // else: use the static node transformation from the hierarchy
    
    glm::mat4 globalTransformation = parentTransform * nodeTransform;
    
    // If this node is a bone, write its final transform
    if (m_boneInfoMap.find(nodeName) != m_boneInfoMap.end()) {
        int index = m_boneInfoMap[nodeName].id;
        glm::mat4 offset = m_boneInfoMap[nodeName].offset;
        if (index < (int)outBoneMatrices.size()) {
            // Final bone matrix = globalTransform * offsetMatrix
            outBoneMatrices[index] = globalTransformation * offset;
        }
    }
    
    // Recurse to children
    for (int i = 0; i < node->childrenCount; i++) {
        (&node->children[i], globalTransformation, animationTime, outBoneMatrices);
    }
}

}
