#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <xaudio2.h>
#include <wrl/client.h>

namespace Engine {
namespace Core {

/**
 * @brief Audio System for AI Movie Studio
 * 
 * Handles all audio operations including:
 * - Sound effects
 * - Background music
 * - Voice synthesis
 * - Audio processing
 */
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    // Initialization
    bool Initialize();
    void Shutdown();

    // Audio management
    bool LoadSound(const std::string& name, const std::string& filePath);
    bool PlaySound(const std::string& name, float volume = 1.0f, bool loop = false);
    bool StopSound(const std::string& name);
    bool StopAllSounds();

    // Volume control
    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_masterVolume; }

    void SetSoundVolume(const std::string& name, float volume);
    float GetSoundVolume(const std::string& name) const;

    // Audio state
    bool IsInitialized() const { return m_initialized; }
    bool IsSoundPlaying(const std::string& name) const;

private:
    bool m_initialized;
    float m_masterVolume;
    
    // XAudio2 components
    Microsoft::WRL::ComPtr<IXAudio2> m_xAudio2;
    IXAudio2MasteringVoice* m_masteringVoice;
    
    // Sound management
    std::unordered_map<std::string, std::vector<uint8_t>> m_soundData;
    std::unordered_map<std::string, IXAudio2SourceVoice*> m_playingSounds;
    std::unordered_map<std::string, float> m_soundVolumes;
};

} // namespace Core
} // namespace Engine
