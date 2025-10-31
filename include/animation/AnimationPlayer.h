#pragma once

#include "animation/AnimationClip.h"
#include <memory>
#include <string>

namespace Engine {

// Forward declarations
namespace Core {
    class SceneObject;
    class SceneManager;
}

namespace Animation {

/**
 * @brief Playback state
 */
enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

/**
 * @brief Animation player for a single clip
 * 
 * Handles playback, looping, speed control, and applies
 * animation values to SceneObjects.
 */
class AnimationPlayer {
public:
    AnimationPlayer();
    ~AnimationPlayer();

    // Clip management
    void SetClip(std::shared_ptr<AnimationClip> clip);
    std::shared_ptr<AnimationClip> GetClip() const { return m_clip; }
    
    // Playback control
    void Play();
    void Pause();
    void Stop();
    void Restart();
    
    PlaybackState GetState() const { return m_state; }
    bool IsPlaying() const { return m_state == PlaybackState::Playing; }
    bool IsPaused() const { return m_state == PlaybackState::Paused; }
    bool IsStopped() const { return m_state == PlaybackState::Stopped; }
    
    // Time control
    void SetTime(float time);
    float GetTime() const { return m_currentTime; }
    float GetNormalizedTime() const; // 0 to 1
    
    void SetSpeed(float speed) { m_speed = speed; }
    float GetSpeed() const { return m_speed; }
    
    // Update (call every frame)
    void Update(float deltaTime, Core::SceneManager* sceneManager);
    
    // Looping
    void SetLooping(bool looping) { m_looping = looping; }
    bool IsLooping() const { return m_looping; }

private:
    std::shared_ptr<AnimationClip> m_clip;
    PlaybackState m_state;
    float m_currentTime;
    float m_speed;
    bool m_looping;
    
    // Apply animation to scene
    void ApplyAnimation(Core::SceneManager* sceneManager);
};

} // namespace Animation
} // namespace Engine

