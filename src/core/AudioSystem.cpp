#include "core/AudioSystem.h"
#include "core/Logger.h"
#include <fstream>

namespace Engine {
namespace Core {

AudioSystem::AudioSystem()
    : m_initialized(false)
    , m_masterVolume(1.0f)
    , m_masteringVoice(nullptr)
{
    Logger::Instance().Debug("AudioSystem created");
}

AudioSystem::~AudioSystem()
{
    Shutdown();
    Logger::Instance().Debug("AudioSystem destroyed");
}

bool AudioSystem::Initialize()
{
    if (m_initialized) {
        Logger::Instance().Warning("AudioSystem already initialized");
        return true;
    }

    // Initialize XAudio2
    HRESULT hr = XAudio2Create(m_xAudio2.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        Logger::Instance().Error("Failed to create XAudio2 engine");
        return false;
    }

    // Create mastering voice
    hr = m_xAudio2->CreateMasteringVoice(&m_masteringVoice);
    if (FAILED(hr)) {
        Logger::Instance().Error("Failed to create mastering voice");
        return false;
    }

    m_initialized = true;
    Logger::Instance().Info("AudioSystem initialized successfully");
    return true;
}

void AudioSystem::Shutdown()
{
    if (!m_initialized) {
        return;
    }

    // Stop all playing sounds
    StopAllSounds();

    // Clean up XAudio2
    if (m_masteringVoice) {
        m_masteringVoice->DestroyVoice();
        m_masteringVoice = nullptr;
    }

    m_xAudio2.Reset();
    m_initialized = false;
    Logger::Instance().Info("AudioSystem shutdown complete");
}

bool AudioSystem::LoadSound(const std::string& name, const std::string& filePath)
{
    if (!m_initialized) {
        Logger::Instance().Error("AudioSystem not initialized");
        return false;
    }

    // Check if sound already exists
    if (m_soundData.find(name) != m_soundData.end()) {
        Logger::Instance().Warning("Sound already loaded: " + name);
        return true;
    }

    // Load sound file (simplified - in real implementation, you'd parse WAV files)
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Logger::Instance().Error("Failed to open sound file: " + filePath);
        return false;
    }

    // Read file data
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    m_soundData[name] = data;
    m_soundVolumes[name] = 1.0f;
    
    Logger::Instance().Info("Loaded sound: " + name);
    return true;
}

bool AudioSystem::PlaySound(const std::string& name, float volume, bool loop)
{
    (void)loop; // Suppress unused parameter warning
    
    if (!m_initialized) {
        Logger::Instance().Error("AudioSystem not initialized");
        return false;
    }

    auto it = m_soundData.find(name);
    if (it == m_soundData.end()) {
        Logger::Instance().Error("Sound not found: " + name);
        return false;
    }

    // Stop if already playing
    StopSound(name);

    // Set volume
    m_soundVolumes[name] = volume;

    // In a real implementation, you would:
    // 1. Parse the WAV file data
    // 2. Create a source voice
    // 3. Submit the audio data
    // 4. Start playback

    Logger::Instance().Info("Playing sound: " + name);
    return true;
}

bool AudioSystem::StopSound(const std::string& name)
{
    if (!m_initialized) {
        return false;
    }

    auto it = m_playingSounds.find(name);
    if (it != m_playingSounds.end()) {
        it->second->Stop();
        it->second->DestroyVoice();
        m_playingSounds.erase(it);
        Logger::Instance().Info("Stopped sound: " + name);
        return true;
    }

    return false;
}

bool AudioSystem::StopAllSounds()
{
    if (!m_initialized) {
        return false;
    }

    for (auto& sound : m_playingSounds) {
        if (sound.second) {
            sound.second->Stop();
            sound.second->DestroyVoice();
        }
    }
    m_playingSounds.clear();

    Logger::Instance().Info("Stopped all sounds");
    return true;
}

void AudioSystem::SetMasterVolume(float volume)
{
    if (!m_initialized) {
        return;
    }

    m_masterVolume = std::max(0.0f, std::min(1.0f, volume));
    
    if (m_masteringVoice) {
        m_masteringVoice->SetVolume(m_masterVolume);
    }
}

void AudioSystem::SetSoundVolume(const std::string& name, float volume)
{
    m_soundVolumes[name] = std::max(0.0f, std::min(1.0f, volume));
}

float AudioSystem::GetSoundVolume(const std::string& name) const
{
    auto it = m_soundVolumes.find(name);
    return (it != m_soundVolumes.end()) ? it->second : 0.0f;
}

bool AudioSystem::IsSoundPlaying(const std::string& name) const
{
    return m_playingSounds.find(name) != m_playingSounds.end();
}

} // namespace Core
} // namespace Engine
