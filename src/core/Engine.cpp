#include "core/Engine.h"
#include "core/Logger.h"
#include "core/Renderer.h"
#include "core/SceneManager.h"
#include "core/SceneObject.h"
#include "core/Light.h"
#include "core/ResourceManager.h"
#include "core/InputSystem.h"
#include "core/AudioSystem.h"
#include "core/CommandHistory.h"
#include "core/AnimationManager.h"
#include "ui/UIManager.h"
#include "ui/AIMovieStudioUI.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <wrl/client.h>

using namespace Microsoft::WRL;

namespace Engine {
namespace Core {

Engine::Engine()
    : m_windowHandle(nullptr)
    , m_windowWidth(0)
    , m_windowHeight(0)
    , m_isInitialized(false)
    , m_isRunning(false)
{
    Logger::Instance().Info("Engine constructor called");
}

Engine::~Engine()
{
    Shutdown();
    Logger::Instance().Info("Engine destructor called");
}

bool Engine::Initialize(HWND windowHandle, uint32_t width, uint32_t height)
{
    Logger::Instance().Info("=== ENGINE INITIALIZATION START ===");
    Logger::Instance().Info("Initializing AI Movie Studio Engine...");

    m_windowHandle = windowHandle;
    m_windowWidth = width;
    m_windowHeight = height;
    Logger::Instance().Info("Window parameters set: " + std::to_string(width) + "x" + std::to_string(height));

    // Initialize DirectX 12
    Logger::Instance().Info("About to initialize DirectX 12...");
    if (!InitializeDirectX()) {
        Logger::Instance().Error("Failed to initialize DirectX 12");
        return false;
    }
    Logger::Instance().Info("DirectX 12 initialization completed");

    // Initialize subsystems
    Logger::Instance().Info("=== ABOUT TO CALL InitializeSubsystems() ===");
    Logger::Instance().Info("About to call InitializeSubsystems()");
    if (!InitializeSubsystems()) {
        Logger::Instance().Error("Failed to initialize subsystems");
        return false;
    }
    Logger::Instance().Info("=== Engine subsystems initialized successfully ===");
    Logger::Instance().Info("InitializeSubsystems() completed successfully");

    m_isInitialized = true;
    m_isRunning = true;
    Logger::Instance().Info("=== ENGINE INITIALIZATION COMPLETE ===");
    return true;
}

void Engine::Shutdown()
{
    Logger::Instance().Info("Engine shutdown started");
    
    if (m_isInitialized) {
        ShutdownSubsystems();
        m_isInitialized = false;
    }
    
    m_isRunning = false;
    Logger::Instance().Info("Engine shutdown complete");
}

void Engine::Update(float deltaTime)
{
    if (!m_isInitialized || !m_isRunning) {
        return;
    }

    // Update animation system FIRST (before scene objects)
    if (m_animationManager && m_sceneManager) {
        m_animationManager->Update(deltaTime, m_sceneManager.get());
    }

    // Update subsystems
    if (m_sceneManager) {
        m_sceneManager->Update(deltaTime);
    }
    
    if (m_uiManager) {
        m_uiManager->Update(deltaTime);
    }
    
    if (m_mainUI) {
        m_mainUI->Update(deltaTime);
    }
}

void Engine::Render()
{
    if (!m_isInitialized || !m_isRunning) {
        return;
    }

    // DirectX rendering disabled for now - it renders to the whole window and covers the UI
    // TODO: Create a child window for the 3D viewport and render DirectX there
    // if (m_renderer) {
    //     m_renderer->BeginFrame();
    //     // Add 3D rendering here
    //     m_renderer->EndFrame();
    // }
}

    void Engine::Render(HDC hdc)
    {
        std::cout << "DEBUG: Engine::Render(HDC) called" << std::endl;
        Logger::Instance().Info("Engine::Render(HDC) called");
        
        if (!m_isInitialized || !m_isRunning) {
            std::cout << "DEBUG: Engine not initialized or not running" << std::endl;
            Logger::Instance().Error("Engine not initialized or not running");
            return;
        }

        // Render UI
        if (m_mainUI) {
            std::cout << "DEBUG: About to call m_mainUI->Render(hdc)" << std::endl;
            Logger::Instance().Info("About to call m_mainUI->Render(hdc)");
            m_mainUI->Render(hdc);
        } else {
            std::cout << "DEBUG: m_mainUI is null!" << std::endl;
            Logger::Instance().Error("m_mainUI is null!");
        }
    }

void Engine::Run()
{
    if (!m_isInitialized) {
        Logger::Instance().Error("Engine not initialized, cannot run");
        return;
    }

    Logger::Instance().Info("Engine run loop started");
    
    while (m_isRunning) {
        // Update engine
        static auto lastTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        Update(deltaTime);
        
        // Small delay to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    Logger::Instance().Info("Engine run loop ended");
}

LRESULT Engine::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    (void)wParam; // Suppress unused parameter warning
    (void)lParam; // Suppress unused parameter warning
    
    // Debug: Log every message processed by engine
    static int engineMessageCount = 0;
    engineMessageCount++;
    if (engineMessageCount % 100 == 0) {
        std::cout << "DEBUG: Engine::ProcessMessage called " << engineMessageCount << " times, current message: 0x" << std::hex << message << std::dec << std::endl;
    }
    switch (message) {
        case WM_SIZE:
            {
                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                int width = clientRect.right - clientRect.left;
                int height = clientRect.bottom - clientRect.top;
                if (width > 0 && height > 0) {
                    m_windowWidth = width;
                    m_windowHeight = height;
                    if (m_renderer) {
                        m_renderer->Resize(width, height);
                    }
                    if (m_uiManager) {
                        m_uiManager->HandleWindowResize(width, height);
                    }
                }
            }
            return 0;
        
        // ============================================================================
        // MOUSE INPUT EVENTS - Forward to UI Manager
        // ============================================================================
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
            {
                std::cout << "========================================" << std::endl;
                std::cout << "MOUSE EVENT DETECTED: 0x" << std::hex << message << std::dec << std::endl;
                std::cout << "========================================" << std::endl;
                
                // Process through InputSystem first
                if (m_inputSystem) {
                    m_inputSystem->ProcessMessage(hwnd, message, wParam, lParam);
                }
                
                if (m_mainUI) {
                    // Forward to UI for handling
                    m_mainUI->HandleMessage(message, wParam, lParam);
                }
            }
            return 0;
        
        // ============================================================================
        // KEYBOARD INPUT EVENTS - Forward to UI Manager
        // ============================================================================
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            {
                std::cout << "========================================" << std::endl;
                std::cout << "KEYBOARD EVENT DETECTED: 0x" << std::hex << message << std::dec << " Key: " << wParam << std::endl;
                std::cout << "========================================" << std::endl;
                
                // Process through InputSystem first
                if (m_inputSystem) {
                    m_inputSystem->ProcessMessage(hwnd, message, wParam, lParam);
                }
                
                if (m_mainUI) {
                    // Forward to UI for handling
                    m_mainUI->HandleMessage(message, wParam, lParam);
                }
            }
            return 0;
            
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                // Render UI
                if (m_mainUI) {
                    m_mainUI->Render(hdc);
                }
                
                EndPaint(hwnd, &ps);
            }
            return 0;
            
        case WM_DESTROY:
            m_isRunning = false;
            PostQuitMessage(0);
            return 0;
            
        default:
            return 0; // Let DefWindowProc handle it
    }
}

bool Engine::InitializeDirectX()
{
    Logger::Instance().Info("Initializing DirectX 12...");
    
    // Create renderer
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->Initialize(m_windowHandle, m_windowWidth, m_windowHeight)) {
        Logger::Instance().Error("Failed to initialize renderer");
        return false;
    }
    
    Logger::Instance().Info("DirectX 12 initialized successfully");
    return true;
}

bool Engine::InitializeSubsystems()
{
    Logger::Instance().Info("=== InitializeSubsystems() METHOD CALLED ===");
    Logger::Instance().Info("InitializeSubsystems() method called - starting subsystem initialization");

    // Create SceneManager
    Logger::Instance().Info("Creating SceneManager...");
    m_sceneManager = std::make_unique<SceneManager>();
    Logger::Instance().Info("SceneManager created successfully");
    
    // Create default scene with initial objects
    Logger::Instance().Info("Creating default scene...");
    m_sceneManager->CreateScene("DefaultScene");
    
    // Add default scene objects
    auto cube = m_sceneManager->CreateObject("Cube", "Mesh");
    if (cube) {
        cube->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
        Logger::Instance().Info("Created default object: Cube");
    }
    
    auto sphere = m_sceneManager->CreateObject("Sphere", "Mesh");
    if (sphere) {
        sphere->SetPosition(DirectX::XMFLOAT3(2.0f, 0.0f, 0.0f));
        Logger::Instance().Info("Created default object: Sphere");
    }
    
    auto pyramid = m_sceneManager->CreateObject("Pyramid", "Mesh");
    if (pyramid) {
        pyramid->SetPosition(DirectX::XMFLOAT3(-2.0f, 0.0f, 0.0f));
        Logger::Instance().Info("Created default object: Pyramid");
    }
    
    // Add default Light object
    auto light = m_sceneManager->CreateLight("Light", "Directional");
    if (light) {
        light->SetPosition(DirectX::XMFLOAT3(2.0f, 2.0f, 2.0f));
        light->SetColor(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f));
        light->SetIntensity(1.0f);
        Logger::Instance().Info("Created default Light object");
    }
    
    Logger::Instance().Info("Default scene initialized with objects and light");

    // Create InputSystem
    Logger::Instance().Info("Creating InputSystem...");
    m_inputSystem = std::make_unique<InputSystem>();
    if (!m_inputSystem->Initialize(m_windowHandle)) {
        Logger::Instance().Error("Failed to initialize InputSystem");
        return false;
    }
    Logger::Instance().Info("InputSystem created successfully");

    // Create AnimationManager
    Logger::Instance().Info("Creating AnimationManager...");
    m_animationManager = std::make_unique<AnimationManager>();
    if (!m_animationManager->Initialize()) {
        Logger::Instance().Error("Failed to initialize AnimationManager");
        return false;
    }
    Logger::Instance().Info("AnimationManager created successfully");

    // Create CommandHistory
    Logger::Instance().Info("Creating CommandHistory...");
    m_commandHistory = std::make_unique<CommandHistory>();
    Logger::Instance().Info("CommandHistory created successfully");

    // Create UIManager
    Logger::Instance().Info("Creating UIManager...");
    m_uiManager = std::make_unique<UI::UIManager>();
    if (!m_uiManager->Initialize(m_windowHandle)) {
        Logger::Instance().Error("Failed to initialize UIManager");
        return false;
    }
    Logger::Instance().Info("UIManager created successfully");

    // Create AIMovieStudioUI
    Logger::Instance().Info("Creating AIMovieStudioUI...");
    m_mainUI = std::make_unique<UI::AIMovieStudioUI>();
    if (!m_mainUI->Initialize(m_uiManager.get())) {
        Logger::Instance().Error("Failed to initialize AIMovieStudioUI");
        return false;
    }
    Logger::Instance().Info("AIMovieStudioUI created successfully");

    Logger::Instance().Info("=== Engine subsystems initialized successfully ===");
    Logger::Instance().Info("InitializeSubsystems() completed successfully");
    return true;
}

void Engine::ShutdownSubsystems()
{
    Logger::Instance().Info("Shutting down subsystems...");
    
    if (m_mainUI) {
        m_mainUI->Shutdown();
        m_mainUI.reset();
    }
    
    if (m_uiManager) {
        m_uiManager->Shutdown();
        m_uiManager.reset();
    }
    
    if (m_sceneManager) {
        m_sceneManager.reset();
    }
    
    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }
    
    Logger::Instance().Info("Subsystems shutdown complete");
}

void Engine::CalculateFrameStats()
{
    // Placeholder for FPS calculation
}

} // namespace Core
} // namespace Engine