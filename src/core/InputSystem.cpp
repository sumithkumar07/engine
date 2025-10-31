#include "core/InputSystem.h"
#include "core/Logger.h"

namespace Engine {
namespace Core {

    InputSystem::InputSystem()
        : m_hwnd(nullptr)
        , m_initialized(false)
        , m_mouseX(0)
        , m_mouseY(0) {
        Logger::Instance().Info("InputSystem created");
    }

    InputSystem::~InputSystem() {
        Shutdown();
        Logger::Instance().Info("InputSystem destroyed");
    }

    bool InputSystem::Initialize(HWND hwnd) {
        if (m_initialized) {
            Logger::Instance().Warning("InputSystem already initialized");
            return true;
        }

        m_hwnd = hwnd;
        m_initialized = true;

        Logger::Instance().Info("InputSystem initialized successfully");
        return true;
    }

    void InputSystem::Shutdown() {
        if (!m_initialized) {
            return;
        }

        m_keyStates.clear();
        m_previousKeyStates.clear();
        m_keyCallbacks.clear();
        m_mouseStates.clear();
        m_previousMouseStates.clear();
        m_mouseCallbacks.clear();

        m_initialized = false;
        Logger::Instance().Info("InputSystem shutdown complete");
    }

    void InputSystem::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        (void)hwnd; // Suppress unused parameter warning
        if (!m_initialized) {
            return;
        }

        switch (message) {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                {
                    int keyCode = static_cast<int>(wParam);
                    m_keyStates[keyCode] = true;
                    
                    // Call key callback if registered
                    auto it = m_keyCallbacks.find(keyCode);
                    if (it != m_keyCallbacks.end()) {
                        it->second(true);
                    }
                }
                break;

            case WM_KEYUP:
            case WM_SYSKEYUP:
                {
                    int keyCode = static_cast<int>(wParam);
                    m_keyStates[keyCode] = false;
                    
                    // Call key callback if registered
                    auto it = m_keyCallbacks.find(keyCode);
                    if (it != m_keyCallbacks.end()) {
                        it->second(false);
                    }
                }
                break;

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
                {
                    int button = GetMouseButtonFromMessage(message);
                    m_mouseStates[button] = true;
                    
                    // Get mouse position
                    m_mouseX = LOWORD(lParam);
                    m_mouseY = HIWORD(lParam);
                    
                    // Call mouse callback if registered
                    auto it = m_mouseCallbacks.find(button);
                    if (it != m_mouseCallbacks.end()) {
                        it->second(m_mouseX, m_mouseY, true);
                    }
                }
                break;

            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
                {
                    int button = GetMouseButtonFromMessage(message);
                    m_mouseStates[button] = false;
                    
                    // Get mouse position
                    m_mouseX = LOWORD(lParam);
                    m_mouseY = HIWORD(lParam);
                    
                    // Call mouse callback if registered
                    auto it = m_mouseCallbacks.find(button);
                    if (it != m_mouseCallbacks.end()) {
                        it->second(m_mouseX, m_mouseY, false);
                    }
                }
                break;

            case WM_MOUSEMOVE:
                {
                    m_mouseX = LOWORD(lParam);
                    m_mouseY = HIWORD(lParam);
                }
                break;
        }
    }

    void InputSystem::Update(float deltaTime) {
        (void)deltaTime; // Suppress unused parameter warning
        
        if (!m_initialized) {
            return;
        }

        // Update previous states
        m_previousKeyStates = m_keyStates;
        m_previousMouseStates = m_mouseStates;
    }

    bool InputSystem::IsKeyDown(int keyCode) const {
        if (!m_initialized) {
            return false;
        }
        
        auto it = m_keyStates.find(keyCode);
        return it != m_keyStates.end() && it->second;
    }

    bool InputSystem::IsKeyPressed(int keyCode) const {
        if (!m_initialized) {
            return false;
        }
        
        bool currentState = IsKeyDown(keyCode);
        auto it = m_previousKeyStates.find(keyCode);
        bool previousState = it != m_previousKeyStates.end() && it->second;
        
        return currentState && !previousState;
    }

    bool InputSystem::IsKeyReleased(int keyCode) const {
        if (!m_initialized) {
            return false;
        }
        
        bool currentState = IsKeyDown(keyCode);
        auto it = m_previousKeyStates.find(keyCode);
        bool previousState = it != m_previousKeyStates.end() && it->second;
        
        return !currentState && previousState;
    }

    void InputSystem::SetKeyCallback(int keyCode, std::function<void(bool)> callback) {
        m_keyCallbacks[keyCode] = callback;
    }

    bool InputSystem::IsMouseButtonDown(int button) const {
        if (!m_initialized) {
            return false;
        }
        
        auto it = m_mouseStates.find(button);
        return it != m_mouseStates.end() && it->second;
    }

    bool InputSystem::IsMouseButtonPressed(int button) const {
        if (!m_initialized) {
            return false;
        }
        
        bool currentState = IsMouseButtonDown(button);
        auto it = m_previousMouseStates.find(button);
        bool previousState = it != m_previousMouseStates.end() && it->second;
        
        return currentState && !previousState;
    }

    bool InputSystem::IsMouseButtonReleased(int button) const {
        if (!m_initialized) {
            return false;
        }
        
        bool currentState = IsMouseButtonDown(button);
        auto it = m_previousMouseStates.find(button);
        bool previousState = it != m_previousMouseStates.end() && it->second;
        
        return !currentState && previousState;
    }

    void InputSystem::GetMousePosition(int& x, int& y) const {
        if (!m_initialized) {
            x = 0;
            y = 0;
            return;
        }
        
        x = m_mouseX;
        y = m_mouseY;
    }

    void InputSystem::SetMouseCallback(int button, std::function<void(int, int, bool)> callback) {
        m_mouseCallbacks[button] = callback;
    }

    void InputSystem::ClearFrameInput() {
        m_previousKeyStates = m_keyStates;
        m_previousMouseStates = m_mouseStates;
    }

    void InputSystem::UpdateKeyboardState() {
        // This could be used for more advanced keyboard state tracking
        // For now, we rely on window messages
    }

    void InputSystem::UpdateMouseState() {
        // This could be used for more advanced mouse state tracking
        // For now, we rely on window messages
    }

    int InputSystem::GetMouseButtonFromMessage(UINT message) const {
        switch (message) {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
                return 0; // Left button
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                return 1; // Right button
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
                return 2; // Middle button
            default:
                return -1; // Unknown button
        }
    }

} // namespace Core
} // namespace Engine
