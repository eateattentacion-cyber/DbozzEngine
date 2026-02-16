#pragma once

#include "ecs/world.h"
#include "ecs/components/audiosource.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <unordered_map>
#include <QString>

namespace DabozzEngine {
namespace Systems {

class AudioSystem {
public:
    AudioSystem(ECS::World* world);
    ~AudioSystem();

    void initialize();
    void shutdown();
    void update(float deltaTime);

    void playSound(ECS::EntityID entity);
    void stopSound(ECS::EntityID entity);
    void pauseSound(ECS::EntityID entity);
    
    void setListenerPosition(const QVector3D& position);
    void setListenerOrientation(const QVector3D& forward, const QVector3D& up);

    bool loadAudioFile(ECS::AudioSource* source);

private:
    struct WavData {
        std::vector<char> data;
        int channels;
        int sampleRate;
        int bitsPerSample;
    };

    bool loadWav(const QString& path, WavData& wav);
    ALenum getALFormat(int channels, int bitsPerSample);

    ECS::World* m_world;
    ALCdevice* m_device;
    ALCcontext* m_context;
    bool m_initialized;
};

} // namespace Systems
} // namespace DabozzEngine
