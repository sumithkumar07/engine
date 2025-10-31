#include "ui/ToolbarSystem.h"
#include "core/Logger.h"
#include "core/Engine.h"
#include "core/SceneManager.h"
#include "core/CommandHistory.h"
#include <algorithm>
#include <commdlg.h> // For file dialogs
#include <filesystem>
#include <iostream>

// Prevent Windows GetObject macro collision
#ifdef GetObject
#undef GetObject
#endif

namespace Engine {
namespace UI {

    ToolbarSystem::ToolbarSystem()
        : UIComponent("ToolbarSystem", 0, 0, 800, 40)
        , m_toolbarHeight(DEFAULT_TOOLBAR_HEIGHT)
        , m_toolSpacing(DEFAULT_TOOL_SPACING)
        , m_groupSpacing(DEFAULT_GROUP_SPACING)
        , m_toolPadding(DEFAULT_TOOL_PADDING)
        , m_dropdownVisible(false)
        , m_dropdownX(0)
        , m_dropdownY(0)
        , m_dropdownWidth(0)
        , m_dropdownHeight(0)
    {
        Engine::Core::Logger::Instance().Debug("ToolbarSystem created");
    }

    ToolbarSystem::~ToolbarSystem()
    {
        Shutdown();
        Engine::Core::Logger::Instance().Debug("ToolbarSystem destroyed");
    }

    bool ToolbarSystem::Initialize()
    {
        CreateDefaultTools();
        Engine::Core::Logger::Instance().Info("ToolbarSystem initialized");
        return true;
    }

    void ToolbarSystem::Shutdown()
    {
        m_toolGroups.clear();
        m_toolGroupIndices.clear();
        m_activeTools.clear();
        m_toolShortcuts.clear();
        
        Engine::Core::Logger::Instance().Info("ToolbarSystem shutdown");
    }

    void ToolbarSystem::Update(float deltaTime)
    {
        (void)deltaTime; // Suppress unused parameter warning
        
        // Update tool states based on application state
        UpdateToolStates();
    }

    void ToolbarSystem::Render(HDC hdc)
    {
        if (!m_visible) {
            return;
        }

        // Draw toolbar background
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT toolbarRect = { m_x, m_y, m_x + m_width, m_y + m_height }; // use full height
        FillRect(hdc, &toolbarRect, hBrush);
        DeleteObject(hBrush);

        // Draw toolbar border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, m_x, m_y, NULL);
        LineTo(hdc, m_x + m_width, m_y);
        LineTo(hdc, m_x + m_width, m_y + m_toolbarHeight);
        LineTo(hdc, m_x, m_y + m_toolbarHeight);
        LineTo(hdc, m_x, m_y);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        // Render tool groups
        RenderToolGroups(hdc);
        
        // Render dropdown if visible
        if (m_dropdownVisible) {
            RenderDropdown(hdc);
        }
    }

    void ToolbarSystem::HandleMouseEvent(int x, int y, int button, bool pressed)
    {
        // DEBUG: Log all clicks
        static int eventCount = 0;
        if (pressed && button == 0) {
            eventCount++;
            if (eventCount <= 10) {
                std::cout << "[ToolbarSystem] HandleMouseEvent #" << eventCount << ": x=" << x << ", y=" << y 
                          << ", visible=" << m_visible << ", toolbar bounds: x=[" << m_x << "," << (m_x + m_width) 
                          << "], y=[" << m_y << "," << (m_y + m_toolbarHeight) << "]" << std::endl;
            }
        }
        
        if (!m_visible) {
            if (pressed && button == 0 && eventCount <= 5) {
                std::cout << "[ToolbarSystem] Rejected: not visible" << std::endl;
            }
            return;
        }

        // Check if click is within toolbar bounds
        if (x >= m_x && x <= m_x + m_width && y >= m_y && y <= m_y + m_toolbarHeight) {
            if (button == 0 && pressed) { // Left mouse button press
                if (eventCount <= 10) {
                    std::cout << "[ToolbarSystem] Click ACCEPTED, calling HandleToolClick(" << x << "," << y << ")" << std::endl;
                }
                HandleToolClick(x, y);
            }
            return; // Handled
        }
        
        // Check if click is within dropdown
        if (m_dropdownVisible && x >= m_dropdownX && x <= m_dropdownX + m_dropdownWidth && 
            y >= m_dropdownY && y <= m_dropdownY + m_dropdownHeight) {
            if (button == 0 && pressed) {
                HandleDropdownClick(x, y);
            }
        }
        // Hide dropdown if clicking outside
        else if (button == 0 && pressed) {
            HideDropdown();
        }
    }

    void ToolbarSystem::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (!m_visible) {
            return;
        }

        if (message == WM_KEYDOWN) {
            ProcessToolShortcut(wParam, lParam);
        }
    }

    void ToolbarSystem::AddToolGroup(const std::string& name, bool collapsible)
    {
        ToolGroup newGroup;
        newGroup.name = name;
        newGroup.collapsible = collapsible;
        m_toolGroups.push_back(newGroup);
        m_toolGroupIndices[name] = static_cast<int>(m_toolGroups.size()) - 1;
    }

    void ToolbarSystem::RemoveToolGroup(const std::string& name)
    {
        auto it = m_toolGroupIndices.find(name);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            m_toolGroups.erase(m_toolGroups.begin() + groupIndex);
            m_toolGroupIndices.erase(it);
            
            // Update indices
            for (auto& pair : m_toolGroupIndices) {
                if (pair.second > groupIndex) {
                    pair.second--;
                }
            }
        }
    }

    void ToolbarSystem::SetToolGroupVisible(const std::string& name, bool visible)
    {
        auto it = m_toolGroupIndices.find(name);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            m_toolGroups[groupIndex].visible = visible;
        }
    }

    void ToolbarSystem::SetToolGroupCollapsed(const std::string& name, bool collapsed)
    {
        auto it = m_toolGroupIndices.find(name);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            m_toolGroups[groupIndex].collapsed = collapsed;
        }
    }

    void ToolbarSystem::AddTool(const std::string& groupName, const std::string& toolId, const std::string& text, 
                               const std::string& tooltip, const std::string& shortcut, std::function<void()> callback)
    {
        auto it = m_toolGroupIndices.find(groupName);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            auto tool = std::make_shared<Tool>();
            tool->id = toolId;
            tool->text = text;
            tool->tooltip = tooltip;
            tool->shortcut = shortcut;
            tool->callback = callback;
            tool->width = static_cast<int>(text.length() * 8 + 16); // Calculate width based on text length
            m_toolGroups[groupIndex].tools.push_back(tool);
            
            // Register shortcut if provided
            if (!shortcut.empty()) {
                m_toolShortcuts[shortcut] = std::make_pair(groupName, toolId);
            }
        }
    }

    void ToolbarSystem::AddToggleTool(const std::string& groupName, const std::string& toolId, const std::string& text, 
                                     const std::string& tooltip, const std::string& shortcut, std::function<void()> callback)
    {
        auto it = m_toolGroupIndices.find(groupName);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            auto tool = std::make_shared<Tool>();
            tool->id = toolId;
            tool->text = text;
            tool->tooltip = tooltip;
            tool->shortcut = shortcut;
            tool->type = ToolType::Toggle;
            tool->callback = callback;
            tool->width = static_cast<int>(text.length() * 8 + 16); // Calculate width based on text length
            m_toolGroups[groupIndex].tools.push_back(tool);
            
            // Register shortcut if provided
            if (!shortcut.empty()) {
                m_toolShortcuts[shortcut] = std::make_pair(groupName, toolId);
            }
        }
    }

    void ToolbarSystem::AddDropdownTool(const std::string& groupName, const std::string& toolId, const std::string& text, 
                                       const std::vector<std::string>& items, const std::string& tooltip, std::function<void(int)> callback)
    {
        auto it = m_toolGroupIndices.find(groupName);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            auto tool = std::make_shared<Tool>();
            tool->id = toolId;
            tool->text = text;
            tool->tooltip = tooltip;
            tool->type = ToolType::Dropdown;
            tool->dropdownItems = items;
            tool->dropdownCallback = callback;
            tool->width = 120; // Wider for dropdown
            m_toolGroups[groupIndex].tools.push_back(tool);
        }
    }

    void ToolbarSystem::AddSeparator(const std::string& groupName, const std::string& afterToolId)
    {
        (void)afterToolId; // Suppress unused parameter warning
        auto it = m_toolGroupIndices.find(groupName);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            auto separator = std::make_shared<Tool>();
            separator->id = "separator_" + std::to_string(m_toolGroups[groupIndex].tools.size());
            separator->text = "|";
            separator->type = ToolType::Separator;
            separator->width = 2;
            m_toolGroups[groupIndex].tools.push_back(separator);
        }
    }

    void ToolbarSystem::AddSpacer(const std::string& groupName, int width)
    {
        auto it = m_toolGroupIndices.find(groupName);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            auto spacer = std::make_shared<Tool>();
            spacer->id = "spacer_" + std::to_string(m_toolGroups[groupIndex].tools.size());
            spacer->text = "";
            spacer->type = ToolType::Spacer;
            spacer->width = width;
            m_toolGroups[groupIndex].tools.push_back(spacer);
        }
    }

    void ToolbarSystem::RemoveTool(const std::string& groupName, const std::string& toolId)
    {
        auto it = m_toolGroupIndices.find(groupName);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            auto& tools = m_toolGroups[groupIndex].tools;
            tools.erase(
                std::remove_if(tools.begin(), tools.end(),
                    [&toolId](const std::shared_ptr<Tool>& tool) { return tool->id == toolId; }),
                tools.end());
        }
    }

    void ToolbarSystem::SetToolText(const std::string& groupName, const std::string& toolId, const std::string& text)
    {
        auto tool = FindTool(groupName, toolId);
        if (tool) {
            tool->text = text;
        }
    }

    void ToolbarSystem::SetToolTooltip(const std::string& groupName, const std::string& toolId, const std::string& tooltip)
    {
        auto tool = FindTool(groupName, toolId);
        if (tool) {
            tool->tooltip = tooltip;
        }
    }

    void ToolbarSystem::SetToolState(const std::string& groupName, const std::string& toolId, ToolState state)
    {
        auto tool = FindTool(groupName, toolId);
        if (tool) {
            tool->state = state;
        }
    }

    void ToolbarSystem::SetToolVisible(const std::string& groupName, const std::string& toolId, bool visible)
    {
        auto tool = FindTool(groupName, toolId);
        if (tool) {
            tool->visible = visible;
        }
    }

    void ToolbarSystem::SetToolCallback(const std::string& groupName, const std::string& toolId, std::function<void()> callback)
    {
        auto tool = FindTool(groupName, toolId);
        if (tool) {
            tool->callback = callback;
        }
    }

    void ToolbarSystem::ExecuteTool(const std::string& groupName, const std::string& toolId)
    {
        std::cout << "[ToolbarSystem] ExecuteTool: group='" << groupName << "', toolId='" << toolId << "'" << std::endl;
        auto tool = FindTool(groupName, toolId);
        if (tool) {
            std::cout << "[ToolbarSystem] ✓ Tool found: text='" << tool->text << "', hasCallback=" << (tool->callback != nullptr) << std::endl;
            if (tool->callback) {
                std::cout << "[ToolbarSystem] ✓ Executing callback..." << std::endl;
                tool->callback();
                std::cout << "[ToolbarSystem] ✓ Callback executed" << std::endl;
            } else {
                std::cout << "[ToolbarSystem] ✗ Tool has no callback registered!" << std::endl;
            }
        } else {
            std::cout << "[ToolbarSystem] ✗ Tool not found: group='" << groupName << "', toolId='" << toolId << "'" << std::endl;
        }
    }

    void ToolbarSystem::ToggleTool(const std::string& groupName, const std::string& toolId)
    {
        std::cout << "[ToolbarSystem] ToggleTool: group='" << groupName << "', toolId='" << toolId << "'" << std::endl;
        auto tool = FindTool(groupName, toolId);
        if (tool) {
            std::cout << "[ToolbarSystem] ✓ Tool found: text='" << tool->text << "', type=" << static_cast<int>(tool->type) 
                      << ", currentState=" << static_cast<int>(tool->state) << std::endl;
            if (tool->type == ToolType::Toggle) {
                if (tool->state == ToolState::Active) {
                    tool->state = ToolState::Normal;
                    std::cout << "[ToolbarSystem]   Toggled from ACTIVE -> NORMAL" << std::endl;
                } else {
                    tool->state = ToolState::Active;
                    std::cout << "[ToolbarSystem]   Toggled from NORMAL -> ACTIVE" << std::endl;
                }
                
                if (tool->callback) {
                    std::cout << "[ToolbarSystem] ✓ Executing toggle callback..." << std::endl;
                    tool->callback();
                    std::cout << "[ToolbarSystem] ✓ Toggle callback executed" << std::endl;
                } else {
                    std::cout << "[ToolbarSystem] ✗ Tool has no callback registered!" << std::endl;
                }
            } else {
                std::cout << "[ToolbarSystem] ✗ Tool is not a Toggle type (type=" << static_cast<int>(tool->type) << ")" << std::endl;
            }
        } else {
            std::cout << "[ToolbarSystem] ✗ Tool not found!" << std::endl;
        }
    }

    void ToolbarSystem::SetActiveTool(const std::string& groupName, const std::string& toolId)
    {
        // Clear other active tools in the same group
        auto it = m_toolGroupIndices.find(groupName);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            for (auto& tool : m_toolGroups[groupIndex].tools) {
                if (tool->type == ToolType::Toggle) {
                    tool->state = ToolState::Normal;
                }
            }
        }
        
        // Set the new active tool
        auto tool = FindTool(groupName, toolId);
        if (tool && tool->type == ToolType::Toggle) {
            tool->state = ToolState::Active;
            m_activeToolGroup = groupName;
            m_activeTool = toolId;
        }
    }

    void ToolbarSystem::ClearActiveTools()
    {
        for (auto& group : m_toolGroups) {
            for (auto& tool : group.tools) {
                if (tool->type == ToolType::Toggle) {
                    tool->state = ToolState::Normal;
                }
            }
        }
        m_activeToolGroup.clear();
        m_activeTool.clear();
    }

    void ToolbarSystem::SetDropdownItems(const std::string& groupName, const std::string& toolId, const std::vector<std::string>& items)
    {
        auto tool = FindTool(groupName, toolId);
        if (tool && tool->type == ToolType::Dropdown) {
            tool->dropdownItems = items;
        }
    }

    void ToolbarSystem::SetDropdownSelection(const std::string& groupName, const std::string& toolId, int index)
    {
        auto tool = FindTool(groupName, toolId);
        if (tool && tool->type == ToolType::Dropdown && index >= 0 && index < static_cast<int>(tool->dropdownItems.size())) {
            tool->selectedIndex = index;
        }
    }

    void ToolbarSystem::ShowDropdown(const std::string& groupName, const std::string& toolId)
    {
        auto tool = FindTool(groupName, toolId);
        if (tool && tool->type == ToolType::Dropdown) {
            m_dropdownVisible = true;
            m_dropdownGroup = groupName;
            m_dropdownTool = toolId;
            m_dropdownX = m_x + 100; // TODO: Calculate proper position
            m_dropdownY = m_y + m_toolbarHeight;
            m_dropdownWidth = tool->width;
            m_dropdownHeight = static_cast<int>(tool->dropdownItems.size()) * DROPDOWN_ITEM_HEIGHT + 10;
        }
    }

    void ToolbarSystem::HideDropdown()
    {
        m_dropdownVisible = false;
        m_dropdownGroup.clear();
        m_dropdownTool.clear();
    }

    void ToolbarSystem::SetToolbarHeight(int height)
    {
        m_toolbarHeight = height;
        m_height = height;
    }

    void ToolbarSystem::SetToolSpacing(int spacing)
    {
        m_toolSpacing = spacing;
    }

    void ToolbarSystem::SetGroupSpacing(int spacing)
    {
        m_groupSpacing = spacing;
    }

    void ToolbarSystem::RefreshLayout()
    {
        // Recalculate toolbar layout
        // This would be called when tools are added/removed or when toolbar size changes
    }

    void ToolbarSystem::ProcessToolShortcut(WPARAM wParam, LPARAM lParam)
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
        
        auto it = m_toolShortcuts.find(shortcut);
        if (it != m_toolShortcuts.end()) {
            ExecuteTool(it->second.first, it->second.second);
        }
    }

    void ToolbarSystem::RegisterToolShortcut(const std::string& shortcut, const std::string& groupName, const std::string& toolId)
    {
        m_toolShortcuts[shortcut] = std::make_pair(groupName, toolId);
    }

    void ToolbarSystem::UnregisterToolShortcut(const std::string& shortcut)
    {
        m_toolShortcuts.erase(shortcut);
    }

    std::shared_ptr<ToolbarSystem::Tool> ToolbarSystem::GetTool(const std::string& groupName, const std::string& toolId)
    {
        return FindTool(groupName, toolId);
    }

    std::vector<std::shared_ptr<ToolbarSystem::Tool>> ToolbarSystem::GetActiveTools() const
    {
        return m_activeTools;
    }

    bool ToolbarSystem::IsToolActive(const std::string& groupName, const std::string& toolId) const
    {
        return m_activeToolGroup == groupName && m_activeTool == toolId;
    }

    void ToolbarSystem::CreateDefaultTools()
    {
        CreateFileTools();
        CreateEditTools();
        CreateViewTools();
        CreateTransformTools();
        CreateRenderTools();
    }

    void ToolbarSystem::CreateFileTools()
    {
        AddToolGroup("File");
        AddTool("File", "new", "New", "Create new project", "Ctrl+N", [this]() { OnFileNew(); });
        AddTool("File", "open", "Open", "Open project", "Ctrl+O", [this]() { OnFileOpen(); });
        AddTool("File", "save", "Save", "Save project", "Ctrl+S", [this]() { OnFileSave(); });
        AddSeparator("File", "save");
        AddTool("File", "export", "Export", "Export project", "Ctrl+E", [this]() { OnFileExport(); });
    }

    void ToolbarSystem::CreateEditTools()
    {
        AddToolGroup("Edit");
        AddTool("Edit", "undo", "Undo", "Undo last action", "Ctrl+Z", [this]() { OnEditUndo(); });
        AddTool("Edit", "redo", "Redo", "Redo last action", "Ctrl+Y", [this]() { OnEditRedo(); });
        AddSeparator("Edit", "redo");
        AddTool("Edit", "cut", "Cut", "Cut selection", "Ctrl+X", [this]() { OnEditCut(); });
        AddTool("Edit", "copy", "Copy", "Copy selection", "Ctrl+C", [this]() { OnEditCopy(); });
        AddTool("Edit", "paste", "Paste", "Paste selection", "Ctrl+V", [this]() { OnEditPaste(); });
        AddTool("Edit", "delete", "Delete", "Delete selection", "Delete", [this]() { OnEditDelete(); });
    }

    void ToolbarSystem::CreateViewTools()
    {
        AddToolGroup("View");
        AddToggleTool("View", "wireframe", "Wireframe", "Toggle wireframe view", "F1", [this]() { OnViewWireframe(); });
        AddToggleTool("View", "solid", "Solid", "Toggle solid view", "F2", [this]() { OnViewSolid(); });
        AddToggleTool("View", "textured", "Textured", "Toggle textured view", "F3", [this]() { OnViewTextured(); });
        AddSeparator("View", "textured");
        AddTool("View", "fullscreen", "Fullscreen", "Toggle fullscreen", "F11", [this]() { OnViewFullscreen(); });
        AddTool("View", "resetCamera", "Reset Camera", "Reset camera view", "F5", [this]() { OnViewResetCamera(); });
    }

    void ToolbarSystem::CreateTransformTools()
    {
        AddToolGroup("Transform");
        AddToggleTool("Transform", "select", "Select", "Selection tool", "Q", [this]() { OnTransformSelect(); });
        AddToggleTool("Transform", "move", "Move", "Move tool", "W", [this]() { OnTransformMove(); });
        AddToggleTool("Transform", "rotate", "Rotate", "Rotate tool", "E", [this]() { OnTransformRotate(); });
        AddToggleTool("Transform", "scale", "Scale", "Scale tool", "R", [this]() { OnTransformScale(); });
    }

    void ToolbarSystem::CreateRenderTools()
    {
        AddToolGroup("Render");
        AddTool("Render", "play", "Play", "Play animation", "Space", [this]() { OnRenderPlay(); });
        AddTool("Render", "pause", "Pause", "Pause animation", "Space", [this]() { OnRenderPause(); });
        AddTool("Render", "stop", "Stop", "Stop animation", "Shift+Space", [this]() { OnRenderStop(); });
        AddSeparator("Render", "stop");
        AddTool("Render", "frame", "Render Frame", "Render current frame", "F12", [this]() { OnRenderFrame(); });
    }

    void ToolbarSystem::RenderToolGroups(HDC hdc)
    {
        int currentX = m_x + 5;
        int y = m_y + (m_toolbarHeight - TOOL_ICON_SIZE) / 2;
        
        for (auto& group : m_toolGroups) {
            if (!group.visible) continue;
            
            RenderToolGroup(hdc, group, currentX, y);
            currentX += m_groupSpacing;
        }
    }

    void ToolbarSystem::RenderToolGroup(HDC hdc, ToolGroup& group, int& x, int y)
    {
        for (auto& tool : group.tools) {
            if (!tool->visible) continue;
            
            RenderTool(hdc, tool, x, y);
            x += tool->width + m_toolSpacing;
        }
    }

    void ToolbarSystem::RenderTool(HDC hdc, std::shared_ptr<Tool> tool, int x, int y)
    {
        if (tool->type == ToolType::Separator) {
            // Draw separator line
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
            HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
            MoveToEx(hdc, x, y, NULL);
            LineTo(hdc, x, y + TOOL_ICON_SIZE);
            ::SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            return;
        }
        
        if (tool->type == ToolType::Spacer) {
            // Draw nothing for spacer
            return;
        }
        
        // Draw tool background
        COLORREF bgColor = RGB(70, 70, 70);
        if (tool->state == ToolState::Pressed || tool->state == ToolState::Active) {
            bgColor = RGB(100, 150, 255);
        } else if (tool->state == ToolState::Disabled) {
            bgColor = RGB(50, 50, 50);
        }
        
        HBRUSH hBrush = CreateSolidBrush(bgColor);
        RECT toolRect = { x, y, x + tool->width, y + TOOL_ICON_SIZE };
        FillRect(hdc, &toolRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw tool border
        COLORREF borderColor = RGB(100, 100, 100);
        if (tool->state == ToolState::Pressed || tool->state == ToolState::Active) {
            borderColor = RGB(150, 200, 255);
        }
        
        HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, x, y, NULL);
        LineTo(hdc, x + tool->width, y);
        LineTo(hdc, x + tool->width, y + TOOL_ICON_SIZE);
        LineTo(hdc, x, y + TOOL_ICON_SIZE);
        LineTo(hdc, x, y);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw tool text
        COLORREF textColor = (tool->state == ToolState::Disabled) ? RGB(128, 128, 128) : RGB(255, 255, 255);
        ::SetTextColor(hdc, textColor);
        SetBkMode(hdc, TRANSPARENT);
        
        RECT textRect = { x + 2, y, x + tool->width - 2, y + TOOL_ICON_SIZE };
        std::wstring wideText(tool->text.begin(), tool->text.end());
        DrawText(hdc, wideText.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER);
        
        // Draw dropdown arrow for dropdown tools
        if (tool->type == ToolType::Dropdown) {
            std::wstring arrow = L"▼";
            RECT arrowRect = { x + tool->width - 15, y, x + tool->width - 5, y + TOOL_ICON_SIZE };
            DrawText(hdc, arrow.c_str(), -1, &arrowRect, DT_CENTER | DT_VCENTER);
        }
    }

    void ToolbarSystem::RenderDropdown(HDC hdc)
    {
        auto tool = FindTool(m_dropdownGroup, m_dropdownTool);
        if (!tool) return;
        
        // Draw dropdown background
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT dropdownRect = { m_dropdownX, m_dropdownY, m_dropdownX + m_dropdownWidth, m_dropdownY + m_dropdownHeight };
        FillRect(hdc, &dropdownRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw dropdown border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, m_dropdownX, m_dropdownY, NULL);
        LineTo(hdc, m_dropdownX + m_dropdownWidth, m_dropdownY);
        LineTo(hdc, m_dropdownX + m_dropdownWidth, m_dropdownY + m_dropdownHeight);
        LineTo(hdc, m_dropdownX, m_dropdownY + m_dropdownHeight);
        LineTo(hdc, m_dropdownX, m_dropdownY);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw dropdown items
        int currentY = m_dropdownY + 5;
        for (size_t i = 0; i < tool->dropdownItems.size(); ++i) {
            RECT itemRect = { m_dropdownX + 5, currentY, m_dropdownX + m_dropdownWidth - 5, currentY + DROPDOWN_ITEM_HEIGHT };
            
            // Highlight selected item
            if (static_cast<int>(i) == tool->selectedIndex) {
                HBRUSH itemBrush = CreateSolidBrush(RGB(80, 120, 200));
                FillRect(hdc, &itemRect, itemBrush);
                DeleteObject(itemBrush);
            }
            
            // Draw item text
            ::SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            std::wstring wideText(tool->dropdownItems[i].begin(), tool->dropdownItems[i].end());
            DrawText(hdc, wideText.c_str(), -1, &itemRect, DT_LEFT | DT_VCENTER);
            
            currentY += DROPDOWN_ITEM_HEIGHT;
        }
    }

    void ToolbarSystem::RenderTooltip(HDC hdc, const std::string& text, int x, int y)
    {
        if (text.empty()) return;
        
        // Calculate tooltip size
        SIZE textSize;
        std::wstring wideText(text.begin(), text.end());
        GetTextExtentPoint32(hdc, wideText.c_str(), static_cast<int>(wideText.length()), &textSize);
        
        int tooltipWidth = textSize.cx + TOOLTIP_PADDING * 2;
        int tooltipHeight = textSize.cy + TOOLTIP_PADDING * 2;
        
        // Draw tooltip background
        HBRUSH hBrush = CreateSolidBrush(RGB(40, 40, 40));
        RECT tooltipRect = { x, y, x + tooltipWidth, y + tooltipHeight };
        FillRect(hdc, &tooltipRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw tooltip border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, x, y, NULL);
        LineTo(hdc, x + tooltipWidth, y);
        LineTo(hdc, x + tooltipWidth, y + tooltipHeight);
        LineTo(hdc, x, y + tooltipHeight);
        LineTo(hdc, x, y);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw tooltip text
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT textRect = { x + TOOLTIP_PADDING, y + TOOLTIP_PADDING, x + tooltipWidth - TOOLTIP_PADDING, y + tooltipHeight - TOOLTIP_PADDING };
        DrawText(hdc, wideText.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
    }

    void ToolbarSystem::HandleToolClick(int x, int y)
    {
        std::cout << "[ToolbarSystem] HandleToolClick: x=" << x << ", y=" << y << " (local coordinates)" << std::endl;
        std::string groupName, toolId;
        auto tool = GetToolFromPosition(x, y, groupName, toolId);
        if (tool) {
            std::cout << "[ToolbarSystem] ✓ Found tool: '" << toolId << "' in group '" << groupName 
                      << "', type=" << static_cast<int>(tool->type) << std::endl;
            if (tool->type == ToolType::Toggle) {
                std::cout << "[ToolbarSystem] Executing TOGGLE tool: " << groupName << " -> " << toolId << std::endl;
                ToggleTool(groupName, toolId);
            } else if (tool->type == ToolType::Dropdown) {
                std::cout << "[ToolbarSystem] Showing DROPDOWN for tool: " << groupName << " -> " << toolId << std::endl;
                ShowDropdown(groupName, toolId);
            } else {
                std::cout << "[ToolbarSystem] Executing ACTION tool: " << groupName << " -> " << toolId << std::endl;
                ExecuteTool(groupName, toolId);
            }
        } else {
            std::cout << "[ToolbarSystem] ✗ No tool found at position (" << x << "," << y << ")" << std::endl;
        }
    }

    void ToolbarSystem::HandleDropdownClick(int x, int y)
    {
        (void)x; // Suppress unused parameter warning
        auto tool = FindTool(m_dropdownGroup, m_dropdownTool);
        if (!tool) return;
        
        int relativeY = y - m_dropdownY - 5;
        int itemIndex = relativeY / DROPDOWN_ITEM_HEIGHT;
        
        if (itemIndex >= 0 && itemIndex < static_cast<int>(tool->dropdownItems.size())) {
            tool->selectedIndex = itemIndex;
            if (tool->dropdownCallback) {
                tool->dropdownCallback(itemIndex);
            }
            HideDropdown();
        }
    }

    std::shared_ptr<ToolbarSystem::Tool> ToolbarSystem::FindTool(const std::string& groupName, const std::string& toolId)
    {
        auto it = m_toolGroupIndices.find(groupName);
        if (it != m_toolGroupIndices.end()) {
            int groupIndex = it->second;
            const auto& tools = m_toolGroups[groupIndex].tools;
            for (const auto& tool : tools) {
                if (tool->id == toolId) {
                    return tool;
                }
            }
        }
        return nullptr;
    }

    std::shared_ptr<ToolbarSystem::Tool> ToolbarSystem::GetToolFromPosition(int x, int y, std::string& groupName, std::string& toolId)
    {
        int currentX = m_x + 5;
        int toolY = m_y + (m_toolbarHeight - TOOL_ICON_SIZE) / 2;
        
        for (const auto& group : m_toolGroups) {
            if (!group.visible) continue;
            
            for (const auto& tool : group.tools) {
                if (!tool->visible) continue;
                
                if (x >= currentX && x <= currentX + tool->width && 
                    y >= toolY && y <= toolY + TOOL_ICON_SIZE) {
                    groupName = group.name;
                    toolId = tool->id;
                    return tool;
                }
                
                currentX += tool->width + m_toolSpacing;
            }
            currentX += m_groupSpacing;
        }
        
        return nullptr;
    }

    void ToolbarSystem::UpdateToolStates()
    {
        // Update tool states based on current application state
        // This would be called to enable/disable tools based on context
    }

    void ToolbarSystem::UpdateActiveTools()
    {
        m_activeTools.clear();
        for (const auto& group : m_toolGroups) {
            for (const auto& tool : group.tools) {
                if (tool->state == ToolState::Active) {
                    m_activeTools.push_back(tool);
                }
            }
        }
    }

    // Tool callback implementations
    void ToolbarSystem::OnFileNew()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: File -> New");
        MessageBoxA(NULL, "TODO: File -> New action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnFileOpen()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: File -> Open");
        
        // Open file dialog
        OPENFILENAMEA ofn;
        char szFile[260] = {0};
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Scene Files\0*.scene\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = "scenes";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        
        if (GetOpenFileNameA(&ofn)) {
            // Extract scene name from file path
            std::filesystem::path filePath(szFile);
            std::string sceneName = filePath.stem().string();
            
            // Load scene via engine
            extern Engine::Core::Engine* g_engine;
            if (g_engine && g_engine->GetSceneManager()) {
                if (g_engine->GetSceneManager()->LoadScene(sceneName)) {
                    MessageBoxA(NULL, ("Scene loaded: " + sceneName).c_str(), "Load Scene", MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBoxA(NULL, ("Failed to load scene: " + sceneName).c_str(), "Load Scene Error", MB_OK | MB_ICONERROR);
                }
            }
        }
    }

    void ToolbarSystem::OnFileSave()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: File -> Save");
        
        // Get current scene name
        extern Engine::Core::Engine* g_engine;
        if (!g_engine || !g_engine->GetSceneManager()) {
            MessageBoxA(NULL, "No scene manager available", "Save Error", MB_OK | MB_ICONERROR);
            return;
        }
        
        std::string currentScene = g_engine->GetSceneManager()->GetCurrentSceneName();
        if (currentScene.empty()) {
            // No scene loaded, prompt for save as
            OnFileSaveAs();
            return;
        }
        
        // Save current scene
        if (g_engine->GetSceneManager()->SaveScene(currentScene)) {
            MessageBoxA(NULL, ("Scene saved: " + currentScene).c_str(), "Save Scene", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxA(NULL, ("Failed to save scene: " + currentScene).c_str(), "Save Scene Error", MB_OK | MB_ICONERROR);
        }
    }
    
    void ToolbarSystem::OnFileSaveAs()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: File -> Save As");
        
        // Save file dialog
        OPENFILENAMEA ofn;
        char szFile[260] = "Untitled.scene";
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Scene Files\0*.scene\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = "scenes";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = "scene";
        
        if (GetSaveFileNameA(&ofn)) {
            // Extract scene name from file path
            std::filesystem::path filePath(szFile);
            std::string sceneName = filePath.stem().string();
            
            // Save scene via engine
            extern Engine::Core::Engine* g_engine;
            if (g_engine && g_engine->GetSceneManager()) {
                // Create scenes directory if it doesn't exist
                std::filesystem::create_directories("scenes");
                
                if (g_engine->GetSceneManager()->SaveScene(sceneName)) {
                    MessageBoxA(NULL, ("Scene saved: " + sceneName).c_str(), "Save Scene", MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBoxA(NULL, ("Failed to save scene: " + sceneName).c_str(), "Save Scene Error", MB_OK | MB_ICONERROR);
                }
            }
        }
    }

    void ToolbarSystem::OnFileExport()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: File -> Export");
        MessageBoxA(NULL, "TODO: File -> Export action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnEditUndo()
    {
        std::cout << "========================================" << std::endl;
        std::cout << "UNDO BUTTON CLICKED (or Ctrl+Z pressed)" << std::endl;
        std::cout << "========================================" << std::endl;
        Engine::Core::Logger::Instance().Info("========================================");
        Engine::Core::Logger::Instance().Info("UNDO BUTTON CLICKED");
        Engine::Core::Logger::Instance().Info("========================================");
        
        extern Engine::Core::Engine* g_engine;
        if (!g_engine || !g_engine->GetCommandHistory() || !g_engine->GetSceneManager()) {
            std::cout << ">>> ERROR: Undo system not available" << std::endl;
            MessageBoxA(NULL, "Undo system not available", "Undo", MB_OK | MB_ICONWARNING);
            return;
        }

        if (g_engine->GetCommandHistory()->CanUndo()) {
            std::string description = g_engine->GetCommandHistory()->GetUndoDescription();
            std::cout << ">>> UNDOING: " << description << std::endl;
            
            if (g_engine->GetCommandHistory()->Undo(g_engine->GetSceneManager())) {
                std::cout << ">>> SUCCESS: Undone: " << description << std::endl;
                Engine::Core::Logger::Instance().Info(">>> SUCCESS: Undone: " + description);
                MessageBoxA(NULL, ("Undone: " + description).c_str(), "Undo Success", MB_OK | MB_ICONINFORMATION);
            } else {
                std::cout << ">>> ERROR: Failed to undo" << std::endl;
                MessageBoxA(NULL, "Failed to undo", "Undo Error", MB_OK | MB_ICONERROR);
            }
        } else {
            std::cout << ">>> INFO: Nothing to undo" << std::endl;
            MessageBoxA(NULL, "Nothing to undo", "Undo", MB_OK | MB_ICONINFORMATION);
        }
    }

    void ToolbarSystem::OnEditRedo()
    {
        std::cout << "========================================" << std::endl;
        std::cout << "REDO BUTTON CLICKED (or Ctrl+Y pressed)" << std::endl;
        std::cout << "========================================" << std::endl;
        Engine::Core::Logger::Instance().Info("========================================");
        Engine::Core::Logger::Instance().Info("REDO BUTTON CLICKED");
        Engine::Core::Logger::Instance().Info("========================================");
        
        extern Engine::Core::Engine* g_engine;
        if (!g_engine || !g_engine->GetCommandHistory() || !g_engine->GetSceneManager()) {
            std::cout << ">>> ERROR: Redo system not available" << std::endl;
            MessageBoxA(NULL, "Redo system not available", "Redo", MB_OK | MB_ICONWARNING);
            return;
        }

        if (g_engine->GetCommandHistory()->CanRedo()) {
            std::string description = g_engine->GetCommandHistory()->GetRedoDescription();
            std::cout << ">>> REDOING: " << description << std::endl;
            
            if (g_engine->GetCommandHistory()->Redo(g_engine->GetSceneManager())) {
                std::cout << ">>> SUCCESS: Redone: " << description << std::endl;
                Engine::Core::Logger::Instance().Info(">>> SUCCESS: Redone: " + description);
                MessageBoxA(NULL, ("Redone: " + description).c_str(), "Redo Success", MB_OK | MB_ICONINFORMATION);
            } else {
                std::cout << ">>> ERROR: Failed to redo" << std::endl;
                MessageBoxA(NULL, "Failed to redo", "Redo Error", MB_OK | MB_ICONERROR);
            }
        } else {
            std::cout << ">>> INFO: Nothing to redo" << std::endl;
            MessageBoxA(NULL, "Nothing to redo", "Redo", MB_OK | MB_ICONINFORMATION);
        }
    }

    void ToolbarSystem::OnEditCut()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Edit -> Cut");
        MessageBoxA(NULL, "TODO: Edit -> Cut action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnEditCopy()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Edit -> Copy");
        MessageBoxA(NULL, "TODO: Edit -> Copy action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnEditPaste()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Edit -> Paste");
        MessageBoxA(NULL, "TODO: Edit -> Paste action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnEditDelete()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Edit -> Delete");
        MessageBoxA(NULL, "TODO: Edit -> Delete action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnViewWireframe()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: View -> Wireframe");
        MessageBoxA(NULL, "TODO: View -> Wireframe action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnViewSolid()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: View -> Solid");
        MessageBoxA(NULL, "TODO: View -> Solid action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnViewTextured()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: View -> Textured");
        MessageBoxA(NULL, "TODO: View -> Textured action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnViewFullscreen()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: View -> Fullscreen");
        MessageBoxA(NULL, "TODO: View -> Fullscreen action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnViewResetCamera()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: View -> Reset Camera");
        MessageBoxA(NULL, "TODO: View -> Reset Camera action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnTransformSelect()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Transform -> Select");
        MessageBoxA(NULL, "TODO: Transform -> Select action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnTransformMove()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Transform -> Move");
        MessageBoxA(NULL, "TODO: Transform -> Move action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnTransformRotate()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Transform -> Rotate");
        MessageBoxA(NULL, "TODO: Transform -> Rotate action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnTransformScale()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Transform -> Scale");
        MessageBoxA(NULL, "TODO: Transform -> Scale action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnRenderPlay()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Render -> Play");
        MessageBoxA(NULL, "TODO: Render -> Play action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnRenderPause()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Render -> Pause");
        MessageBoxA(NULL, "TODO: Render -> Pause action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnRenderStop()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Render -> Stop");
        MessageBoxA(NULL, "TODO: Render -> Stop action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

    void ToolbarSystem::OnRenderFrame()
    {
        Engine::Core::Logger::Instance().Info("Toolbar: Render -> Render Frame");
        MessageBoxA(NULL, "TODO: Render -> Render Frame action.", "Toolbar Debug", MB_OK | MB_ICONINFORMATION);
    }

} // namespace UI
} // namespace Engine
