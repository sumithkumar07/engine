#include "ui/SceneHierarchy.h"
#include "ui/ViewportRenderer.h"
#include "core/Logger.h"
#include <algorithm>
#include <iostream>

namespace Engine {
namespace UI {

    SceneHierarchy::SceneHierarchy()
        : UIComponent("SceneHierarchy", 0, 0, 200, 400)
        , m_viewportRenderer(nullptr)
        , m_scrollOffset(0)
        , m_itemHeight(20)
    {
        Engine::Core::Logger::Instance().Debug("SceneHierarchy created");
    }

    SceneHierarchy::~SceneHierarchy()
    {
        Shutdown();
        Engine::Core::Logger::Instance().Debug("SceneHierarchy destroyed");
    }

    bool SceneHierarchy::Initialize(class ViewportRenderer* viewportRenderer)
    {
        if (!viewportRenderer) {
            Engine::Core::Logger::Instance().Error("ViewportRenderer is null");
            return false;
        }

        m_viewportRenderer = viewportRenderer;
        UpdateObjectList();
        
        Engine::Core::Logger::Instance().Info("SceneHierarchy initialized");
        return true;
    }

    void SceneHierarchy::Shutdown()
    {
        m_viewportRenderer = nullptr;
        m_objectList.clear();
        m_selectedObject.clear();
        Engine::Core::Logger::Instance().Info("SceneHierarchy shutdown");
    }

    void SceneHierarchy::Update(float deltaTime)
    {
        (void)deltaTime; // Suppress unused parameter warning
        UpdateObjectList();
    }

    void SceneHierarchy::Render(HDC hdc)
    {
        if (!m_visible) {
            return;
        }

        // Draw background
        HBRUSH hBrush = CreateSolidBrush(RGB(40, 40, 40));
        RECT rect = { m_x, m_y, m_x + m_width, m_y + m_height };
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        // Draw border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, m_x, m_y, NULL);
        LineTo(hdc, m_x + m_width, m_y);
        LineTo(hdc, m_x + m_width, m_y + m_height);
        LineTo(hdc, m_x, m_y + m_height);
        LineTo(hdc, m_x, m_y);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        // Draw title
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT titleRect = { m_x + 5, m_y + 5, m_x + m_width - 5, m_y + 25 };
        DrawText(hdc, L"Scene Hierarchy", -1, &titleRect, DT_LEFT | DT_VCENTER);

        // Draw object list - start well below title to prevent truncation
        RenderObjectList(hdc);

        // Draw scroll bar if needed
        if (m_objectList.size() > (m_height - 80) / m_itemHeight) {
            RenderScrollBar(hdc);
        }
    }

    void SceneHierarchy::HandleMouseEvent(int x, int y, int button, bool pressed)
    {
        // DEBUG: Log ALL events (especially clicks)
        static int eventCount = 0;
        if (pressed && button == 0) {
            // This is a CLICK - log it prominently
            eventCount++;
            std::cout << "========================================" << std::endl;
            std::cout << "[SceneHierarchy] CLICK #" << eventCount << " DETECTED!" << std::endl;
            std::cout << "  Coordinates: x=" << x << ", y=" << y << std::endl;
            std::cout << "  Visible: " << m_visible << ", Button: " << button << ", Pressed: " << pressed << std::endl;
            std::cout << "  Bounds: x=[0," << m_width << "], y=[0," << m_height << "] (m_x=" << m_x << ", m_y=" << m_y << ")" << std::endl;
            std::cout << "========================================" << std::endl;
        }
        
        if (!m_visible) {
            if (pressed && button == 0 && eventCount <= 5) {
                std::cout << "[SceneHierarchy] CLICK REJECTED: not visible" << std::endl;
            }
            return;
        }
        
        if (!pressed) {
            // Mouse move or button up - ignore for now
            return;
        }
        
        if (button != 0) {
            if (pressed && eventCount <= 5) {
                std::cout << "[SceneHierarchy] CLICK REJECTED: wrong button (button=" << button << ", need 0)" << std::endl;
            }
            return;
        }

        // Check if click is within bounds
        // Coordinates are already relative to parent panel, and m_x, m_y should be 0 for children
        // But we check against 0-width/height bounds to be safe
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
            if (eventCount <= 10) {
                std::cout << "[SceneHierarchy] Click outside bounds: x=" << x << " not in [0," << m_width 
                          << "], y=" << y << " not in [0," << m_height << "] (m_x=" << m_x << ", m_y=" << m_y << ")" << std::endl;
            }
            return;
        }

        // Convert coordinates from parent-relative to component-relative (if needed)
        // For SceneHierarchy, coordinates should already be relative (m_x=0, m_y=0), but check anyway
        int localX = x - m_x;
        int localY = y - m_y;
        
        // Check if click is on an object (using local coordinates)
        std::string objectName = GetObjectAtPosition(localX, localY);
        if (!objectName.empty()) {
            std::cout << "[SceneHierarchy] ✓ Click detected on object: '" << objectName << "' at local coords (" << localX << "," << localY << ")" << std::endl;
            SelectObject(objectName);
        } else {
            if (eventCount <= 10) {
                std::cout << "[SceneHierarchy] ✗ Click at (" << localX << "," << localY 
                          << ") - no object found. itemStartY=80, itemHeight=" << m_itemHeight 
                          << ", visibleItems=" << ((m_height - 80) / m_itemHeight)
                          << ", objectList.size()=" << m_objectList.size() << std::endl;
            }
        }
    }

    void SceneHierarchy::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam)
    {
        (void)message; (void)wParam; (void)lParam; // Suppress unused parameter warnings
        
        // Handle keyboard shortcuts for scene hierarchy
        if (message == WM_KEYDOWN) {
            switch (wParam) {
                case VK_DELETE:
                    if (!m_selectedObject.empty()) {
                        // Fire delete callback instead of directly removing
                        if (m_deleteCallback) {
                            m_deleteCallback(m_selectedObject);
                        } else {
                            // Fallback: direct removal (no undo)
                            RemoveObject(m_selectedObject);
                        }
                    }
                    break;
                case VK_F2:
                    if (!m_selectedObject.empty()) {
                        // Start rename mode
                        Engine::Core::Logger::Instance().Info("Rename object: " + m_selectedObject);
                    }
                    break;
            }
        }
    }

    void SceneHierarchy::AddObject(const std::string& name, const std::string& type)
    {
        (void)type; // Suppress unused parameter warning
        
        auto it = std::find(m_objectList.begin(), m_objectList.end(), name);
        if (it == m_objectList.end()) {
            m_objectList.push_back(name);
            Engine::Core::Logger::Instance().Info("Added object to hierarchy: " + name);
        }
    }

    void SceneHierarchy::RemoveObject(const std::string& name)
    {
        auto it = std::find(m_objectList.begin(), m_objectList.end(), name);
        if (it != m_objectList.end()) {
            m_objectList.erase(it);
            if (m_selectedObject == name) {
                m_selectedObject.clear();
            }
            Engine::Core::Logger::Instance().Info("Removed object from hierarchy: " + name);
        }
    }

    void SceneHierarchy::SelectObject(const std::string& name)
    {
        m_selectedObject = name;
        std::cout << "========================================" << std::endl;
        std::cout << "SCENE HIERARCHY: Object clicked: " << name << std::endl;
        std::cout << "========================================" << std::endl;
        Engine::Core::Logger::Instance().Info("========================================");
        Engine::Core::Logger::Instance().Info("SCENE HIERARCHY: Object clicked: " + name);
        Engine::Core::Logger::Instance().Info("========================================");
        
        if (m_selectionCallback) {
            m_selectionCallback(name);
        }
    }

    void SceneHierarchy::ClearSelection()
    {
        m_selectedObject.clear();
        Engine::Core::Logger::Instance().Debug("Cleared selection");
    }

    void SceneHierarchy::SetSelectionCallback(std::function<void(const std::string&)> callback)
    {
        m_selectionCallback = callback;
    }

    void SceneHierarchy::SetDoubleClickCallback(std::function<void(const std::string&)> callback)
    {
        m_doubleClickCallback = callback;
    }

    void SceneHierarchy::SetDeleteCallback(std::function<void(const std::string&)> callback)
    {
        m_deleteCallback = callback;
    }

    void SceneHierarchy::RenderObjectList(HDC hdc)
    {
        int startY = m_y + 80; // Even more padding to prevent text truncation
        int visibleItems = (m_height - 80) / m_itemHeight;
        
        for (size_t i = 0; i < m_objectList.size() && i < static_cast<size_t>(visibleItems); ++i) {
            size_t index = i + m_scrollOffset;
            if (index >= m_objectList.size()) break;

            const std::string& objectName = m_objectList[index];
            int itemY = startY + static_cast<int>(i) * m_itemHeight;

            // Highlight selected object
            if (objectName == m_selectedObject) {
                HBRUSH hBrush = CreateSolidBrush(RGB(60, 100, 150));
                RECT rect = { m_x + 2, itemY, m_x + m_width - 2, itemY + m_itemHeight };
                FillRect(hdc, &rect, hBrush);
                DeleteObject(hBrush);
            }

            // Draw object name
            ::SetTextColor(hdc, RGB(255, 255, 255));
            RECT textRect = { m_x + 5, itemY, m_x + m_width - 5, itemY + m_itemHeight };
            
            // Convert string to wide string
            std::wstring wideName(objectName.begin(), objectName.end());
            DrawText(hdc, wideName.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
        }
    }

    void SceneHierarchy::RenderScrollBar(HDC hdc)
    {
        // Simple scroll bar implementation
        int scrollBarX = m_x + m_width - 15;
        int scrollBarY = m_y + 30;
        int scrollBarHeight = m_height - 30;
        int visibleItems = (m_height - 30) / m_itemHeight;

        // Draw scroll bar background
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT rect = { scrollBarX, scrollBarY, scrollBarX + 15, scrollBarY + scrollBarHeight };
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        // Draw scroll bar thumb
        hBrush = CreateSolidBrush(RGB(100, 100, 100));
        int thumbHeight = (20 > scrollBarHeight / 4) ? 20 : scrollBarHeight / 4;
        int maxItems = static_cast<int>(m_objectList.size()) - visibleItems;
        int thumbY = scrollBarY + (m_scrollOffset * (scrollBarHeight - thumbHeight)) / ((maxItems > 1) ? maxItems : 1);
        rect = { scrollBarX + 2, thumbY, scrollBarX + 13, thumbY + thumbHeight };
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);
    }

    std::string SceneHierarchy::GetObjectAtPosition(int x, int y)
    {
        // x, y are now local coordinates (relative to this component)
        // Match the rendering offset: items start at 80 pixels from top (title takes ~30px, padding adds 50px)
        int itemStartY = 80;
        
        // Check bounds using local coordinates (m_x and m_y should be 0 for local coords, but check anyway)
        int localX = x - m_x;
        int localY = y - m_y;
        
        if (localX < 5 || localX > m_width - 20 || localY < itemStartY) {
            // DEBUG: Log why click was rejected
            static int rejectCount = 0;
            if (++rejectCount <= 5) {
                std::cout << "[GetObjectAtPosition] Rejected: localX=" << localX << " (" 
                          << (localX < 5 ? "too left" : (localX > m_width - 20 ? "too right" : "ok"))
                          << "), localY=" << localY << " (" << (localY < itemStartY ? "above items" : "ok")
                          << "), itemStartY=" << itemStartY << std::endl;
            }
            return "";
        }

        int itemIndex = (localY - itemStartY) / m_itemHeight;
        int visibleItems = (m_height - 80) / m_itemHeight;
        
        // DEBUG: Log calculation
        static int calcCount = 0;
        if (++calcCount <= 5) {
            std::cout << "[GetObjectAtPosition] localY=" << localY << ", itemStartY=" << itemStartY 
                      << ", itemIndex=" << itemIndex << ", visibleItems=" << visibleItems 
                      << ", scrollOffset=" << m_scrollOffset << ", listSize=" << m_objectList.size() << std::endl;
        }
        
        if (itemIndex >= 0 && itemIndex < visibleItems) {
            size_t listIndex = itemIndex + m_scrollOffset;
            if (listIndex < m_objectList.size()) {
                return m_objectList[listIndex];
            }
        }

        return "";
    }

    void SceneHierarchy::UpdateObjectList()
    {
        // In a real implementation, this would query the scene manager
        // for the current list of objects
        if (m_viewportRenderer) {
            // This would call the viewport renderer to get scene objects
            // For now, we'll just maintain a static list
        }
    }

} // namespace UI
} // namespace Engine
