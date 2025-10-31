#pragma once

#include "animation/AnimationPlayer.h"
#include "animation/AnimationClip.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace Engine {
namespace Core {

// Forward declaration
class SceneManager;

/**
 * @brief Animation manager for the entire scene
 * 
 * Manages animation clips and players.
 * Handles playback, blending, and updates all active animations.
 */
class AnimationManager {
public:
    AnimationManager();
    ~AnimationManager();

    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Clip management
    void AddClip(std::shared_ptr<Animation::AnimationClip> clip);
    void RemoveClip(const std::string& name);
    std::shared_ptr<Animation::AnimationClip> GetClip(const std::string& name);
    std::shared_ptr<Animation::AnimationClip> CreateClip(const std::string& name);
    
    // Playback control
    void PlayClip(const std::string& clipName, bool looping = false);
    void StopClip(const std::string& clipName);
    void PauseClip(const std::string& clipName);
    void StopAll();
    
    // Player access
    Animation::AnimationPlayer* GetPlayer(const std::string& clipName);
    
    // Update (call every frame)
    void Update(float deltaTime, SceneManager* sceneManager);
    
    // Query
    bool IsPlaying(const std::string& clipName) const;
    std::vector<std::string> GetAllClipNames() const;
    int GetActivePlayerCount() const;

private:
    std::unordered_map<std::string, std::shared_ptr<Animation::AnimationClip>> m_clips;
    std::unordered_map<std::string, std::unique_ptr<Animation::AnimationPlayer>> m_players;
    
    bool m_initialized;
};

} // namespace Core
} // namespace Engine

