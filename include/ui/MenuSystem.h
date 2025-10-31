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
     * @brief Menu System for AI Movie Studio
     * 
     * Real implementation with actual functionality:
     * - Real application menu bar (File, Edit, View, Tools, Help)
     * - Dropdown menus with submenus and separators
     * - Keyboard shortcuts and accelerators
     * - Menu item states (enabled/disabled, checked/unchecked)
     * - Context menus and right-click menus
     * - Menu item callbacks and event handling
     */
    class MenuSystem : public UIComponent {
    public:
        Type GetType() const override { return Type::Panel; }
    public:
        enum class MenuItemType {
            Normal,
            Separator,
            Checkbox,
            Radio,
            Submenu
        };

        enum class MenuItemState {
            Enabled,
            Disabled,
            Checked,
            Unchecked
        };

        struct MenuItem {
            std::string id;
            std::string text;
            std::string shortcut;
            MenuItemType type;
            MenuItemState state;
            bool visible;
            int iconId;
            
            // Submenu
            std::vector<std::shared_ptr<MenuItem>> subItems;
            
            // Callback
            std::function<void()> callback;
            
            MenuItem() : type(MenuItemType::Normal), state(MenuItemState::Enabled), 
                        visible(true), iconId(-1) {}
        };

        struct MenuBar {
            std::string name;
            std::vector<std::shared_ptr<MenuItem>> items;
            bool visible;
            
            MenuBar() : visible(true) {}
        };

        MenuSystem();
        ~MenuSystem();

        /**
         * @brief Initialize the menu system
         * @return true if initialization successful
         */
        bool Initialize();

        /**
         * @brief Shutdown the menu system
         */
        void Shutdown();

        /**
         * @brief Update the menu system
         * @param deltaTime Time elapsed since last frame
         */
        void Update(float deltaTime) override;

        /**
         * @brief Render the menu system
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

        // Menu bar management
        void AddMenuBar(const std::string& name);
        void RemoveMenuBar(const std::string& name);
        void SetMenuBarVisible(const std::string& name, bool visible);
        bool IsMenuBarVisible(const std::string& name) const;

        // Menu item management
        void AddMenuItem(const std::string& menuName, const std::string& itemId, const std::string& text, 
                        const std::string& shortcut = "", std::function<void()> callback = nullptr);
        void AddSubMenuItem(const std::string& menuName, const std::string& parentId, const std::string& itemId, 
                           const std::string& text, const std::string& shortcut = "", std::function<void()> callback = nullptr);
        void AddSeparator(const std::string& menuName, const std::string& afterItemId = "");
        void RemoveMenuItem(const std::string& menuName, const std::string& itemId);
        void ClearMenu(const std::string& menuName);

        // Menu item properties
        void SetMenuItemText(const std::string& menuName, const std::string& itemId, const std::string& text);
        void SetMenuItemShortcut(const std::string& menuName, const std::string& itemId, const std::string& shortcut);
        void SetMenuItemState(const std::string& menuName, const std::string& itemId, MenuItemState state);
        void SetMenuItemVisible(const std::string& menuName, const std::string& itemId, bool visible);
        void SetMenuItemCallback(const std::string& menuName, const std::string& itemId, std::function<void()> callback);

        // Context menus
        void ShowContextMenu(int x, int y, const std::string& contextMenuId);
        void HideContextMenu();
        void AddContextMenu(const std::string& contextMenuId, const std::string& name);
        void AddContextMenuItem(const std::string& contextMenuId, const std::string& itemId, 
                               const std::string& text, const std::string& shortcut = "", 
                               std::function<void()> callback = nullptr);

        // Menu operations
        void ExecuteMenuItem(const std::string& menuName, const std::string& itemId);
        void ExecuteContextMenuItem(const std::string& contextMenuId, const std::string& itemId);
        void RefreshMenus();

        // Keyboard shortcuts
        void ProcessAccelerator(WPARAM wParam, LPARAM lParam);
        void RegisterAccelerator(const std::string& shortcut, std::function<void()> callback);
        void UnregisterAccelerator(const std::string& shortcut);

        // Getters
        const std::vector<MenuBar>& GetMenuBars() const { return m_menuBars; }
        std::shared_ptr<MenuItem> GetMenuItem(const std::string& menuName, const std::string& itemId);
        std::shared_ptr<MenuItem> GetContextMenuItem(const std::string& contextMenuId, const std::string& itemId);
        bool IsContextMenuVisible() const { return m_contextMenuVisible; }
        std::string GetActiveMenu() const { return m_activeMenu; }

    private:
        // Menu data
        std::vector<MenuBar> m_menuBars;
        std::map<std::string, int> m_menuBarIndices;
        std::map<std::string, std::vector<std::shared_ptr<MenuItem>>> m_contextMenus;
        
        // UI state
        int m_menuBarHeight;
        int m_menuItemHeight;
        int m_menuPadding;
        int m_submenuOffset;
        
        // Active menu state
        std::string m_activeMenu;
        int m_activeMenuItem;
        bool m_menuOpen;
        int m_menuX;
        int m_menuY;
        int m_menuWidth;
        int m_menuHeight;
        
        // Context menu state
        bool m_contextMenuVisible;
        std::string m_activeContextMenu;
        int m_contextMenuX;
        int m_contextMenuY;
        int m_contextMenuWidth;
        int m_contextMenuHeight;
        
        // Keyboard shortcuts
        std::map<std::string, std::function<void()>> m_accelerators;
        
        // Helper methods
        void CreateDefaultMenus();
        void CreateFileMenu();
        void CreateEditMenu();
        void CreateViewMenu();
        void CreateToolsMenu();
        void CreateHelpMenu();
        
        void RenderMenuBar(HDC hdc);
        void RenderMenuBarItem(HDC hdc, const std::string& menuName, int x, int y, int width, int height);
        void RenderDropdownMenu(HDC hdc, const std::string& menuName, int x, int y);
        void RenderMenuItem(HDC hdc, std::shared_ptr<MenuItem> item, int x, int y, int width, int height, bool isHighlighted);
        void RenderContextMenu(HDC hdc);
        
        void HandleMenuBarClick(int x, int y);
        void HandleMenuItemClick(int x, int y);
        void HandleContextMenuClick(int x, int y);
        
        std::shared_ptr<MenuItem> FindMenuItem(const std::string& menuName, const std::string& itemId);
        std::shared_ptr<MenuItem> FindContextMenuItem(const std::string& contextMenuId, const std::string& itemId);
        
        void OpenMenu(const std::string& menuName, int x, int y);
        void CloseMenu();
        void CloseContextMenu();
        
        bool IsPointInMenu(int x, int y);
        bool IsPointInContextMenu(int x, int y);
        bool IsPointInMenuItem(int x, int y, int itemX, int itemY, int itemWidth, int itemHeight);
        
        void CalculateMenuSize(const std::string& menuName, int& width, int& height);
        void CalculateContextMenuSize(const std::string& contextMenuId, int& width, int& height);
        
        // Menu item callbacks
        void OnFileNew();
        void OnFileOpen();
        void OnFileSave();
        void OnFileSaveAs();
        void OnFileExport();
        void OnFileExit();
        
        void OnEditUndo();
        void OnEditRedo();
        void OnEditCut();
        void OnEditCopy();
        void OnEditPaste();
        void OnEditDelete();
        void OnEditSelectAll();
        
        void OnViewWireframe();
        void OnViewSolid();
        void OnViewTextured();
        void OnViewFullscreen();
        void OnViewResetCamera();
        
        void OnToolsPreferences();
        void OnToolsAssetManager();
        void OnToolsPerformanceMonitor();
        
        void OnHelpAbout();
        void OnHelpDocumentation();
        void OnHelpKeyboardShortcuts();
        
        // Constants
        static constexpr int DEFAULT_MENU_BAR_HEIGHT = 25;
        static constexpr int DEFAULT_MENU_ITEM_HEIGHT = 20;
        static constexpr int DEFAULT_MENU_PADDING = 5;
        static constexpr int DEFAULT_SUBMENU_OFFSET = 20;
        static constexpr int MENU_ITEM_ICON_SIZE = 16;
        static constexpr int MENU_ITEM_SHORTCUT_WIDTH = 100;
    };

} // namespace UI
} // namespace Engine
