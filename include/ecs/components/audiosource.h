#pragma once

#include "ecs/component.h"
#include <QString>
#include <AL/al.h>

namespace DabozzEngine {
namespace ECS {

struct AudioSource : public Component {
    QString filePath;
    float volume;
    float pitch;
    bool loop;
    bool playOnStart;
    bool spatial;

    // OpenAL handles
    ALuint sourceId;
    ALuint bufferId;
    bool isPlaying;
    bool isLoaded;

    AudioSource(const QString& path = QString(), float vol = 1.0f, float p = 1.0f, bool l = false)
        : filePath(path), volume(vol), pitch(p), loop(l),
          playOnStart(false), spatial(false),
          sourceId(0), bufferId(0), isPlaying(false), isLoaded(false) {}
};

} // namespace ECS
} // namespace DabozzEngine
