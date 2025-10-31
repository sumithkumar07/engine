#include "core/AnimationManager.h"
#include "core/Logger.h"

namespace Engine {
namespace Core {

AnimationManager::AnimationManager()
    : m_initialized(false) {
    Logger::Instance().Debug("AnimationManager created");
}

AnimationManager::~AnimationManager() {
    Shutdown();
    Logger::Instance().Debug("AnimationManager destroyed");
}

bool AnimationManager::Initialize() {
    if (m_initialized) {
        Logger::Instance().Warning("AnimationManager already initialized");
        return true;
    }
    
    m_initialized = true;
    Logger::Instance().Info("AnimationManager initialized");
    return true;
}

void AnimationManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    StopAll();
    m_players.clear();
    m_clips.clear();
    
    m_initialized = false;
    Logger::Instance().Info("AnimationManager shutdown");
}

void AnimationManager::AddClip(std::shared_ptr<Animation::AnimationClip> clip) {
    if (!clip) {
        Logger::Instance().Error("AnimationManager::AddClip - Null clip");
        return;
    }
    
    m_clips[clip->GetName()] = clip;
    Logger::Instance().Info("Added animation clip: " + clip->GetName());
}

void AnimationManager::RemoveClip(const std::string& name) {
    // Stop player if running
    StopClip(name);
    
    // Remove clip
    m_clips.erase(name);
    Logger::Instance().Info("Removed animation clip: " + name);
}

std::shared_ptr<Animation::AnimationClip> AnimationManager::GetClip(const std::string& name) {
    auto it = m_clips.find(name);
    return (it != m_clips.end()) ? it->second : nullptr;
}

std::shared_ptr<Animation::AnimationClip> AnimationManager::CreateClip(const std::string& name) {
    auto clip = std::make_shared<Animation::AnimationClip>(name);
    AddClip(clip);
    return clip;
}

void AnimationManager::PlayClip(const std::string& clipName, bool looping) {
    auto clip = GetClip(clipName);
    if (!clip) {
        Logger::Instance().Error("AnimationManager::PlayClip - Clip not found: " + clipName);
        return;
    }
    
    // Get or create player
    auto& player = m_players[clipName];
    if (!player) {
        player = std::make_unique<Animation::AnimationPlayer>();
        player->SetClip(clip);
    }
    
    player->SetLooping(looping);
    player->Play();
    
    Logger::Instance().Info("Playing animation: " + clipName);
}

void AnimationManager::StopClip(const std::string& clipName) {
    auto it = m_players.find(clipName);
    if (it != m_players.end()) {
        it->second->Stop();
        m_players.erase(it); // Remove stopped player
        Logger::Instance().Info("Stopped animation: " + clipName);
    }
}

void AnimationManager::PauseClip(const std::string& clipName) {
    auto it = m_players.find(clipName);
    if (it != m_players.end()) {
        it->second->Pause();
        Logger::Instance().Info("Paused animation: " + clipName);
    }
}

void AnimationManager::StopAll() {
    for (auto& pair : m_players) {
        pair.second->Stop();
    }
    m_players.clear();
    Logger::Instance().Info("Stopped all animations");
}

Animation::AnimationPlayer* AnimationManager::GetPlayer(const std::string& clipName) {
    auto it = m_players.find(clipName);
    return (it != m_players.end()) ? it->second.get() : nullptr;
}

void AnimationManager::Update(float deltaTime, SceneManager* sceneManager) {
    if (!m_initialized || !sceneManager) {
        return;
    }
    
    // Update all active players
    std::vector<std::string> stoppedPlayers;
    
    for (auto& pair : m_players) {
        pair.second->Update(deltaTime, sceneManager);
        
        // Collect stopped players for removal
        if (pair.second->IsStopped()) {
            stoppedPlayers.push_back(pair.first);
        }
    }
    
    // Remove stopped players
    for (const auto& name : stoppedPlayers) {
        m_players.erase(name);
    }
}

bool AnimationManager::IsPlaying(const std::string& clipName) const {
    auto it = m_players.find(clipName);
    return (it != m_players.end()) && it->second->IsPlaying();
}

std::vector<std::string> AnimationManager::GetAllClipNames() const {
    std::vector<std::string> names;
    names.reserve(m_clips.size());
    
    for (const auto& pair : m_clips) {
        names.push_back(pair.first);
    }
    
    return names;
}

int AnimationManager::GetActivePlayerCount() const {
    return static_cast<int>(m_players.size());
}

} // namespace Core
} // namespace Engine

