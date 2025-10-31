#pragma once

#include <memory>
#include <vector>
#include <windows.h>
#include "UI.h"

namespace Engine {
namespace UI {

    // Forward declarations
    class UIRenderer;
    class AIMovieStudioUI;

    /**
     * @brief UIManager class - Same as before, but with proper event handling
     * 
     * Manages all UI components and handles events.
     * Same design as the previous implementation, but with proper event propagation.
     */
    class UIManager {
    public:
        UIManager();
        ~UIManager();

        // Initialization
        bool Initialize(HWND hwnd);
        void Shutdown();
        void Update(float deltaTime);

        // Main UI
        void SetMainUI(std::shared_ptr<AIMovieStudioUI> mainUI);
        std::shared_ptr<AIMovieStudioUI> GetMainUI() const { return m_mainUI; }

        // Component management
        void AddComponent(std::shared_ptr<UIComponent> component);
        void RemoveComponent(std::shared_ptr<UIComponent> component);
        void RemoveComponent(const std::string& name);
        std::shared_ptr<UIComponent> GetComponent(const std::string& name);

        // Rendering
        void Render();

        // Event handling
        void HandleMouseEvent(int x, int y, int button, bool isDown);
        void HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam);
        void HandleWindowResize(int width, int height);

        // Getters
        HWND GetWindowHandle() const { return m_hwnd; }
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }

    private:
        // Window and rendering
        HWND m_hwnd;
        int m_width;
        int m_height;
        std::unique_ptr<UIRenderer> m_renderer;

        // UI components
        std::vector<std::shared_ptr<UIComponent>> m_components;
        std::shared_ptr<AIMovieStudioUI> m_mainUI;

        // Event handling
        std::shared_ptr<UIComponent> FindComponentAt(int x, int y);
        void PropagateMouseEvent(std::shared_ptr<UIComponent> component, int x, int y, int button, bool isDown);
        void PropagateKeyboardEvent(std::shared_ptr<UIComponent> component, UINT message, WPARAM wParam, LPARAM lParam);
    };

} // namespace UI
} // namespace Engine
