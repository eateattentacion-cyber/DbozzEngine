#pragma once
#include <QString>
#include <vector>
#include <map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct aiAnimation;
struct aiNode;

namespace DabozzEngine::Renderer {

struct BoneInfo {
    int id;
    glm::mat4 offset;
};

struct KeyPosition {
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation {
    glm::quat rotation;
    float timeStamp;
};

struct KeyScale {
    glm::vec3 scale;
    float timeStamp;
};

class Bone {
public:
    Bone(const QString& name, int id, const std::vector<KeyPosition>& positions,
         const std::vector<KeyRotation>& rotations, const std::vector<KeyScale>& scales);
    
    void update(float animationTime);
    glm::mat4 getLocalTransform() const { return m_localTransform; }
    QString getName() const { return m_name; }
    int getBoneID() const { return m_id; }
    
private:
    int getPositionIndex(float animationTime);
    int getRotationIndex(float animationTime);
    int getScaleIndex(float animationTime);
    
    float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
    
    glm::mat4 interpolatePosition(float animationTime);
    glm::mat4 interpolateRotation(float animationTime);
    glm::mat4 interpolateScale(float animationTime);
    
    std::vector<KeyPosition> m_positions;
    std::vector<KeyRotation> m_rotations;
    std::vector<KeyScale> m_scales;
    
    glm::mat4 m_localTransform;
    QString m_name;
    int m_id;
};

struct AssimpNodeData {
    glm::mat4 transformation;
    QString name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

class Animation {
public:
    Animation() = default;
    Animation(const QString& animationPath, class Skeleton* skeleton);
    
    Bone* findBone(const QString& name);
    float getTicksPerSecond() const { return m_ticksPerSecond; }
    float getDuration() const { return m_duration; }
    const AssimpNodeData& getRootNode() const { return m_rootNode; }
    const std::map<QString, BoneInfo>& getBoneInfoMap() const { return m_boneInfoMap; }
    
    void updateBoneTransforms(float timeInSeconds, std::vector<glm::mat4>& outBoneMatrices);
    
private:
    void readMissingBones(const aiAnimation* animation, class Skeleton& skeleton);
    void readHierarchyData(AssimpNodeData& dest, const aiNode* src);
    void calculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform, 
                                float animationTime, std::vector<glm::mat4>& outBoneMatrices);
    
    float m_duration;
    int m_ticksPerSecond;
    std::vector<Bone> m_bones;
    AssimpNodeData m_rootNode;
    std::map<QString, BoneInfo> m_boneInfoMap;
};

}
