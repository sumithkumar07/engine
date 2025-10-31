#include "animation/AnimationPlayer.h"
#include "core/SceneManager.h"
#include "core/SceneObject.h"
#include "core/Logger.h"

namespace Engine {
namespace Animation {

AnimationPlayer::AnimationPlayer()
    : m_state(PlaybackState::Stopped)
    , m_currentTime(0.0f)
    , m_speed(1.0f)
    , m_looping(false) {
}

AnimationPlayer::~AnimationPlayer() {
}

void AnimationPlayer::SetClip(std::shared_ptr<AnimationClip> clip) {
    bool wasPlaying = IsPlaying();
    Stop(); // Stop current animation
    
    m_clip = clip;
    
    // Resume if was playing
    if (wasPlaying && m_clip) {
        Play();
    }
}

void AnimationPlayer::Play() {
    if (!m_clip) {
        Core::Logger::Instance().Warning("AnimationPlayer::Play - No clip set");
        return;
    }
    
    m_state = PlaybackState::Playing;
    Core::Logger::Instance().Debug("AnimationPlayer: Playing clip '" + m_clip->GetName() + "'");
}

void AnimationPlayer::Pause() {
    if (m_state == PlaybackState::Playing) {
        m_state = PlaybackState::Paused;
        Core::Logger::Instance().Debug("AnimationPlayer: Paused");
    }
}

void AnimationPlayer::Stop() {
    m_state = PlaybackState::Stopped;
    m_currentTime = 0.0f;
    Core::Logger::Instance().Debug("AnimationPlayer: Stopped");
}

void AnimationPlayer::Restart() {
    Stop();
    Play();
}

void AnimationPlayer::SetTime(float time) {
    if (!m_clip) return;
    
    m_currentTime = time;
    
    // Clamp or loop
    if (m_looping) {
        float duration = m_clip->GetDuration();
        if (duration > 0.0f) {
            while (m_currentTime > duration) {
                m_currentTime -= duration;
            }
            while (m_currentTime < 0.0f) {
                m_currentTime += duration;
            }
        }
    } else {
        m_currentTime = std::max(0.0f, std::min(m_currentTime, m_clip->GetDuration()));
    }
}

float AnimationPlayer::GetNormalizedTime() const {
    if (!m_clip || m_clip->GetDuration() <= 0.0f) {
        return 0.0f;
    }
    return m_currentTime / m_clip->GetDuration();
}

void AnimationPlayer::Update(float deltaTime, Core::SceneManager* sceneManager) {
    if (!m_clip || m_state != PlaybackState::Playing || !sceneManager) {
        return;
    }
    
    // Advance time
    m_currentTime += deltaTime * m_speed;
    
    float duration = m_clip->GetDuration();
    
    // Handle end of animation
    if (m_currentTime >= duration) {
        if (m_looping) {
            m_currentTime = std::fmod(m_currentTime, duration);
        } else {
            m_currentTime = duration;
            Stop();
        }
    }
    
    // Apply animation to scene objects
    ApplyAnimation(sceneManager);
}

void AnimationPlayer::ApplyAnimation(Core::SceneManager* sceneManager) {
    if (!m_clip || !sceneManager) return;
    
    // Get all animated objects
    auto animatedObjects = m_clip->GetAnimatedObjects();
    
    for (const auto& objectName : animatedObjects) {
        // Get scene object
        auto sceneObject = sceneManager->FindObjectByName(objectName);
        if (!sceneObject) {
            continue; // Object doesn't exist in scene
        }
        
        // Evaluate and apply position
        DirectX::XMFLOAT3 position = m_clip->EvaluatePosition(objectName, m_currentTime);
        if (m_clip->GetCurve(objectName, AnimationProperty::PositionX) ||
            m_clip->GetCurve(objectName, AnimationProperty::PositionY) ||
            m_clip->GetCurve(objectName, AnimationProperty::PositionZ)) {
            sceneObject->SetPosition(position);
        }
        
        // Evaluate and apply rotation
        DirectX::XMFLOAT3 rotation = m_clip->EvaluateRotation(objectName, m_currentTime);
        if (m_clip->GetCurve(objectName, AnimationProperty::RotationX) ||
            m_clip->GetCurve(objectName, AnimationProperty::RotationY) ||
            m_clip->GetCurve(objectName, AnimationProperty::RotationZ)) {
            sceneObject->SetRotation(rotation);
        }
        
        // Evaluate and apply scale
        DirectX::XMFLOAT3 scale = m_clip->EvaluateScale(objectName, m_currentTime);
        if (m_clip->GetCurve(objectName, AnimationProperty::ScaleX) ||
            m_clip->GetCurve(objectName, AnimationProperty::ScaleY) ||
            m_clip->GetCurve(objectName, AnimationProperty::ScaleZ)) {
            sceneObject->SetScale(scale);
        }
    }
}

} // namespace Animation
} // namespace Engine

