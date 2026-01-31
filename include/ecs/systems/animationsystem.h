#pragma once

#include "ecs/world.h"
#include "ecs/components/animator.h"
#include "ecs/components/animatorgraph.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace DabozzEngine {
namespace Systems {

class AnimationSystem {
public:
    AnimationSystem(ECS::World* world) : m_world(world) {}

    void update(float deltaTime) {
        if (!m_world) return;

        for (ECS::EntityID entity : m_world->getEntities()) {
            ECS::Animator* animator = m_world->getComponent<ECS::Animator>(entity);
            if (!animator) continue;

            if (animator->graph && !animator->graph->states.empty()) {
                updateGraph(animator, deltaTime);
            } else {
                animator->update(deltaTime);
            }
        }
    }

private:
    void updateGraph(ECS::Animator* animator, float deltaTime) {
        auto* graph = animator->graph.get();
        if (graph->activeStateId == -1) return;

        auto* activeState = graph->findState(graph->activeStateId);
        if (!activeState) return;

        auto clipIt = animator->animations.find(activeState->clipName);
        if (clipIt == animator->animations.end()) return;
        auto& activeClip = clipIt->second;

        // Advance current time
        animator->currentTime += deltaTime * activeState->speed;

        float duration = activeClip->getDuration() / activeClip->getTicksPerSecond();
        if (duration <= 0.0f) duration = 1.0f;

        float normalizedTime = animator->currentTime / duration;

        // Handle looping
        if (animator->currentTime >= duration) {
            if (activeState->loop) {
                animator->currentTime = fmod(animator->currentTime, duration);
                normalizedTime = animator->currentTime / duration;
            } else {
                animator->currentTime = duration;
                normalizedTime = 1.0f;
            }
        }

        // Evaluate transitions (only when not already transitioning)
        if (!graph->inTransition) {
            auto outgoing = graph->getTransitionsFrom(graph->activeStateId);
            for (auto* transition : outgoing) {
                bool shouldFire = true;

                // Check exit time
                if (transition->hasExitTime && normalizedTime < transition->exitTime) {
                    shouldFire = false;
                }

                // Check parameter conditions
                if (shouldFire) {
                    for (auto& cond : transition->conditions) {
                        if (!evaluateCondition(graph, cond)) {
                            shouldFire = false;
                            break;
                        }
                    }
                }

                if (shouldFire) {
                    // Start transition
                    graph->previousStateId = graph->activeStateId;
                    graph->previousClipTime = animator->currentTime;
                    graph->activeStateId = transition->destStateId;
                    graph->inTransition = true;
                    graph->transitionProgress = 0.0f;
                    graph->activeTransitionBlendDuration = transition->blendDuration;
                    animator->currentTime = 0.0f;
                    break;
                }
            }
        }

        // Reset triggers after evaluation
        for (auto& [name, param] : graph->parameters) {
            if (param.type == ECS::AnimParamType::Trigger) {
                param.value = false;
            }
        }

        // Sample animations and blend
        if (graph->inTransition) {
            graph->transitionProgress += deltaTime / std::max(graph->activeTransitionBlendDuration, 0.001f);

            if (graph->transitionProgress >= 1.0f) {
                graph->transitionProgress = 1.0f;
                graph->inTransition = false;
                graph->previousStateId = -1;
            }

            // Sample previous clip
            auto* prevState = graph->findState(graph->previousStateId);
            std::vector<glm::mat4> prevMatrices(100, glm::mat4(1.0f));
            if (prevState) {
                auto prevClipIt = animator->animations.find(prevState->clipName);
                if (prevClipIt != animator->animations.end()) {
                    graph->previousClipTime += deltaTime * prevState->speed;
                    prevClipIt->second->updateBoneTransforms(graph->previousClipTime, prevMatrices);
                }
            }

            // Sample active clip
            auto* curState = graph->findState(graph->activeStateId);
            std::vector<glm::mat4> curMatrices(100, glm::mat4(1.0f));
            if (curState) {
                auto curClipIt = animator->animations.find(curState->clipName);
                if (curClipIt != animator->animations.end()) {
                    curClipIt->second->updateBoneTransforms(animator->currentTime, curMatrices);
                }
            }

            // Blend
            float t = graph->transitionProgress;
            for (size_t i = 0; i < animator->boneMatrices.size(); i++) {
                blendMatrices(prevMatrices[i], curMatrices[i], t, animator->boneMatrices[i]);
            }
        } else {
            // No transition, just sample the active clip
            activeClip->updateBoneTransforms(animator->currentTime, animator->boneMatrices);
        }
    }

    bool evaluateCondition(ECS::AnimatorGraph* graph, const ECS::TransitionCondition& cond) {
        auto it = graph->parameters.find(cond.paramName);
        if (it == graph->parameters.end()) return false;

        auto& param = it->second;

        if (param.type == ECS::AnimParamType::Trigger) {
            return std::get<bool>(param.value);
        }

        if (param.type == ECS::AnimParamType::Bool) {
            bool paramVal = std::get<bool>(param.value);
            bool threshVal = std::get<bool>(cond.threshold);
            switch (cond.op) {
                case ECS::CompareOp::Equal:    return paramVal == threshVal;
                case ECS::CompareOp::NotEqual: return paramVal != threshVal;
                default: return false;
            }
        }

        if (param.type == ECS::AnimParamType::Float) {
            float paramVal = std::get<float>(param.value);
            float threshVal = std::get<float>(cond.threshold);
            switch (cond.op) {
                case ECS::CompareOp::Greater:  return paramVal > threshVal;
                case ECS::CompareOp::Less:     return paramVal < threshVal;
                case ECS::CompareOp::Equal:    return paramVal == threshVal;
                case ECS::CompareOp::NotEqual: return paramVal != threshVal;
            }
        }

        if (param.type == ECS::AnimParamType::Int) {
            int paramVal = std::get<int>(param.value);
            int threshVal = std::get<int>(cond.threshold);
            switch (cond.op) {
                case ECS::CompareOp::Greater:  return paramVal > threshVal;
                case ECS::CompareOp::Less:     return paramVal < threshVal;
                case ECS::CompareOp::Equal:    return paramVal == threshVal;
                case ECS::CompareOp::NotEqual: return paramVal != threshVal;
            }
        }

        return false;
    }

    void blendMatrices(const glm::mat4& a, const glm::mat4& b, float t, glm::mat4& out) {
        // Decompose both matrices
        glm::vec3 scaleA, scaleB, transA, transB, skewA, skewB;
        glm::quat rotA, rotB;
        glm::vec4 perspA, perspB;

        glm::decompose(a, scaleA, rotA, transA, skewA, perspA);
        glm::decompose(b, scaleB, rotB, transB, skewB, perspB);

        // Interpolate
        glm::vec3 finalTrans = glm::mix(transA, transB, t);
        glm::quat finalRot = glm::slerp(rotA, rotB, t);
        glm::vec3 finalScale = glm::mix(scaleA, scaleB, t);

        // Recompose
        out = glm::translate(glm::mat4(1.0f), finalTrans)
            * glm::mat4_cast(finalRot)
            * glm::scale(glm::mat4(1.0f), finalScale);
    }

    ECS::World* m_world;
};

}
}
