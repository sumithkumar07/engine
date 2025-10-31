#pragma once

#include <memory>
#include <windows.h>
#include "UI.h"
#include "SceneHierarchy.h"
#include "AICommandProcessor.h"
#include "TimelineSystem.h"
#include "AssetBrowser.h"
#include "PropertiesPanel.h"
#include "MenuSystem.h"
#include "ToolbarSystem.h"

namespace Engine {
namespace UI {

    // Forward declarations
    class UIManager;
    class ViewportRenderer;
    class SceneHierarchy;
    class AICommandProcessor;
    class TimelineSystem;
    class AssetBrowser;
    class PropertiesPanel;
    class MenuSystem;
    class ToolbarSystem;

    /**
     * @brief AIMovieStudioUI class - Same as before, but with proper layout management
     * 
     * Main UI layout for the AI Movie Studio application.
     * Same design as the previous implementation, but with proper panel management.
     */
    class AIMovieStudioUI {
    public:
        AIMovieStudioUI();
        ~AIMovieStudioUI();

        // Initialization
        bool Initialize(UIManager* uiManager);
        void Shutdown();
        void Update(float deltaTime);

        // Layout management
        void UpdateLayout(int width, int height);
        void Render();
        void Render(HDC hdc); // For UI integration
        void HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

        // Panel access
        std::shared_ptr<UIPanel> GetLeftPanel() const { return m_leftPanel; }
        std::shared_ptr<UIPanel> GetCenterPanel() const { return m_centerPanel; }
        std::shared_ptr<UIPanel> GetRightPanel() const { return m_rightPanel; }
        std::shared_ptr<UIPanel> GetBottomPanel() const { return m_bottomPanel; }
        std::shared_ptr<UIPanel> GetTopPanel() const { return m_topPanel; }

        // Component access
        std::shared_ptr<ViewportRenderer> GetViewportRenderer() const { return m_viewportRenderer; }
        std::shared_ptr<SceneHierarchy> GetSceneHierarchy() const { return m_sceneHierarchy; }
        std::shared_ptr<AICommandProcessor> GetAICommandProcessor() const { return m_aiCommandProcessor; }
        std::shared_ptr<TimelineSystem> GetTimelineSystem() const { return m_timelineSystem; }
        std::shared_ptr<AssetBrowser> GetAssetBrowser() const { return m_assetBrowser; }
        std::shared_ptr<PropertiesPanel> GetPropertiesPanel() const { return m_propertiesPanel; }
        std::shared_ptr<MenuSystem> GetMenuSystem() const { return m_menuSystem; }
        std::shared_ptr<ToolbarSystem> GetToolbarSystem() const { return m_toolbarSystem; }

        // Event handling
        void HandleMouseEvent(int x, int y, int button, bool isDown);
        void HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam);
        void HandleWindowResize(int width, int height);

    private:
        // UI Manager reference
        UIManager* m_uiManager;
        bool m_initialized;

        // Main panels
        std::shared_ptr<UIPanel> m_leftPanel;
        std::shared_ptr<UIPanel> m_centerPanel;
        std::shared_ptr<UIPanel> m_rightPanel;
        std::shared_ptr<UIPanel> m_bottomPanel;
        std::shared_ptr<UIPanel> m_topPanel;
        
        // Left panel sub-panels for better organization
        std::shared_ptr<UIPanel> m_leftTopPanel;      // Scene Hierarchy
        std::shared_ptr<UIPanel> m_leftMiddlePanel;   // Asset Browser
        std::shared_ptr<UIPanel> m_leftBottomPanel;   // AI Command Processor

        // UI components
        std::shared_ptr<ViewportRenderer> m_viewportRenderer;
        std::shared_ptr<SceneHierarchy> m_sceneHierarchy;
        std::shared_ptr<AICommandProcessor> m_aiCommandProcessor;
        std::shared_ptr<TimelineSystem> m_timelineSystem;
        std::shared_ptr<AssetBrowser> m_assetBrowser;
        std::shared_ptr<PropertiesPanel> m_propertiesPanel;
        std::shared_ptr<MenuSystem> m_menuSystem;
        std::shared_ptr<ToolbarSystem> m_toolbarSystem;

        std::string m_lastSelectedHierarchyName;

        // Layout constants - AI-First layout with smart panel management
        static constexpr int LEFT_PANEL_WIDTH = 320;
        static constexpr int RIGHT_PANEL_WIDTH = 320;
        static constexpr int BOTTOM_PANEL_HEIGHT = 200;  // Timeline + Properties
        static constexpr int TOP_PANEL_HEIGHT = 60;
        static constexpr int TIMELINE_HEIGHT = 100;      // Timeline portion
        static constexpr int PROPERTIES_HEIGHT = 100;    // Properties portion

        // Private methods
        void CreatePanels();
        void CreateComponents();
        void UpdatePanelLayouts(int width, int height);
    };

} // namespace UI
} // namespace Engine
