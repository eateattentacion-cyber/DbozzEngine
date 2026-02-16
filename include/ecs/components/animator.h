#pragma once

#include "ecs/component.h"
#include "renderer/animation.h"
#include "renderer/skeleton.h"
#include "ecs/components/animatorgraph.h"
#include <memory>
#include <vector>
#include <map>
#include <QString>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

namespace DabozzEngine {
namespace ECS {

struct Animator : public Component {
    std::shared_ptr<Renderer::Animation> currentAnimation;
    QString currentClipName;
    std::map<QString, std::shared_ptr<Renderer::Animation>> animations;
    std::shared_ptr<Renderer::Skeleton> skeleton;
    std::shared_ptr<AnimatorGraph> graph;

    float currentTime;
    bool isPlaying;
    bool loop;
    float playbackSpeed;

    // Bone transforms for rendering
    std::vector<glm::mat4> boneMatrices;

    Animator()
        : currentTime(0.0f)
        , isPlaying(false)
        , loop(true)
        , playbackSpeed(1.0f)
    {
        // Initialize with identity matrices (max 100 bones)
        boneMatrices.resize(100, glm::mat4(1.0f));
    }

    void addAnimation(const QString& name, std::shared_ptr<Renderer::Animation> anim) {
        animations[name] = anim;
        // If this is the first animation, set it as current
        if (!currentAnimation) {
            currentAnimation = anim;
            currentClipName = name;
        }
    }

    void playAnimation(const QString& name) {
        auto it = animations.find(name);
        if (it != animations.end()) {
            currentAnimation = it->second;
            currentClipName = name;
            currentTime = 0.0f;
            isPlaying = true;
        }
    }

    void play() {
        isPlaying = true;
    }

    void pause() {
        isPlaying = false;
    }

    void stop() {
        isPlaying = false;
        currentTime = 0.0f;
    }

    void setAnimation(std::shared_ptr<Renderer::Animation> anim) {
        currentAnimation = anim;
        currentTime = 0.0f;
        if (animations.empty()) {
            animations["default"] = anim;
            currentClipName = "default";
        }
    }

    void update(float deltaTime) {
        if (!isPlaying || !currentAnimation) {
            return;
        }

        currentTime += deltaTime * playbackSpeed;

        // Handle looping
        if (currentTime >= currentAnimation->getDuration()) {
            if (loop) {
                currentTime = fmod(currentTime, currentAnimation->getDuration());
            } else {
                currentTime = currentAnimation->getDuration();
                isPlaying = false;
            }
        }

        // Update bone transforms
        currentAnimation->updateBoneTransforms(currentTime, boneMatrices);
    }
};

}
}
