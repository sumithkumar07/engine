#include "ui/MenuSystem.h"
#include "core/Logger.h"
#include <algorithm>
#include <sstream>

namespace Engine {
namespace UI {

    MenuSystem::MenuSystem()
        : UIComponent("MenuSystem", 0, 0, 800, 25)
        , m_menuBarHeight(DEFAULT_MENU_BAR_HEIGHT)
        , m_menuItemHeight(DEFAULT_MENU_ITEM_HEIGHT)
        , m_menuPadding(DEFAULT_MENU_PADDING)
        , m_submenuOffset(DEFAULT_SUBMENU_OFFSET)
        , m_activeMenuItem(-1)
        , m_menuOpen(false)
        , m_menuX(0)
        , m_menuY(0)
        , m_menuWidth(0)
        , m_menuHeight(0)
        , m_contextMenuVisible(false)
        , m_contextMenuX(0)
        , m_contextMenuY(0)
        , m_contextMenuWidth(0)
        , m_contextMenuHeight(0)
    {
        Engine::Core::Logger::Instance().Debug("MenuSystem created");
    }

    MenuSystem::~MenuSystem()
    {
        Shutdown();
        Engine::Core::Logger::Instance().Debug("MenuSystem destroyed");
    }

    bool MenuSystem::Initialize()
    {
        CreateDefaultMenus();
        Engine::Core::Logger::Instance().Info("MenuSystem initialized");
        return true;
    }

    void MenuSystem::Shutdown()
    {
        m_menuBars.clear();
        m_menuBarIndices.clear();
        m_contextMenus.clear();
        m_accelerators.clear();
        
        Engine::Core::Logger::Instance().Info("MenuSystem shutdown");
    }

    void MenuSystem::Update(float deltaTime)
    {
        (void)deltaTime; // Suppress unused parameter warning
    }

    void MenuSystem::Render(HDC hdc)
    {
        if (!m_visible) {
            return;
        }

        // Draw menu bar background
        HBRUSH hBrush = CreateSolidBrush(RGB(50, 50, 50));
        RECT menuBarRect = { m_x, m_y, m_x + m_width, m_y + m_height }; // use full height
        FillRect(hdc, &menuBarRect, hBrush);
        DeleteObject(hBrush);

        // Draw menu bar border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, m_x, m_y, NULL);
        LineTo(hdc, m_x + m_width, m_y);
        LineTo(hdc, m_x + m_width, m_y + m_menuBarHeight);
        LineTo(hdc, m_x, m_y + m_menuBarHeight);
        LineTo(hdc, m_x, m_y);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        // Render menu bar items
        RenderMenuBar(hdc);
        
        // Render dropdown menu if open
        if (m_menuOpen) {
            RenderDropdownMenu(hdc, m_activeMenu, m_menuX, m_menuY);
        }
        
        // Render context menu if visible
        if (m_contextMenuVisible) {
            RenderContextMenu(hdc);
        }
    }

    void MenuSystem::HandleMouseEvent(int x, int y, int button, bool pressed)
    {
        if (!m_visible) {
            return;
        }

        // Check if click is within menu bar bounds
        if (x >= m_x && x <= m_x + m_width && y >= m_y && y <= m_y + m_menuBarHeight) {
            if (button == 0 && pressed) { // Left mouse button press
                HandleMenuBarClick(x, y);
            }
        }
        // Check if click is within dropdown menu
        else if (m_menuOpen && IsPointInMenu(x, y)) {
            if (button == 0 && pressed) {
                HandleMenuItemClick(x, y);
            }
        }
        // Check if click is within context menu
        else if (m_contextMenuVisible && IsPointInContextMenu(x, y)) {
            if (button == 0 && pressed) {
                HandleContextMenuClick(x, y);
            }
        }
        // Close menus if clicking outside
        else if (button == 0 && pressed) {
            CloseMenu();
            CloseContextMenu();
        }
    }

    void MenuSystem::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (!m_visible) {
            return;
        }

        if (message == WM_KEYDOWN) {
            // Handle accelerator keys
            ProcessAccelerator(wParam, lParam);
            
            // Handle menu navigation
            if (m_menuOpen) {
                switch (wParam) {
                    case VK_ESCAPE:
                        CloseMenu();
                        break;
                    case VK_UP:
                        if (m_activeMenuItem > 0) {
                            m_activeMenuItem--;
                        }
                        break;
                    case VK_DOWN:
                        {
                            auto menuIt = m_menuBarIndices.find(m_activeMenu);
                            if (menuIt != m_menuBarIndices.end()) {
                                int menuIndex = menuIt->second;
                                if (m_activeMenuItem < static_cast<int>(m_menuBars[menuIndex].items.size()) - 1) {
                                    m_activeMenuItem++;
                                }
                            }
                        }
                        break;
                    case VK_RETURN:
                        {
                            auto menuIt = m_menuBarIndices.find(m_activeMenu);
                            if (menuIt != m_menuBarIndices.end()) {
                                int menuIndex = menuIt->second;
                                if (m_activeMenuItem >= 0 && m_activeMenuItem < static_cast<int>(m_menuBars[menuIndex].items.size())) {
                                    auto item = m_menuBars[menuIndex].items[m_activeMenuItem];
                                    if (item->callback) {
                                        item->callback();
                                    }
                                    CloseMenu();
                                }
                            }
                        }
                        break;
                }
            }
        }
    }

    void MenuSystem::AddMenuBar(const std::string& name)
    {
        MenuBar newMenuBar;
        newMenuBar.name = name;
        m_menuBars.push_back(newMenuBar);
        m_menuBarIndices[name] = static_cast<int>(m_menuBars.size()) - 1;
    }

    void MenuSystem::RemoveMenuBar(const std::string& name)
    {
        auto it = m_menuBarIndices.find(name);
        if (it != m_menuBarIndices.end()) {
            int menuIndex = it->second;
            m_menuBars.erase(m_menuBars.begin() + menuIndex);
            m_menuBarIndices.erase(it);
            
            // Update indices
            for (auto& pair : m_menuBarIndices) {
                if (pair.second > menuIndex) {
                    pair.second--;
                }
            }
        }
    }

    void MenuSystem::SetMenuBarVisible(const std::string& name, bool visible)
    {
        auto it = m_menuBarIndices.find(name);
        if (it != m_menuBarIndices.end()) {
            int menuIndex = it->second;
            m_menuBars[menuIndex].visible = visible;
        }
    }

    bool MenuSystem::IsMenuBarVisible(const std::string& name) const
    {
        auto it = m_menuBarIndices.find(name);
        if (it != m_menuBarIndices.end()) {
            int menuIndex = it->second;
            return m_menuBars[menuIndex].visible;
        }
        return false;
    }

    void MenuSystem::AddMenuItem(const std::string& menuName, const std::string& itemId, const std::string& text, 
                                const std::string& shortcut, std::function<void()> callback)
    {
        auto it = m_menuBarIndices.find(menuName);
        if (it != m_menuBarIndices.end()) {
            int menuIndex = it->second;
            auto item = std::make_shared<MenuItem>();
            item->id = itemId;
            item->text = text;
            item->shortcut = shortcut;
            item->callback = callback;
            m_menuBars[menuIndex].items.push_back(item);
        }
    }

    void MenuSystem::AddSubMenuItem(const std::string& menuName, const std::string& parentId, const std::string& itemId, 
                                   const std::string& text, const std::string& shortcut, std::function<void()> callback)
    {
        auto parentItem = FindMenuItem(menuName, parentId);
        if (parentItem) {
            auto item = std::make_shared<MenuItem>();
            item->id = itemId;
            item->text = text;
            item->shortcut = shortcut;
            item->callback = callback;
            parentItem->subItems.push_back(item);
        }
    }

    void MenuSystem::AddSeparator(const std::string& menuName, const std::string& afterItemId)
    {
        (void)afterItemId; // Suppress unused parameter warning
        auto it = m_menuBarIndices.find(menuName);
        if (it != m_menuBarIndices.end()) {
            int menuIndex = it->second;
            auto separator = std::make_shared<MenuItem>();
            separator->id = "separator_" + std::to_string(m_menuBars[menuIndex].items.size());
            separator->text = "-";
            separator->type = MenuItemType::Separator;
            m_menuBars[menuIndex].items.push_back(separator);
        }
    }

    void MenuSystem::RemoveMenuItem(const std::string& menuName, const std::string& itemId)
    {
        auto it = m_menuBarIndices.find(menuName);
        if (it != m_menuBarIndices.end()) {
            int menuIndex = it->second;
            auto& items = m_menuBars[menuIndex].items;
            items.erase(
                std::remove_if(items.begin(), items.end(),
                    [&itemId](const std::shared_ptr<MenuItem>& item) { return item->id == itemId; }),
                items.end());
        }
    }

    void MenuSystem::ClearMenu(const std::string& menuName)
    {
        auto it = m_menuBarIndices.find(menuName);
        if (it != m_menuBarIndices.end()) {
            int menuIndex = it->second;
            m_menuBars[menuIndex].items.clear();
        }
    }

    void MenuSystem::SetMenuItemText(const std::string& menuName, const std::string& itemId, const std::string& text)
    {
        auto item = FindMenuItem(menuName, itemId);
        if (item) {
            item->text = text;
        }
    }

    void MenuSystem::SetMenuItemShortcut(const std::string& menuName, const std::string& itemId, const std::string& shortcut)
    {
        auto item = FindMenuItem(menuName, itemId);
        if (item) {
            item->shortcut = shortcut;
        }
    }

    void MenuSystem::SetMenuItemState(const std::string& menuName, const std::string& itemId, MenuItemState state)
    {
        auto item = FindMenuItem(menuName, itemId);
        if (item) {
            item->state = state;
        }
    }

    void MenuSystem::SetMenuItemVisible(const std::string& menuName, const std::string& itemId, bool visible)
    {
        auto item = FindMenuItem(menuName, itemId);
        if (item) {
            item->visible = visible;
        }
    }

    void MenuSystem::SetMenuItemCallback(const std::string& menuName, const std::string& itemId, std::function<void()> callback)
    {
        auto item = FindMenuItem(menuName, itemId);
        if (item) {
            item->callback = callback;
        }
    }

    void MenuSystem::ShowContextMenu(int x, int y, const std::string& contextMenuId)
    {
        auto it = m_contextMenus.find(contextMenuId);
        if (it != m_contextMenus.end()) {
            m_contextMenuVisible = true;
            m_activeContextMenu = contextMenuId;
            m_contextMenuX = x;
            m_contextMenuY = y;
            CalculateContextMenuSize(contextMenuId, m_contextMenuWidth, m_contextMenuHeight);
        }
    }

    void MenuSystem::HideContextMenu()
    {
        m_contextMenuVisible = false;
        m_activeContextMenu.clear();
    }

    void MenuSystem::AddContextMenu(const std::string& contextMenuId, const std::string& name)
    {
        (void)name; // Suppress unused parameter warning
        m_contextMenus[contextMenuId] = std::vector<std::shared_ptr<MenuItem>>();
    }

    void MenuSystem::AddContextMenuItem(const std::string& contextMenuId, const std::string& itemId, 
                                       const std::string& text, const std::string& shortcut, std::function<void()> callback)
    {
        auto it = m_contextMenus.find(contextMenuId);
        if (it != m_contextMenus.end()) {
            auto item = std::make_shared<MenuItem>();
            item->id = itemId;
            item->text = text;
            item->shortcut = shortcut;
            item->callback = callback;
            it->second.push_back(item);
        }
    }

    void MenuSystem::ExecuteMenuItem(const std::string& menuName, const std::string& itemId)
    {
        auto item = FindMenuItem(menuName, itemId);
        if (item && item->callback) {
            item->callback();
        }
    }

    void MenuSystem::ExecuteContextMenuItem(const std::string& contextMenuId, const std::string& itemId)
    {
        auto item = FindContextMenuItem(contextMenuId, itemId);
        if (item && item->callback) {
            item->callback();
        }
    }

    void MenuSystem::RefreshMenus()
    {
        // Refresh menu states based on current application state
        // This would be called when the application state changes
    }

    void MenuSystem::ProcessAccelerator(WPARAM wParam, LPARAM lParam)
    {
        (void)lParam; // Suppress unused parameter warning
        // Check for Ctrl+key combinations
        bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
        bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        
        std::string shortcut;
        if (ctrlPressed) shortcut += "Ctrl+";
        if (altPressed) shortcut += "Alt+";
        if (shiftPressed) shortcut += "Shift+";
        
        // Add the key
        if (wParam >= 'A' && wParam <= 'Z') {
            shortcut += static_cast<char>(wParam);
        } else if (wParam >= VK_F1 && wParam <= VK_F12) {
            shortcut += "F" + std::to_string(wParam - VK_F1 + 1);
        } else {
            switch (wParam) {
                case VK_RETURN: shortcut += "Enter"; break;
                case VK_ESCAPE: shortcut += "Escape"; break;
                case VK_SPACE: shortcut += "Space"; break;
                case VK_TAB: shortcut += "Tab"; break;
                case VK_DELETE: shortcut += "Delete"; break;
                case VK_INSERT: shortcut += "Insert"; break;
                case VK_HOME: shortcut += "Home"; break;
                case VK_END: shortcut += "End"; break;
                case VK_PRIOR: shortcut += "PageUp"; break;
                case VK_NEXT: shortcut += "PageDown"; break;
                case VK_LEFT: shortcut += "Left"; break;
                case VK_RIGHT: shortcut += "Right"; break;
                case VK_UP: shortcut += "Up"; break;
                case VK_DOWN: shortcut += "Down"; break;
            }
        }
        
        auto it = m_accelerators.find(shortcut);
        if (it != m_accelerators.end()) {
            it->second();
        }
    }

    void MenuSystem::RegisterAccelerator(const std::string& shortcut, std::function<void()> callback)
    {
        m_accelerators[shortcut] = callback;
    }

    void MenuSystem::UnregisterAccelerator(const std::string& shortcut)
    {
        m_accelerators.erase(shortcut);
    }

    std::shared_ptr<MenuSystem::MenuItem> MenuSystem::GetMenuItem(const std::string& menuName, const std::string& itemId)
    {
        return FindMenuItem(menuName, itemId);
    }

    std::shared_ptr<MenuSystem::MenuItem> MenuSystem::GetContextMenuItem(const std::string& contextMenuId, const std::string& itemId)
    {
        return FindContextMenuItem(contextMenuId, itemId);
    }

    void MenuSystem::CreateDefaultMenus()
    {
        CreateFileMenu();
        CreateEditMenu();
        CreateViewMenu();
        CreateToolsMenu();
        CreateHelpMenu();
    }

    void MenuSystem::CreateFileMenu()
    {
        AddMenuBar("File");
        AddMenuItem("File", "new", "New", "Ctrl+N", [this]() { OnFileNew(); });
        AddMenuItem("File", "open", "Open", "Ctrl+O", [this]() { OnFileOpen(); });
        AddMenuItem("File", "save", "Save", "Ctrl+S", [this]() { OnFileSave(); });
        AddMenuItem("File", "saveAs", "Save As", "Ctrl+Shift+S", [this]() { OnFileSaveAs(); });
        AddSeparator("File", "saveAs");
        AddMenuItem("File", "export", "Export", "Ctrl+E", [this]() { OnFileExport(); });
        AddSeparator("File", "export");
        AddMenuItem("File", "exit", "Exit", "Alt+F4", [this]() { OnFileExit(); });
    }

    void MenuSystem::CreateEditMenu()
    {
        AddMenuBar("Edit");
        AddMenuItem("Edit", "undo", "Undo", "Ctrl+Z", [this]() { OnEditUndo(); });
        AddMenuItem("Edit", "redo", "Redo", "Ctrl+Y", [this]() { OnEditRedo(); });
        AddSeparator("Edit", "redo");
        AddMenuItem("Edit", "cut", "Cut", "Ctrl+X", [this]() { OnEditCut(); });
        AddMenuItem("Edit", "copy", "Copy", "Ctrl+C", [this]() { OnEditCopy(); });
        AddMenuItem("Edit", "paste", "Paste", "Ctrl+V", [this]() { OnEditPaste(); });
        AddMenuItem("Edit", "delete", "Delete", "Delete", [this]() { OnEditDelete(); });
        AddSeparator("Edit", "delete");
        AddMenuItem("Edit", "selectAll", "Select All", "Ctrl+A", [this]() { OnEditSelectAll(); });
    }

    void MenuSystem::CreateViewMenu()
    {
        AddMenuBar("View");
        AddMenuItem("View", "wireframe", "Wireframe", "F1", [this]() { OnViewWireframe(); });
        AddMenuItem("View", "solid", "Solid", "F2", [this]() { OnViewSolid(); });
        AddMenuItem("View", "textured", "Textured", "F3", [this]() { OnViewTextured(); });
        AddSeparator("View", "textured");
        AddMenuItem("View", "fullscreen", "Fullscreen", "F11", [this]() { OnViewFullscreen(); });
        AddMenuItem("View", "resetCamera", "Reset Camera", "F5", [this]() { OnViewResetCamera(); });
    }

    void MenuSystem::CreateToolsMenu()
    {
        AddMenuBar("Tools");
        AddMenuItem("Tools", "preferences", "Preferences", "Ctrl+Comma", [this]() { OnToolsPreferences(); });
        AddMenuItem("Tools", "assetManager", "Asset Manager", "Ctrl+Shift+A", [this]() { OnToolsAssetManager(); });
        AddMenuItem("Tools", "performanceMonitor", "Performance Monitor", "Ctrl+Shift+P", [this]() { OnToolsPerformanceMonitor(); });
    }

    void MenuSystem::CreateHelpMenu()
    {
        AddMenuBar("Help");
        AddMenuItem("Help", "about", "About", "", [this]() { OnHelpAbout(); });
        AddMenuItem("Help", "documentation", "Documentation", "F1", [this]() { OnHelpDocumentation(); });
        AddMenuItem("Help", "keyboardShortcuts", "Keyboard Shortcuts", "Ctrl+Shift+H", [this]() { OnHelpKeyboardShortcuts(); });
    }

    void MenuSystem::RenderMenuBar(HDC hdc)
    {
        int currentX = m_x + 5;
        
        for (const auto& menuBar : m_menuBars) {
            if (!menuBar.visible) continue;
            
            // Calculate menu width
            SIZE textSize;
            std::wstring wideText(menuBar.name.begin(), menuBar.name.end());
            GetTextExtentPoint32(hdc, wideText.c_str(), static_cast<int>(wideText.length()), &textSize);
            int menuWidth = textSize.cx + 20;
            
            // Draw menu bar item
            RenderMenuBarItem(hdc, menuBar.name, currentX, m_y, menuWidth, m_menuBarHeight);
            
            currentX += menuWidth;
        }
    }

    void MenuSystem::RenderMenuBarItem(HDC hdc, const std::string& menuName, int x, int y, int width, int height)
    {
        // Draw menu item background
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT menuRect = { x, y, x + width, y + height };
        FillRect(hdc, &menuRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw menu item text
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT textRect = { x + 10, y, x + width - 10, y + height };
        std::wstring wideText(menuName.begin(), menuName.end());
        DrawText(hdc, wideText.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER);
        
        // Draw dropdown arrow
        std::wstring arrow = L"▼";
        RECT arrowRect = { x + width - 15, y, x + width - 5, y + height };
        DrawText(hdc, arrow.c_str(), -1, &arrowRect, DT_CENTER | DT_VCENTER);
    }

    void MenuSystem::RenderDropdownMenu(HDC hdc, const std::string& menuName, int x, int y)
    {
        auto it = m_menuBarIndices.find(menuName);
        if (it == m_menuBarIndices.end()) return;
        
        int menuIndex = it->second;
        const auto& menuBar = m_menuBars[menuIndex];
        
        // Draw menu background
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT menuRect = { x, y, x + m_menuWidth, y + m_menuHeight };
        FillRect(hdc, &menuRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw menu border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, x, y, NULL);
        LineTo(hdc, x + m_menuWidth, y);
        LineTo(hdc, x + m_menuWidth, y + m_menuHeight);
        LineTo(hdc, x, y + m_menuHeight);
        LineTo(hdc, x, y);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw menu items
        int currentY = y + 5;
        for (size_t i = 0; i < menuBar.items.size(); ++i) {
            if (!menuBar.items[i]->visible) continue;
            
            bool isHighlighted = (static_cast<int>(i) == m_activeMenuItem);
            RenderMenuItem(hdc, menuBar.items[i], x + 5, currentY, m_menuWidth - 10, m_menuItemHeight, isHighlighted);
            currentY += m_menuItemHeight + 2;
        }
    }

    void MenuSystem::RenderMenuItem(HDC hdc, std::shared_ptr<MenuItem> item, int x, int y, int width, int height, bool isHighlighted)
    {
        if (item->type == MenuItemType::Separator) {
            // Draw separator line
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
            HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
            MoveToEx(hdc, x, y + height / 2, NULL);
            LineTo(hdc, x + width, y + height / 2);
            ::SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            return;
        }
        
        // Draw highlight background
        if (isHighlighted) {
            HBRUSH hBrush = CreateSolidBrush(RGB(80, 120, 200));
            RECT highlightRect = { x, y, x + width, y + height };
            FillRect(hdc, &highlightRect, hBrush);
            DeleteObject(hBrush);
        }
        
        // Draw menu item text
        COLORREF textColor = (item->state == MenuItemState::Disabled) ? RGB(128, 128, 128) : RGB(255, 255, 255);
        ::SetTextColor(hdc, textColor);
        SetBkMode(hdc, TRANSPARENT);
        
        RECT textRect = { x + 5, y, x + width - MENU_ITEM_SHORTCUT_WIDTH, y + height };
        std::wstring wideText(item->text.begin(), item->text.end());
        DrawText(hdc, wideText.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
        
        // Draw shortcut
        if (!item->shortcut.empty()) {
            RECT shortcutRect = { x + width - MENU_ITEM_SHORTCUT_WIDTH, y, x + width - 5, y + height };
            std::wstring wideShortcut(item->shortcut.begin(), item->shortcut.end());
            DrawText(hdc, wideShortcut.c_str(), -1, &shortcutRect, DT_RIGHT | DT_VCENTER);
        }
        
        // Draw checkbox or radio button
        if (item->type == MenuItemType::Checkbox || item->type == MenuItemType::Radio) {
            RECT checkRect = { x + 5, y + 2, x + 15, y + height - 2 };
            if (item->state == MenuItemState::Checked) {
                std::wstring check = L"✓";
                DrawText(hdc, check.c_str(), -1, &checkRect, DT_CENTER | DT_VCENTER);
            }
        }
    }

    void MenuSystem::RenderContextMenu(HDC hdc)
    {
        auto it = m_contextMenus.find(m_activeContextMenu);
        if (it == m_contextMenus.end()) return;
        
        const auto& contextMenu = it->second;
        
        // Draw context menu background
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT menuRect = { m_contextMenuX, m_contextMenuY, m_contextMenuX + m_contextMenuWidth, m_contextMenuY + m_contextMenuHeight };
        FillRect(hdc, &menuRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw context menu border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, m_contextMenuX, m_contextMenuY, NULL);
        LineTo(hdc, m_contextMenuX + m_contextMenuWidth, m_contextMenuY);
        LineTo(hdc, m_contextMenuX + m_contextMenuWidth, m_contextMenuY + m_contextMenuHeight);
        LineTo(hdc, m_contextMenuX, m_contextMenuY + m_contextMenuHeight);
        LineTo(hdc, m_contextMenuX, m_contextMenuY);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw context menu items
        int currentY = m_contextMenuY + 5;
        for (const auto& item : contextMenu) {
            if (!item->visible) continue;
            
            RenderMenuItem(hdc, item, m_contextMenuX + 5, currentY, m_contextMenuWidth - 10, m_menuItemHeight, false);
            currentY += m_menuItemHeight + 2;
        }
    }

    void MenuSystem::HandleMenuBarClick(int x, int y)
    {
        (void)y; // Suppress unused parameter warning
        int currentX = m_x + 5;
        
        for (const auto& menuBar : m_menuBars) {
            if (!menuBar.visible) continue;
            
            // Calculate menu width
            SIZE textSize;
            std::wstring wideText(menuBar.name.begin(), menuBar.name.end());
            HDC hdc = GetDC(nullptr);
            GetTextExtentPoint32(hdc, wideText.c_str(), static_cast<int>(wideText.length()), &textSize);
            ReleaseDC(nullptr, hdc);
            int menuWidth = textSize.cx + 20;
            
            if (x >= currentX && x <= currentX + menuWidth) {
                OpenMenu(menuBar.name, currentX, m_y + m_menuBarHeight);
                break;
            }
            
            currentX += menuWidth;
        }
    }

    void MenuSystem::HandleMenuItemClick(int x, int y)
    {
        (void)x; // Suppress unused parameter warning
        auto it = m_menuBarIndices.find(m_activeMenu);
        if (it == m_menuBarIndices.end()) return;
        
        int menuIndex = it->second;
        const auto& menuBar = m_menuBars[menuIndex];
        
        int currentY = m_menuY + 5;
        for (size_t i = 0; i < menuBar.items.size(); ++i) {
            if (!menuBar.items[i]->visible) continue;
            
            if (y >= currentY && y <= currentY + m_menuItemHeight) {
                if (menuBar.items[i]->callback) {
                    menuBar.items[i]->callback();
                }
                CloseMenu();
                break;
            }
            
            currentY += m_menuItemHeight + 2;
        }
    }

    void MenuSystem::HandleContextMenuClick(int x, int y)
    {
        (void)x; // Suppress unused parameter warning
        auto it = m_contextMenus.find(m_activeContextMenu);
        if (it == m_contextMenus.end()) return;
        
        const auto& contextMenu = it->second;
        
        int currentY = m_contextMenuY + 5;
        for (const auto& item : contextMenu) {
            if (!item->visible) continue;
            
            if (y >= currentY && y <= currentY + m_menuItemHeight) {
                if (item->callback) {
                    item->callback();
                }
                CloseContextMenu();
                break;
            }
            
            currentY += m_menuItemHeight + 2;
        }
    }

    std::shared_ptr<MenuSystem::MenuItem> MenuSystem::FindMenuItem(const std::string& menuName, const std::string& itemId)
    {
        auto it = m_menuBarIndices.find(menuName);
        if (it != m_menuBarIndices.end()) {
            int menuIndex = it->second;
            const auto& items = m_menuBars[menuIndex].items;
            for (const auto& item : items) {
                if (item->id == itemId) {
                    return item;
                }
            }
        }
        return nullptr;
    }

    std::shared_ptr<MenuSystem::MenuItem> MenuSystem::FindContextMenuItem(const std::string& contextMenuId, const std::string& itemId)
    {
        auto it = m_contextMenus.find(contextMenuId);
        if (it != m_contextMenus.end()) {
            const auto& items = it->second;
            for (const auto& item : items) {
                if (item->id == itemId) {
                    return item;
                }
            }
        }
        return nullptr;
    }

    void MenuSystem::OpenMenu(const std::string& menuName, int x, int y)
    {
        m_activeMenu = menuName;
        m_menuX = x;
        m_menuY = y;
        m_menuOpen = true;
        m_activeMenuItem = 0;
        
        CalculateMenuSize(menuName, m_menuWidth, m_menuHeight);
    }

    void MenuSystem::CloseMenu()
    {
        m_menuOpen = false;
        m_activeMenu.clear();
        m_activeMenuItem = -1;
    }

    void MenuSystem::CloseContextMenu()
    {
        m_contextMenuVisible = false;
        m_activeContextMenu.clear();
    }

    bool MenuSystem::IsPointInMenu(int x, int y)
    {
        return x >= m_menuX && x <= m_menuX + m_menuWidth && y >= m_menuY && y <= m_menuY + m_menuHeight;
    }

    bool MenuSystem::IsPointInContextMenu(int x, int y)
    {
        return x >= m_contextMenuX && x <= m_contextMenuX + m_contextMenuWidth && 
               y >= m_contextMenuY && y <= m_contextMenuY + m_contextMenuHeight;
    }

    bool MenuSystem::IsPointInMenuItem(int x, int y, int itemX, int itemY, int itemWidth, int itemHeight)
    {
        return x >= itemX && x <= itemX + itemWidth && y >= itemY && y <= itemY + itemHeight;
    }

    void MenuSystem::CalculateMenuSize(const std::string& menuName, int& width, int& height)
    {
        auto it = m_menuBarIndices.find(menuName);
        if (it == m_menuBarIndices.end()) {
            width = height = 0;
            return;
        }
        
        int menuIndex = it->second;
        const auto& menuBar = m_menuBars[menuIndex];
        
        width = 200; // Default width
        height = 10; // Top padding
        
        for (const auto& item : menuBar.items) {
            if (!item->visible) continue;
            
            if (item->type == MenuItemType::Separator) {
                height += 5;
            } else {
                height += m_menuItemHeight + 2;
            }
        }
        
        height += 5; // Bottom padding
    }

    void MenuSystem::CalculateContextMenuSize(const std::string& contextMenuId, int& width, int& height)
    {
        auto it = m_contextMenus.find(contextMenuId);
        if (it == m_contextMenus.end()) {
            width = height = 0;
            return;
        }
        
        const auto& contextMenu = it->second;
        
        width = 200; // Default width
        height = 10; // Top padding
        
        for (const auto& item : contextMenu) {
            if (!item->visible) continue;
            
            if (item->type == MenuItemType::Separator) {
                height += 5;
            } else {
                height += m_menuItemHeight + 2;
            }
        }
        
        height += 5; // Bottom padding
    }

    // Menu item callback implementations
    void MenuSystem::OnFileNew()
    {
        Engine::Core::Logger::Instance().Info("File -> New");
        // TODO: Implement new project
    }

    void MenuSystem::OnFileOpen()
    {
        Engine::Core::Logger::Instance().Info("File -> Open");
        // TODO: Implement open project
    }

    void MenuSystem::OnFileSave()
    {
        Engine::Core::Logger::Instance().Info("File -> Save");
        // TODO: Implement save project
    }

    void MenuSystem::OnFileSaveAs()
    {
        Engine::Core::Logger::Instance().Info("File -> Save As");
        // TODO: Implement save as project
    }

    void MenuSystem::OnFileExport()
    {
        Engine::Core::Logger::Instance().Info("File -> Export");
        // TODO: Implement export project
    }

    void MenuSystem::OnFileExit()
    {
        Engine::Core::Logger::Instance().Info("File -> Exit");
        // TODO: Implement exit application
    }

    void MenuSystem::OnEditUndo()
    {
        Engine::Core::Logger::Instance().Info("Edit -> Undo");
        // TODO: Implement undo
    }

    void MenuSystem::OnEditRedo()
    {
        Engine::Core::Logger::Instance().Info("Edit -> Redo");
        // TODO: Implement redo
    }

    void MenuSystem::OnEditCut()
    {
        Engine::Core::Logger::Instance().Info("Edit -> Cut");
        // TODO: Implement cut
    }

    void MenuSystem::OnEditCopy()
    {
        Engine::Core::Logger::Instance().Info("Edit -> Copy");
        // TODO: Implement copy
    }

    void MenuSystem::OnEditPaste()
    {
        Engine::Core::Logger::Instance().Info("Edit -> Paste");
        // TODO: Implement paste
    }

    void MenuSystem::OnEditDelete()
    {
        Engine::Core::Logger::Instance().Info("Edit -> Delete");
        // TODO: Implement delete
    }

    void MenuSystem::OnEditSelectAll()
    {
        Engine::Core::Logger::Instance().Info("Edit -> Select All");
        // TODO: Implement select all
    }

    void MenuSystem::OnViewWireframe()
    {
        Engine::Core::Logger::Instance().Info("View -> Wireframe");
        // TODO: Implement wireframe view
    }

    void MenuSystem::OnViewSolid()
    {
        Engine::Core::Logger::Instance().Info("View -> Solid");
        // TODO: Implement solid view
    }

    void MenuSystem::OnViewTextured()
    {
        Engine::Core::Logger::Instance().Info("View -> Textured");
        // TODO: Implement textured view
    }

    void MenuSystem::OnViewFullscreen()
    {
        Engine::Core::Logger::Instance().Info("View -> Fullscreen");
        // TODO: Implement fullscreen toggle
    }

    void MenuSystem::OnViewResetCamera()
    {
        Engine::Core::Logger::Instance().Info("View -> Reset Camera");
        // TODO: Implement camera reset
    }

    void MenuSystem::OnToolsPreferences()
    {
        Engine::Core::Logger::Instance().Info("Tools -> Preferences");
        // TODO: Implement preferences dialog
    }

    void MenuSystem::OnToolsAssetManager()
    {
        Engine::Core::Logger::Instance().Info("Tools -> Asset Manager");
        // TODO: Implement asset manager
    }

    void MenuSystem::OnToolsPerformanceMonitor()
    {
        Engine::Core::Logger::Instance().Info("Tools -> Performance Monitor");
        // TODO: Implement performance monitor
    }

    void MenuSystem::OnHelpAbout()
    {
        Engine::Core::Logger::Instance().Info("Help -> About");
        // TODO: Implement about dialog
    }

    void MenuSystem::OnHelpDocumentation()
    {
        Engine::Core::Logger::Instance().Info("Help -> Documentation");
        // TODO: Implement documentation
    }

    void MenuSystem::OnHelpKeyboardShortcuts()
    {
        Engine::Core::Logger::Instance().Info("Help -> Keyboard Shortcuts");
        // TODO: Implement keyboard shortcuts dialog
    }

} // namespace UI
} // namespace Engine
