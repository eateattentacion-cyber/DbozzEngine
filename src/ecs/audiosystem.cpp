#include "ecs/systems/audiosystem.h"
#include <QFile>
#include <QDebug>
#include <cstring>

namespace DabozzEngine {
namespace Systems {

AudioSystem::AudioSystem(ECS::World* world)
    : m_world(world), m_device(nullptr), m_context(nullptr), m_initialized(false)
{
}

AudioSystem::~AudioSystem()
{
    shutdown();
}

void AudioSystem::initialize()
{
    if (m_initialized) return;

    m_device = alcOpenDevice(nullptr);
    if (!m_device) {
        qWarning() << "AudioSystem: Failed to open audio device";
        return;
    }

    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context) {
        qWarning() << "AudioSystem: Failed to create audio context";
        alcCloseDevice(m_device);
        m_device = nullptr;
        return;
    }

    alcMakeContextCurrent(m_context);
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    float orientation[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
    alListenerfv(AL_ORIENTATION, orientation);

    m_initialized = true;
    qDebug() << "AudioSystem: Initialized successfully";
}

void AudioSystem::shutdown()
{
    if (!m_initialized) return;

    for (ECS::EntityID entity : m_world->getEntities()) {
        ECS::AudioSource* audio = m_world->getComponent<ECS::AudioSource>(entity);
        if (!audio) continue;

        if (audio->sourceId) {
            alSourceStop(audio->sourceId);
            alDeleteSources(1, &audio->sourceId);
            audio->sourceId = 0;
        }
        if (audio->bufferId) {
            alDeleteBuffers(1, &audio->bufferId);
            audio->bufferId = 0;
        }
        audio->isPlaying = false;
        audio->isLoaded = false;
    }

    alcMakeContextCurrent(nullptr);
    if (m_context) {
        alcDestroyContext(m_context);
        m_context = nullptr;
    }
    if (m_device) {
        alcCloseDevice(m_device);
        m_device = nullptr;
    }

    m_initialized = false;
}

void AudioSystem::update(float deltaTime)
{
    if (!m_initialized) return;

    for (ECS::EntityID entity : m_world->getEntities()) {
        ECS::AudioSource* audio = m_world->getComponent<ECS::AudioSource>(entity);
        if (!audio) continue;

        if (!audio->isLoaded && !audio->filePath.isEmpty()) {
            loadAudioFile(audio);
        }

        if (!audio->isLoaded) continue;

        alSourcef(audio->sourceId, AL_GAIN, audio->volume);
        alSourcef(audio->sourceId, AL_PITCH, audio->pitch);
        alSourcei(audio->sourceId, AL_LOOPING, audio->loop ? AL_TRUE : AL_FALSE);

        if (audio->isPlaying) {
            ALint state;
            alGetSourcei(audio->sourceId, AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING && state != AL_PAUSED) {
                audio->isPlaying = false;
            }
        }

        if (audio->playOnStart && !audio->isPlaying) {
            alSourcePlay(audio->sourceId);
            audio->isPlaying = true;
            audio->playOnStart = false;
        }
    }
}

void AudioSystem::playSound(ECS::EntityID entity)
{
    if (!m_initialized) return;

    ECS::AudioSource* audio = m_world->getComponent<ECS::AudioSource>(entity);
    if (!audio || !audio->isLoaded) return;

    alSourcePlay(audio->sourceId);
    audio->isPlaying = true;
}

void AudioSystem::stopSound(ECS::EntityID entity)
{
    if (!m_initialized) return;

    ECS::AudioSource* audio = m_world->getComponent<ECS::AudioSource>(entity);
    if (!audio || !audio->isLoaded) return;

    alSourceStop(audio->sourceId);
    audio->isPlaying = false;
}

void AudioSystem::pauseSound(ECS::EntityID entity)
{
    if (!m_initialized) return;

    ECS::AudioSource* audio = m_world->getComponent<ECS::AudioSource>(entity);
    if (!audio || !audio->isLoaded) return;

    alSourcePause(audio->sourceId);
    audio->isPlaying = false;
}

bool AudioSystem::loadAudioFile(ECS::AudioSource* source)
{
    if (!m_initialized || !source) return false;

    if (source->sourceId) {
        alDeleteSources(1, &source->sourceId);
        source->sourceId = 0;
    }
    if (source->bufferId) {
        alDeleteBuffers(1, &source->bufferId);
        source->bufferId = 0;
    }

    WavData wav;
    if (!loadWav(source->filePath, wav)) {
        qWarning() << "AudioSystem: Failed to load" << source->filePath;
        return false;
    }

    ALenum format = getALFormat(wav.channels, wav.bitsPerSample);
    if (format == 0) {
        qWarning() << "AudioSystem: Unsupported format" << wav.channels << "ch" << wav.bitsPerSample << "bit";
        return false;
    }

    alGenBuffers(1, &source->bufferId);
    alBufferData(source->bufferId, format, wav.data.data(), (ALsizei)wav.data.size(), wav.sampleRate);

    alGenSources(1, &source->sourceId);
    alSourcei(source->sourceId, AL_BUFFER, source->bufferId);
    alSourcef(source->sourceId, AL_GAIN, source->volume);
    alSourcef(source->sourceId, AL_PITCH, source->pitch);
    alSourcei(source->sourceId, AL_LOOPING, source->loop ? AL_TRUE : AL_FALSE);

    source->isLoaded = true;
    qDebug() << "AudioSystem: Loaded" << source->filePath;
    return true;
}

bool AudioSystem::loadWav(const QString& path, WavData& wav)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray fileData = file.readAll();
    file.close();

    if (fileData.size() < 44) return false;

    const char* raw = fileData.constData();

    if (std::memcmp(raw, "RIFF", 4) != 0) return false;
    if (std::memcmp(raw + 8, "WAVE", 4) != 0) return false;

    int pos = 12;
    int fmtPos = -1;
    int dataPos = -1;
    int dataSize = 0;

    while (pos + 8 <= fileData.size()) {
        char chunkId[5] = {};
        std::memcpy(chunkId, raw + pos, 4);
        int chunkSize = *reinterpret_cast<const int32_t*>(raw + pos + 4);

        if (std::memcmp(chunkId, "fmt ", 4) == 0) {
            fmtPos = pos + 8;
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            dataPos = pos + 8;
            dataSize = chunkSize;
        }

        pos += 8 + chunkSize;
        if (pos % 2 != 0) pos++; 
    }

    if (fmtPos < 0 || dataPos < 0) return false;

    int16_t audioFormat = *reinterpret_cast<const int16_t*>(raw + fmtPos);
    if (audioFormat != 1) return false; 

    wav.channels = *reinterpret_cast<const int16_t*>(raw + fmtPos + 2);
    wav.sampleRate = *reinterpret_cast<const int32_t*>(raw + fmtPos + 4);
    wav.bitsPerSample = *reinterpret_cast<const int16_t*>(raw + fmtPos + 14);

    if (dataPos + dataSize > fileData.size()) {
        dataSize = fileData.size() - dataPos;
    }

    wav.data.resize(dataSize);
    std::memcpy(wav.data.data(), raw + dataPos, dataSize);

    return true;
}

ALenum AudioSystem::getALFormat(int channels, int bitsPerSample)
{
    if (channels == 1 && bitsPerSample == 8) return AL_FORMAT_MONO8;
    if (channels == 1 && bitsPerSample == 16) return AL_FORMAT_MONO16;
    if (channels == 2 && bitsPerSample == 8) return AL_FORMAT_STEREO8;
    if (channels == 2 && bitsPerSample == 16) return AL_FORMAT_STEREO16;
    return 0;
}

} // namespace Systems
} // namespace DabozzEngine
