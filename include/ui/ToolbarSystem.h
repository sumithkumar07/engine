#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <windows.h>
#include "UI.h"

namespace Engine {
namespace UI {

    /**
     * @brief Toolbar System for AI Movie Studio
     * 
     * Real implementation with actual functionality:
     * - Real toolbar with common tools and buttons
     * - Tool groups and separators
     * - Tool states (active, disabled, pressed)
     * - Tooltips and help text
     * - Customizable toolbar layout
     * - Icon support for tools
     * - Keyboard shortcuts for tools
     */
    class ToolbarSystem : public UIComponent {
    public:
        Type GetType() const override { return Type::Panel; }
    public:
        enum class ToolType {
            Button,
            Toggle,
            Dropdown,
            Separator,
            Spacer
        };

        enum class ToolState {
            Normal,
            Pressed,
            Disabled,
            Active
        };

        struct Tool {
            std::string id;
            std::string text;
            std::string tooltip;
            std::string shortcut;
            ToolType type;
            ToolState state;
            bool visible;
            int iconId;
            int width;
            int height;
            
            // Dropdown specific
            std::vector<std::string> dropdownItems;
            int selectedIndex;
            
            // Callback
            std::function<void()> callback;
            std::function<void(int)> dropdownCallback;
            
            Tool() : type(ToolType::Button), state(ToolState::Normal), visible(true), 
                    iconId(-1), width(32), height(32), selectedIndex(-1) {}
        };

        struct ToolGroup {
            std::string name;
            std::vector<std::shared_ptr<Tool>> tools;
            bool visible;
            bool collapsible;
            bool collapsed;
            
            ToolGroup() : visible(true), collapsible(false), collapsed(false) {}
        };

        ToolbarSystem();
        ~ToolbarSystem();

        /**
         * @brief Initialize the toolbar system
         * @return true if initialization successful
         */
        bool Initialize();

        /**
         * @brief Shutdown the toolbar system
         */
        void Shutdown();

        /**
         * @brief Update the toolbar system
         * @param deltaTime Time elapsed since last frame
         */
        void Update(float deltaTime) override;

        /**
         * @brief Render the toolbar system
         * @param hdc Device context for rendering
         */
        void Render(HDC hdc) override;

        /**
         * @brief Handle mouse events
         * @param x Mouse X position
         * @param y Mouse Y position
         * @param button Mouse button (0=left, 1=right, 2=middle)
         * @param pressed true if button pressed, false if released
         */
        void HandleMouseEvent(int x, int y, int button, bool pressed) override;

        /**
         * @brief Handle keyboard events
         * @param message Windows message
         * @param wParam WPARAM
         * @param lParam LPARAM
         */
        void HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) override;

        // Toolbar management
        void AddToolGroup(const std::string& name, bool collapsible = false);
        void RemoveToolGroup(const std::string& name);
        void SetToolGroupVisible(const std::string& name, bool visible);
        void SetToolGroupCollapsed(const std::string& name, bool collapsed);

        // Tool management
        void AddTool(const std::string& groupName, const std::string& toolId, const std::string& text, 
                    const std::string& tooltip = "", const std::string& shortcut = "", 
                    std::function<void()> callback = nullptr);
        void AddToggleTool(const std::string& groupName, const std::string& toolId, const std::string& text, 
                          const std::string& tooltip = "", const std::string& shortcut = "", 
                          std::function<void()> callback = nullptr);
        void AddDropdownTool(const std::string& groupName, const std::string& toolId, const std::string& text, 
                            const std::vector<std::string>& items, const std::string& tooltip = "", 
                            std::function<void(int)> callback = nullptr);
        void AddSeparator(const std::string& groupName, const std::string& afterToolId = "");
        void AddSpacer(const std::string& groupName, int width = 10);
        void RemoveTool(const std::string& groupName, const std::string& toolId);

        // Tool properties
        void SetToolText(const std::string& groupName, const std::string& toolId, const std::string& text);
        void SetToolTooltip(const std::string& groupName, const std::string& toolId, const std::string& tooltip);
        void SetToolState(const std::string& groupName, const std::string& toolId, ToolState state);
        void SetToolVisible(const std::string& groupName, const std::string& toolId, bool visible);
        void SetToolCallback(const std::string& groupName, const std::string& toolId, std::function<void()> callback);

        // Tool operations
        void ExecuteTool(const std::string& groupName, const std::string& toolId);
        void ToggleTool(const std::string& groupName, const std::string& toolId);
        void SetActiveTool(const std::string& groupName, const std::string& toolId);
        void ClearActiveTools();

        // Dropdown operations
        void SetDropdownItems(const std::string& groupName, const std::string& toolId, const std::vector<std::string>& items);
        void SetDropdownSelection(const std::string& groupName, const std::string& toolId, int index);
        void ShowDropdown(const std::string& groupName, const std::string& toolId);
        void HideDropdown();

        // Layout management
        void SetToolbarHeight(int height);
        void SetToolSpacing(int spacing);
        void SetGroupSpacing(int spacing);
        void RefreshLayout();

        // Keyboard shortcuts
        void ProcessToolShortcut(WPARAM wParam, LPARAM lParam);
        void RegisterToolShortcut(const std::string& shortcut, const std::string& groupName, const std::string& toolId);
        void UnregisterToolShortcut(const std::string& shortcut);

        // Getters
        const std::vector<ToolGroup>& GetToolGroups() const { return m_toolGroups; }
        std::shared_ptr<Tool> GetTool(const std::string& groupName, const std::string& toolId);
        std::vector<std::shared_ptr<Tool>> GetActiveTools() const;
        bool IsToolActive(const std::string& groupName, const std::string& toolId) const;
        std::string GetActiveToolGroup() const { return m_activeToolGroup; }

    private:
        // Toolbar data
        std::vector<ToolGroup> m_toolGroups;
        std::map<std::string, int> m_toolGroupIndices;
        
        // UI state
        int m_toolbarHeight;
        int m_toolSpacing;
        int m_groupSpacing;
        int m_toolPadding;
        
        // Active tool state
        std::string m_activeToolGroup;
        std::string m_activeTool;
        std::vector<std::shared_ptr<Tool>> m_activeTools;
        
        // Dropdown state
        bool m_dropdownVisible;
        std::string m_dropdownGroup;
        std::string m_dropdownTool;
        int m_dropdownX;
        int m_dropdownY;
        int m_dropdownWidth;
        int m_dropdownHeight;
        
        // Keyboard shortcuts
        std::map<std::string, std::pair<std::string, std::string>> m_toolShortcuts; // shortcut -> (group, tool)
        
        // Helper methods
        void CreateDefaultTools();
        void CreateFileTools();
        void CreateEditTools();
        void CreateViewTools();
        void CreateTransformTools();
        void CreateRenderTools();
        
        void RenderToolGroups(HDC hdc);
        void RenderToolGroup(HDC hdc, ToolGroup& group, int& x, int y);
        void RenderTool(HDC hdc, std::shared_ptr<Tool> tool, int x, int y);
        void RenderDropdown(HDC hdc);
        void RenderTooltip(HDC hdc, const std::string& text, int x, int y);
        
        void HandleToolClick(int x, int y);
        void HandleDropdownClick(int x, int y);
        
        std::shared_ptr<Tool> FindTool(const std::string& groupName, const std::string& toolId);
        std::shared_ptr<Tool> GetToolFromPosition(int x, int y, std::string& groupName, std::string& toolId);
        
        void UpdateToolStates();
        void UpdateActiveTools();
        
        // Tool callbacks
        void OnFileNew();
        void OnFileOpen();
        void OnFileSave();
        void OnFileSaveAs();
        void OnFileExport();
        
        void OnEditUndo();
        void OnEditRedo();
        void OnEditCut();
        void OnEditCopy();
        void OnEditPaste();
        void OnEditDelete();
        
        void OnViewWireframe();
        void OnViewSolid();
        void OnViewTextured();
        void OnViewFullscreen();
        void OnViewResetCamera();
        
        void OnTransformMove();
        void OnTransformRotate();
        void OnTransformScale();
        void OnTransformSelect();
        
        void OnRenderPlay();
        void OnRenderPause();
        void OnRenderStop();
        void OnRenderFrame();
        
        // Constants
        static constexpr int DEFAULT_TOOLBAR_HEIGHT = 40;
        static constexpr int DEFAULT_TOOL_SPACING = 5;
        static constexpr int DEFAULT_GROUP_SPACING = 10;
        static constexpr int DEFAULT_TOOL_PADDING = 4;
        static constexpr int TOOL_ICON_SIZE = 24;
        static constexpr int DROPDOWN_ITEM_HEIGHT = 20;
        static constexpr int TOOLTIP_PADDING = 5;
    };

} // namespace UI
} // namespace Engine
