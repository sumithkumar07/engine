#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <windows.h>

namespace Engine {
namespace Core {

    class InputSystem {
    public:
        InputSystem();
        ~InputSystem();

        // Initialization
        bool Initialize(HWND hwnd);
        void Shutdown();

        // Input handling
        void ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        void Update(float deltaTime);

        // Keyboard input
        bool IsKeyDown(int keyCode) const;
        bool IsKeyPressed(int keyCode) const;
        bool IsKeyReleased(int keyCode) const;
        void SetKeyCallback(int keyCode, std::function<void(bool)> callback);

        // Mouse input
        bool IsMouseButtonDown(int button) const;
        bool IsMouseButtonPressed(int button) const;
        bool IsMouseButtonReleased(int button) const;
        void GetMousePosition(int& x, int& y) const;
        void SetMouseCallback(int button, std::function<void(int, int, bool)> callback);

        // Input state
        void ClearFrameInput();
        bool IsInitialized() const { return m_initialized; }

    private:
        HWND m_hwnd;
        bool m_initialized;

        // Keyboard state
        std::unordered_map<int, bool> m_keyStates;
        std::unordered_map<int, bool> m_previousKeyStates;
        std::unordered_map<int, std::function<void(bool)>> m_keyCallbacks;

        // Mouse state
        std::unordered_map<int, bool> m_mouseStates;
        std::unordered_map<int, bool> m_previousMouseStates;
        std::unordered_map<int, std::function<void(int, int, bool)>> m_mouseCallbacks;
        int m_mouseX;
        int m_mouseY;

        // Helper methods
        void UpdateKeyboardState();
        void UpdateMouseState();
        int GetMouseButtonFromMessage(UINT message) const;
    };

} // namespace Core
} // namespace Engine
