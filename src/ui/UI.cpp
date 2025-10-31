#include "ui/UI.h"
#include "core/Logger.h"
#include <algorithm>
#include <iostream>
#include <fstream>

namespace Engine {
namespace UI {

    // UIComponent implementation
    UIComponent::UIComponent(const std::string& name, int x, int y, int width, int height)
        : m_name(name), m_x(x), m_y(y), m_width(width), m_height(height),
          m_visible(true), m_borderWidth(0) {
        
        // Initialize colors to default values
        m_backgroundColor = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
        m_textColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        m_borderColor = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
        
        Engine::Core::Logger::Instance().Debug("UIComponent created: " + name);
    }

    void UIComponent::SetPosition(int x, int y) {
        m_x = x;
        m_y = y;
    }

    void UIComponent::SetSize(int width, int height) {
        m_width = width;
        m_height = height;
    }

    void UIComponent::SetBounds(int x, int y, int width, int height) {
        m_x = x;
        m_y = y;
        m_width = width;
        m_height = height;
    }

    void UIComponent::SetBackgroundColor(float r, float g, float b, float a) {
        m_backgroundColor = DirectX::XMFLOAT4(r, g, b, a);
    }

    void UIComponent::SetTextColor(float r, float g, float b, float a) {
        m_textColor = DirectX::XMFLOAT4(r, g, b, a);
    }

    void UIComponent::SetBorderColor(float r, float g, float b, float a) {
        m_borderColor = DirectX::XMFLOAT4(r, g, b, a);
    }

    bool UIComponent::Contains(int x, int y) const {
        return x >= m_x && x < m_x + m_width && y >= m_y && y < m_y + m_height;
    }

    // UIPanel implementation
    UIPanel::UIPanel(const std::string& name, int x, int y, int width, int height)
        : UIComponent(name, x, y, width, height), m_windowHandle(nullptr) {
        
        Engine::Core::Logger::Instance().Debug("UIPanel created: " + name);
    }

    UIPanel::~UIPanel() {
        Engine::Core::Logger::Instance().Debug("UIPanel destroyed: " + m_name);
    }

    void UIPanel::AddChild(std::shared_ptr<UIComponent> child) {
        if (!child) {
            Engine::Core::Logger::Instance().Warning("Attempted to add null child to panel: " + m_name);
            return;
        }
        
        m_children.push_back(child);
        Engine::Core::Logger::Instance().Debug("Added child '" + child->GetName() + "' to panel '" + m_name + "'");
    }

    void UIPanel::RemoveChild(std::shared_ptr<UIComponent> child) {
        if (!child) {
            Engine::Core::Logger::Instance().Warning("Attempted to remove null child from panel: " + m_name);
            return;
        }
        
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end()) {
            m_children.erase(it);
            Engine::Core::Logger::Instance().Debug("Removed child '" + child->GetName() + "' from panel '" + m_name + "'");
        } else {
            Engine::Core::Logger::Instance().Warning("Child '" + child->GetName() + "' not found in panel '" + m_name + "'");
        }
    }

    void UIPanel::RemoveChild(const std::string& name) {
        auto it = std::find_if(m_children.begin(), m_children.end(),
            [&name](const std::shared_ptr<UIComponent>& child) {
                return child && child->GetName() == name;
            });
        
        if (it != m_children.end()) {
            Engine::Core::Logger::Instance().Debug("Removing child '" + name + "' from panel '" + m_name + "'");
            m_children.erase(it);
        } else {
            Engine::Core::Logger::Instance().Warning("Child '" + name + "' not found in panel '" + m_name + "'");
        }
    }

    void UIPanel::RemoveAllChildren() {
        Engine::Core::Logger::Instance().Debug("Removing all children from panel: " + m_name);
        m_children.clear();
    }

    void UIPanel::RemoveAllChildrenOfType(UIComponent::Type type) {
        auto it = std::remove_if(m_children.begin(), m_children.end(),
            [type](const std::shared_ptr<UIComponent>& child) {
                return child && child->GetType() == type;
            });
        
        if (it != m_children.end()) {
            Engine::Core::Logger::Instance().Debug("Removed children of type from panel: " + m_name);
            m_children.erase(it, m_children.end());
        }
    }

    std::shared_ptr<UIComponent> UIPanel::GetChild(const std::string& name) {
        auto it = std::find_if(m_children.begin(), m_children.end(),
            [&name](const std::shared_ptr<UIComponent>& child) {
                return child && child->GetName() == name;
            });
        
        if (it != m_children.end()) {
            return *it;
        }
        
        return nullptr;
    }

    void UIPanel::Update(float deltaTime) {
        // Update all children
        for (auto& child : m_children) {
            if (child) {
                child->Update(deltaTime);
            }
        }
    }

    void UIPanel::Render(HDC hdc) {
        std::cout << "DEBUG: UIPanel::Render called for '" << m_name << "' - visible: " << m_visible << ", children: " << m_children.size() << std::endl;
        Engine::Core::Logger::Instance().Info("UIPanel::Render called for '" + m_name + "' - visible: " + (m_visible ? "true" : "false") + ", children: " + std::to_string(m_children.size()));
        
        if (!m_visible) {
            std::cout << "DEBUG: Panel '" << m_name << "' not visible, skipping render" << std::endl;
            return;
        }
        
        // Create brush for background
        HBRUSH bgBrush = CreateSolidBrush(RGB(
            static_cast<int>(m_backgroundColor.x * 255),
            static_cast<int>(m_backgroundColor.y * 255),
            static_cast<int>(m_backgroundColor.z * 255)
        ));
        
        // Create pen for border
        HPEN borderPen = nullptr;
        if (m_borderWidth > 0) {
            borderPen = CreatePen(PS_SOLID, m_borderWidth, RGB(
                static_cast<int>(m_borderColor.x * 255),
                static_cast<int>(m_borderColor.y * 255),
                static_cast<int>(m_borderColor.z * 255)
            ));
        }
        
        // Select objects into DC
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
        HPEN oldPen = nullptr;
        if (borderPen) {
            oldPen = (HPEN)SelectObject(hdc, borderPen);
        }
        
        // Draw panel
        Rectangle(hdc, m_x, m_y, m_x + m_width, m_y + m_height);
        
        // DEBUG: Log panel rendering to file
        std::ofstream debugFile("panel_debug.txt", std::ios::app);
        debugFile << "RENDERED: Panel '" << m_name << "' at (" << m_x << "," << m_y << ") size " << m_width << "x" << m_height << " (right edge at " << (m_x + m_width) << ")" << std::endl;
        debugFile.close();
        
        std::cout << "DEBUG: Drew panel '" << m_name << "' at (" << m_x << "," << m_y << ") size " << m_width << "x" << m_height << " (right edge at " << (m_x + m_width) << ")" << std::endl;
        Engine::Core::Logger::Instance().Info("Drew panel '" + m_name + "' at (" + std::to_string(m_x) + "," + std::to_string(m_y) + ") size " + std::to_string(m_width) + "x" + std::to_string(m_height) + " (right edge at " + std::to_string(m_x + m_width) + ")");
        
        // Restore DC
        SelectObject(hdc, oldBrush);
        if (borderPen) {
            SelectObject(hdc, oldPen);
            DeleteObject(borderPen);
        }
        DeleteObject(bgBrush);
        
        // Render children with coordinate transformation
        for (auto& child : m_children) {
            if (child && child->IsVisible()) {
                std::cout << "DEBUG: Rendering child '" << child->GetName() << "' of panel '" << m_name << "'" << std::endl;
                Engine::Core::Logger::Instance().Info("Rendering child '" + child->GetName() + "' of panel '" + m_name + "'");
                
                // Save the current DC state
                int savedDC = SaveDC(hdc);
                
                // Set up coordinate transformation for child rendering
                SetViewportOrgEx(hdc, m_x, m_y, nullptr);
                
                // Render the child (relative to the panel)
                child->Render(hdc);
                
                // Restore the DC state
                RestoreDC(hdc, savedDC);
            }
        }

        // === REMOVE DEBUG BORDERS: No thick colored line
        // (Do not draw extra debug rectangles here; panels should only be drawn by their own settings!)
        
        std::cout << "DEBUG: UIPanel::Render completed for '" << m_name << "'" << std::endl;
        Engine::Core::Logger::Instance().Info("UIPanel::Render completed for '" + m_name + "'");
    }

    void UIPanel::HandleMouseEvent(int x, int y, int button, bool isDown) {
        if (!m_visible || !Contains(x, y)) {
            return;
        }
        
        // Convert coordinates to be relative to this panel
        int localX = x - m_x;
        int localY = y - m_y;
        
        // Handle children (in reverse order for proper z-ordering)
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            auto& child = *it;
            if (child && child->IsVisible()) {
                // Check if click is within child bounds using local coordinates
                int childX = child->GetX();
                int childY = child->GetY();
                int childW = child->GetWidth();
                int childH = child->GetHeight();
                
                // DEBUG: Log child checking (only for clicks, first 10 times)
                static int checkCount = 0;
                if (isDown && button == 0) {
                    checkCount++;
                    if (checkCount <= 20) {
                        std::cout << "[UIPanel:" << m_name << "] Checking child '" << child->GetName() 
                                  << "': local(" << localX << "," << localY << ") vs child(" << childX << "," << childY 
                                  << "," << childW << "," << childH << ")" << std::endl;
                    }
                }
                
                if (localX >= childX && localX < childX + childW &&
                    localY >= childY && localY < childY + childH) {
                    // DEBUG: Log when forwarding to child (only for clicks)
                    static int forwardCount = 0;
                    if (isDown && button == 0) {
                        forwardCount++;
                        if (forwardCount <= 20) {
                            std::cout << "[UIPanel:" << m_name << "] >>> HIT child '" << child->GetName() 
                                      << "' - forwarding: local(" << localX << "," << localY << ") -> child(" 
                                      << childX << "," << childY << "," << childW << "," << childH << ")" << std::endl;
                        }
                    }
                    
                    // Pass local coordinates (relative to this panel) to child
                    // Child's coordinates are already relative to its parent
                    child->HandleMouseEvent(localX, localY, button, isDown);
                    break; // Only handle the topmost child
                } else {
                    if (isDown && button == 0 && checkCount <= 20) {
                        std::cout << "[UIPanel:" << m_name << "] Child '" << child->GetName() << "' MISS" << std::endl;
                    }
                }
            }
        }
    }

    void UIPanel::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) {
        // Forward to children
        for (auto& child : m_children) {
            if (child && child->IsVisible()) {
                child->HandleKeyboardEvent(message, wParam, lParam);
            }
        }
    }

    // UIButton implementation
    UIButton::UIButton(const std::string& name, const std::string& text, int x, int y, int width, int height)
        : UIComponent(name, x, y, width, height), m_text(text), m_hovered(false), m_pressed(false) {
        
        // Set default hover color
        m_hoverColor = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
        
        Engine::Core::Logger::Instance().Debug("UIButton created: " + name + " (" + text + ")");
    }

    UIButton::~UIButton() {
        Engine::Core::Logger::Instance().Debug("UIButton destroyed: " + m_name);
    }

    void UIButton::SetHoverColor(float r, float g, float b, float a) {
        m_hoverColor = DirectX::XMFLOAT4(r, g, b, a);
    }

    void UIButton::Update(float deltaTime) {
        (void)deltaTime; // Suppress unused parameter warning
        // Update button state
        // This could include animations, etc.
    }

    void UIButton::Render(HDC hdc) {
        if (!m_visible) {
            return;
        }
        
        // Determine colors based on state
        DirectX::XMFLOAT4 bgColor = m_backgroundColor;
        if (m_pressed) {
            // Darker when pressed
            bgColor.x *= 0.7f;
            bgColor.y *= 0.7f;
            bgColor.z *= 0.7f;
        } else if (m_hovered) {
            bgColor = m_hoverColor;
        }
        
        // Create brush for background
        HBRUSH bgBrush = CreateSolidBrush(RGB(
            static_cast<int>(bgColor.x * 255),
            static_cast<int>(bgColor.y * 255),
            static_cast<int>(bgColor.z * 255)
        ));
        
        // Create pen for border
        HPEN borderPen = nullptr;
        if (m_borderWidth > 0) {
            borderPen = CreatePen(PS_SOLID, m_borderWidth, RGB(
                static_cast<int>(m_borderColor.x * 255),
                static_cast<int>(m_borderColor.y * 255),
                static_cast<int>(m_borderColor.z * 255)
            ));
        }
        
        // Select objects into DC
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
        HPEN oldPen = nullptr;
        if (borderPen) {
            oldPen = (HPEN)SelectObject(hdc, borderPen);
        }
        
        // Draw button
        Rectangle(hdc, m_x, m_y, m_x + m_width, m_y + m_height);
        
        // Draw text
        if (!m_text.empty()) {
            ::SetTextColor(hdc, RGB(
                static_cast<int>(m_textColor.x * 255),
                static_cast<int>(m_textColor.y * 255),
                static_cast<int>(m_textColor.z * 255)
            ));
            SetBkMode(hdc, TRANSPARENT);
            
            RECT textRect = { m_x, m_y, m_x + m_width, m_y + m_height };
            DrawTextA(hdc, m_text.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        
        // Restore DC
        SelectObject(hdc, oldBrush);
        if (borderPen) {
            SelectObject(hdc, oldPen);
            DeleteObject(borderPen);
        }
        DeleteObject(bgBrush);
    }

    void UIButton::HandleMouseEvent(int x, int y, int button, bool isDown) {
        if (!m_visible || !Contains(x, y)) {
            m_hovered = false;
            m_pressed = false;
            return;
        }
        
        m_hovered = true;
        
        if (button == 0) { // Left mouse button
            if (isDown) {
                m_pressed = true;
            } else {
                if (m_pressed && m_onClickCallback) {
                    m_onClickCallback();
                }
                m_pressed = false;
            }
        }
    }

    // UITextArea implementation
    UITextArea::UITextArea(const std::string& name, const std::string& text, int x, int y, int width, int height)
        : UIComponent(name, x, y, width, height), m_text(text), m_editable(true), 
          m_focused(false), m_cursorPosition(0) {
        
        Engine::Core::Logger::Instance().Debug("UITextArea created: " + name + " (" + text + ")");
    }

    UITextArea::~UITextArea() {
        Engine::Core::Logger::Instance().Debug("UITextArea destroyed: " + m_name);
    }

    void UITextArea::AppendText(const std::string& text) {
        m_text += text;
    }

    void UITextArea::ClearText() {
        m_text.clear();
        m_cursorPosition = 0;
    }

    void UITextArea::Update(float deltaTime) {
        (void)deltaTime; // Suppress unused parameter warning
        // Update text area state
        // This could include cursor blinking, etc.
    }

    void UITextArea::Render(HDC hdc) {
        if (!m_visible) {
            return;
        }
        
        // Create brush for background
        HBRUSH bgBrush = CreateSolidBrush(RGB(
            static_cast<int>(m_backgroundColor.x * 255),
            static_cast<int>(m_backgroundColor.y * 255),
            static_cast<int>(m_backgroundColor.z * 255)
        ));
        
        // Create pen for border
        HPEN borderPen = nullptr;
        if (m_borderWidth > 0) {
            borderPen = CreatePen(PS_SOLID, m_borderWidth, RGB(
                static_cast<int>(m_borderColor.x * 255),
                static_cast<int>(m_borderColor.y * 255),
                static_cast<int>(m_borderColor.z * 255)
            ));
        }
        
        // Select objects into DC
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
        HPEN oldPen = nullptr;
        if (borderPen) {
            oldPen = (HPEN)SelectObject(hdc, borderPen);
        }
        
        // Draw text area
        Rectangle(hdc, m_x, m_y, m_x + m_width, m_y + m_height);
        
        // Draw text or placeholder
        std::string displayText = m_text.empty() ? m_placeholderText : m_text;
        if (!displayText.empty()) {
            ::SetTextColor(hdc, RGB(
                static_cast<int>(m_textColor.x * 255),
                static_cast<int>(m_textColor.y * 255),
                static_cast<int>(m_textColor.z * 255)
            ));
            SetBkMode(hdc, TRANSPARENT);
            
            RECT textRect = { m_x + 5, m_y + 5, m_x + m_width - 5, m_y + m_height - 5 };
            DrawTextA(hdc, displayText.c_str(), -1, &textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
        }
        
        // Restore DC
        SelectObject(hdc, oldBrush);
        if (borderPen) {
            SelectObject(hdc, oldPen);
            DeleteObject(borderPen);
        }
        DeleteObject(bgBrush);
    }

    void UITextArea::HandleMouseEvent(int x, int y, int button, bool isDown) {
        if (!m_visible) {
            return;
        }
        
        if (Contains(x, y)) {
            if (button == 0 && isDown) { // Left mouse button down
                m_focused = true;
                // TODO: Set cursor position based on click location
            }
        } else {
            m_focused = false;
        }
    }

    void UITextArea::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) {
        (void)lParam; // Suppress unused parameter warning
        if (!m_visible || !m_focused || !m_editable) {
            return;
        }
        
        if (message == WM_KEYDOWN) {
            switch (wParam) {
                case VK_BACK: // Backspace
                    if (!m_text.empty() && m_cursorPosition > 0) {
                        m_text.erase(m_cursorPosition - 1, 1);
                        m_cursorPosition--;
                    }
                    break;
                    
                case VK_DELETE: // Delete
                    if (!m_text.empty() && m_cursorPosition < m_text.length()) {
                        m_text.erase(m_cursorPosition, 1);
                    }
                    break;
                    
                case VK_LEFT: // Left arrow
                    if (m_cursorPosition > 0) {
                        m_cursorPosition--;
                    }
                    break;
                    
                case VK_RIGHT: // Right arrow
                    if (m_cursorPosition < m_text.length()) {
                        m_cursorPosition++;
                    }
                    break;
                    
                case VK_HOME: // Home
                    m_cursorPosition = 0;
                    break;
                    
                case VK_END: // End
                    m_cursorPosition = static_cast<int>(m_text.length());
                    break;
            }
        } else if (message == WM_CHAR) {
            // Handle character input
            char ch = static_cast<char>(wParam);
            if (ch >= 32 && ch <= 126) { // Printable characters
                m_text.insert(m_cursorPosition, 1, ch);
                m_cursorPosition++;
            }
        }
    }

} // namespace UI
} // namespace Engine
