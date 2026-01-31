#pragma once
#include <QMatrix4x4>
#include <QString>
#include <map>

namespace DabozzEngine::Renderer {

struct BoneInfo;

class Skeleton {
public:
    Skeleton() : m_boneCounter(0) {}
    
    // Load bone info from FBX file
    void loadFromFile(const std::string& filepath);
    
    std::map<QString, BoneInfo>& getBoneInfoMap() { return m_boneInfoMap; }
    int& getBoneCount() { return m_boneCounter; }
    
private:
    std::map<QString, BoneInfo> m_boneInfoMap;
    int m_boneCounter;
};

}
