#include "ui/UIManager.h"
#include "ui/AIMovieStudioUI.h"
#include "ui/UIRenderer.h"
#include "core/Logger.h"
#include <algorithm>

namespace Engine {
namespace UI {

    UIManager::UIManager()
        : m_hwnd(nullptr), m_width(0), m_height(0) {
        
        Engine::Core::Logger::Instance().Info("UIManager created");
    }

    UIManager::~UIManager() {
        Shutdown();
        Engine::Core::Logger::Instance().Info("UIManager destroyed");
    }

    void UIManager::Update(float deltaTime) {
        // Update main UI
        if (m_mainUI) {
            m_mainUI->Update(deltaTime);
        }
        
        // Update all UI components
        for (auto& component : m_components) {
            if (component) {
                component->Update(deltaTime);
            }
        }
    }

    bool UIManager::Initialize(HWND hwnd) {
        Engine::Core::Logger::Instance().Info("Initializing UIManager...");
        
        m_hwnd = hwnd;
        
        // Get window size
        RECT rect;
        if (GetClientRect(hwnd, &rect)) {
            m_width = rect.right - rect.left;
            m_height = rect.bottom - rect.top;
        } else {
            Engine::Core::Logger::Instance().Error("Failed to get window client rect");
            return false;
        }
        
        Engine::Core::Logger::Instance().Info("UIManager initialized successfully");
        return true;
    }

    void UIManager::Shutdown() {
        Engine::Core::Logger::Instance().Info("Shutting down UIManager...");
        
        // Clear main UI
        if (m_mainUI) {
            m_mainUI->Shutdown();
            m_mainUI.reset();
        }
        
        // Clear components
        m_components.clear();
        
        // Reset renderer
        m_renderer.reset();
        
        m_hwnd = nullptr;
        m_width = 0;
        m_height = 0;
        
        Engine::Core::Logger::Instance().Info("UIManager shutdown complete");
    }

    void UIManager::SetMainUI(std::shared_ptr<AIMovieStudioUI> mainUI) {
        m_mainUI = mainUI;
        Engine::Core::Logger::Instance().Info("Main UI set in UIManager");
    }

    void UIManager::AddComponent(std::shared_ptr<UIComponent> component) {
        if (!component) {
            Engine::Core::Logger::Instance().Warning("Attempted to add null component to UIManager");
            return;
        }
        
        m_components.push_back(component);
        Engine::Core::Logger::Instance().Debug("Added component '" + component->GetName() + "' to UIManager");
    }

    void UIManager::RemoveComponent(std::shared_ptr<UIComponent> component) {
        if (!component) {
            Engine::Core::Logger::Instance().Warning("Attempted to remove null component from UIManager");
            return;
        }
        
        auto it = std::find(m_components.begin(), m_components.end(), component);
        if (it != m_components.end()) {
            m_components.erase(it);
            Engine::Core::Logger::Instance().Debug("Removed component '" + component->GetName() + "' from UIManager");
        } else {
            Engine::Core::Logger::Instance().Warning("Component '" + component->GetName() + "' not found in UIManager");
        }
    }

    void UIManager::RemoveComponent(const std::string& name) {
        auto it = std::find_if(m_components.begin(), m_components.end(),
            [&name](const std::shared_ptr<UIComponent>& component) {
                return component && component->GetName() == name;
            });
        
        if (it != m_components.end()) {
            Engine::Core::Logger::Instance().Debug("Removing component '" + name + "' from UIManager");
            m_components.erase(it);
        } else {
            Engine::Core::Logger::Instance().Warning("Component '" + name + "' not found in UIManager");
        }
    }

    std::shared_ptr<UIComponent> UIManager::GetComponent(const std::string& name) {
        auto it = std::find_if(m_components.begin(), m_components.end(),
            [&name](const std::shared_ptr<UIComponent>& component) {
                return component && component->GetName() == name;
            });
        
        if (it != m_components.end()) {
            return *it;
        }
        
        return nullptr;
    }

    void UIManager::Render() {
        if (!m_hwnd) {
            return;
        }
        
        // Get device context
        HDC hdc = GetDC(m_hwnd);
        if (!hdc) {
            Engine::Core::Logger::Instance().Error("Failed to get device context");
            return;
        }
        
        // Render main UI
        if (m_mainUI) {
            m_mainUI->Render();
        }
        
        // Render additional components
        for (auto& component : m_components) {
            if (component && component->IsVisible()) {
                component->Render(hdc);
            }
        }
        
        // Release device context
        ReleaseDC(m_hwnd, hdc);
    }

    void UIManager::HandleMouseEvent(int x, int y, int button, bool isDown) {
        // Find component at mouse position
        auto component = FindComponentAt(x, y);
        
        if (component) {
            PropagateMouseEvent(component, x, y, button, isDown);
        }
    }

    void UIManager::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) {
        // Forward to main UI first
        if (m_mainUI) {
            m_mainUI->HandleKeyboardEvent(message, wParam, lParam);
        }
        
        // Forward to components
        for (auto& component : m_components) {
            if (component && component->IsVisible()) {
                PropagateKeyboardEvent(component, message, wParam, lParam);
            }
        }
    }

    void UIManager::HandleWindowResize(int width, int height) {
        m_width = width;
        m_height = height;
        
        // Notify main UI
        if (m_mainUI) {
            m_mainUI->HandleWindowResize(width, height);
        }
        
        Engine::Core::Logger::Instance().Debug("UIManager resized to: " + std::to_string(width) + "x" + std::to_string(height));
    }

    std::shared_ptr<UIComponent> UIManager::FindComponentAt(int x, int y) {
        // Check main UI first
        if (m_mainUI) {
            // TODO: Implement proper hit testing for main UI
            // For now, just return the main UI if it contains the point
        }
        
        // Check components (in reverse order for proper z-ordering)
        for (auto it = m_components.rbegin(); it != m_components.rend(); ++it) {
            auto& component = *it;
            if (component && component->IsVisible() && component->Contains(x, y)) {
                return component;
            }
        }
        
        return nullptr;
    }

    void UIManager::PropagateMouseEvent(std::shared_ptr<UIComponent> component, int x, int y, int button, bool isDown) {
        if (!component) {
            return;
        }
        
        component->HandleMouseEvent(x, y, button, isDown);
    }

    void UIManager::PropagateKeyboardEvent(std::shared_ptr<UIComponent> component, UINT message, WPARAM wParam, LPARAM lParam) {
        if (!component) {
            return;
        }
        
        component->HandleKeyboardEvent(message, wParam, lParam);
    }

} // namespace UI
} // namespace Engine
