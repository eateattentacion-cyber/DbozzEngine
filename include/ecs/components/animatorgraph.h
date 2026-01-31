#pragma once

#include <QString>
#include <QPointF>
#include <vector>
#include <map>
#include <variant>

namespace DabozzEngine::ECS {

enum class AnimParamType { Bool, Float, Int, Trigger };

struct AnimParam {
    QString name;
    AnimParamType type;
    std::variant<bool, float, int> value;

    AnimParam() : type(AnimParamType::Bool), value(false) {}
    AnimParam(const QString& n, AnimParamType t)
        : name(n), type(t)
    {
        switch (t) {
            case AnimParamType::Bool:    value = false; break;
            case AnimParamType::Float:   value = 0.0f;  break;
            case AnimParamType::Int:     value = 0;     break;
            case AnimParamType::Trigger: value = false;  break;
        }
    }
};

enum class CompareOp { Greater, Less, Equal, NotEqual };

struct TransitionCondition {
    QString paramName;
    CompareOp op = CompareOp::Equal;
    std::variant<bool, float, int> threshold;

    TransitionCondition() : threshold(true) {}
};

struct AnimTransition {
    int id = 0;
    int sourceStateId = -1;
    int destStateId = -1;
    float blendDuration = 0.25f;
    bool hasExitTime = true;
    float exitTime = 0.9f;
    std::vector<TransitionCondition> conditions;
};

struct AnimState {
    int id = 0;
    QString name;
    QString clipName;
    float speed = 1.0f;
    bool loop = true;
    QPointF editorPosition;
};

class AnimatorGraph {
public:
    // Graph data
    std::vector<AnimState> states;
    std::vector<AnimTransition> transitions;
    std::map<QString, AnimParam> parameters;
    int entryStateId = -1;

    // Runtime state
    int activeStateId = -1;
    int previousStateId = -1;
    float transitionProgress = 0.0f;
    bool inTransition = false;
    float activeTransitionBlendDuration = 0.0f;
    float previousClipTime = 0.0f;

    // ID counters
    int nextStateId = 0;
    int nextTransitionId = 0;

    int addState(const QString& name, const QString& clipName, QPointF pos);
    int addTransition(int fromStateId, int toStateId);
    void removeState(int id);
    void removeTransition(int id);

    AnimState* findState(int id);
    AnimState* findStateByClip(const QString& clipName);
    std::vector<AnimTransition*> getTransitionsFrom(int stateId);

    void setEntryState(int stateId);
    void reset();
};

}
