#include "ecs/components/animatorgraph.h"
#include <algorithm>

namespace DabozzEngine::ECS {

int AnimatorGraph::addState(const QString& name, const QString& clipName, QPointF pos)
{
    AnimState state;
    state.id = nextStateId++;
    state.name = name;
    state.clipName = clipName;
    state.editorPosition = pos;
    states.push_back(state);

    // First state becomes entry
    if (entryStateId == -1) {
        entryStateId = state.id;
        activeStateId = state.id;
    }

    return state.id;
}

int AnimatorGraph::addTransition(int fromStateId, int toStateId)
{
    AnimTransition t;
    t.id = nextTransitionId++;
    t.sourceStateId = fromStateId;
    t.destStateId = toStateId;
    transitions.push_back(t);
    return t.id;
}

void AnimatorGraph::removeState(int id)
{
    transitions.erase(
        std::remove_if(transitions.begin(), transitions.end(),
            [id](const AnimTransition& t) {
                return t.sourceStateId == id || t.destStateId == id;
            }),
        transitions.end()
    );

    states.erase(
        std::remove_if(states.begin(), states.end(),
            [id](const AnimState& s) { return s.id == id; }),
        states.end()
    );

    if (entryStateId == id) {
        entryStateId = states.empty() ? -1 : states[0].id;
    }
    if (activeStateId == id) {
        activeStateId = entryStateId;
    }
}

void AnimatorGraph::removeTransition(int id)
{
    transitions.erase(
        std::remove_if(transitions.begin(), transitions.end(),
            [id](const AnimTransition& t) { return t.id == id; }),
        transitions.end()
    );
}

AnimState* AnimatorGraph::findState(int id)
{
    for (auto& s : states) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

AnimState* AnimatorGraph::findStateByClip(const QString& clipName)
{
    for (auto& s : states) {
        if (s.clipName == clipName) return &s;
    }
    return nullptr;
}

std::vector<AnimTransition*> AnimatorGraph::getTransitionsFrom(int stateId)
{
    std::vector<AnimTransition*> result;
    for (auto& t : transitions) {
        if (t.sourceStateId == stateId) {
            result.push_back(&t);
        }
    }
    return result;
}

void AnimatorGraph::setEntryState(int stateId)
{
    entryStateId = stateId;
}

void AnimatorGraph::reset()
{
    activeStateId = entryStateId;
    previousStateId = -1;
    transitionProgress = 0.0f;
    inTransition = false;
    previousClipTime = 0.0f;
    for (auto& [name, param] : parameters) {
        if (param.type == AnimParamType::Trigger) {
            param.value = false;
        }
    }
}

}
