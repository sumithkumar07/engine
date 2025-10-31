#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <string>

using namespace Microsoft::WRL;

// Forward declarations
namespace Engine {
namespace Core {
class Scene;
class SceneManager;
class AudioSystem;
class CommandHistory;

// Forward declaration for Renderer (it's in Engine::Rendering namespace)
class Renderer; // This will be Engine::Rendering::Renderer
class LightManager;
class MaterialManager;
class MeshManager;
class AnimationManager;
class InputSystem;
class ResourceManager;
class FrustumCullingManager;
class MemoryPoolManager;
}

namespace UI {
    class UIManager;
    class AIMovieStudioUI;
}

namespace Core {

/**
 * @brief Main engine class that manages all engine subsystems
 * 
 * This class is the central hub for the AI Movie Studio engine.
 * It initializes and manages all core systems including rendering,
 * scene management, audio, and AI communication.
 */
class Engine {
public:
    Engine();
    ~Engine();

    // Core lifecycle methods
    bool Initialize(HWND windowHandle, uint32_t width, uint32_t height);
    void Shutdown();
    void Update(float deltaTime);
    void Render();
    void Render(HDC hdc);
    void Run();

    // Message handling
    LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Getters
    HWND GetWindowHandle() const { return m_windowHandle; }
    bool IsRunning() const { return m_isRunning; }
    uint32_t GetWindowWidth() const { return m_windowWidth; }
    uint32_t GetWindowHeight() const { return m_windowHeight; }

    // Subsystem access
    SceneManager* GetSceneManager() const { return m_sceneManager.get(); }
    Renderer* GetRenderer() const { return m_renderer.get(); }
    AudioSystem* GetAudioSystem() const { return m_audioSystem.get(); }
    InputSystem* GetInputSystem() const { return m_inputSystem.get(); }
    ResourceManager* GetResourceManager() const { return m_resourceManager.get(); }
    AnimationManager* GetAnimationManager() const { return m_animationManager.get(); }
    CommandHistory* GetCommandHistory() const { return m_commandHistory.get(); }

private:
    // Window properties
    HWND m_windowHandle;
    uint32_t m_windowWidth;
    uint32_t m_windowHeight;
    bool m_isInitialized;
    bool m_isRunning;

    // Core subsystems
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<AudioSystem> m_audioSystem;
    std::unique_ptr<InputSystem> m_inputSystem;
    std::unique_ptr<ResourceManager> m_resourceManager;
    std::unique_ptr<AnimationManager> m_animationManager;
    std::unique_ptr<CommandHistory> m_commandHistory;

    // UI system
    std::unique_ptr<UI::UIManager> m_uiManager;
    std::unique_ptr<UI::AIMovieStudioUI> m_mainUI;

    // Private methods
    bool InitializeDirectX();
    bool InitializeSubsystems();
    void ShutdownSubsystems();
    void CalculateFrameStats();
};

} // namespace Core
} // namespace Engine