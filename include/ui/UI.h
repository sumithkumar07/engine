#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <windows.h>
#include <DirectXMath.h>

namespace Engine {
namespace UI {

    // Forward declarations
    class UIManager;
    class UIRenderer;

    /**
     * @brief UIComponent base class - Same as before, but with proper inheritance
     * 
     * Base class for all UI components.
     * Same design as the previous implementation, but with proper virtual functions.
     */
    class UIComponent {
    public:
        enum class Type {
            Panel,
            Button,
            TextArea,
            Menu,
            Toolbar
        };

        UIComponent(const std::string& name, int x, int y, int width, int height);
        virtual ~UIComponent() = default;

        // Basic properties
        const std::string& GetName() const { return m_name; }
        int GetX() const { return m_x; }
        int GetY() const { return m_y; }
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        
        void SetPosition(int x, int y);
        void SetSize(int width, int height);
        void SetBounds(int x, int y, int width, int height);

        // Visibility
        bool IsVisible() const { return m_visible; }
        void SetVisible(bool visible) { m_visible = visible; }

        // Color properties
        const DirectX::XMFLOAT4& GetBackgroundColor() const { return m_backgroundColor; }
        const DirectX::XMFLOAT4& GetTextColor() const { return m_textColor; }
        const DirectX::XMFLOAT4& GetBorderColor() const { return m_borderColor; }
        
        void SetBackgroundColor(float r, float g, float b, float a);
        void SetTextColor(float r, float g, float b, float a);
        void SetBorderColor(float r, float g, float b, float a);

        // Border
        int GetBorderWidth() const { return m_borderWidth; }
        void SetBorderWidth(int width) { m_borderWidth = width; }

        // Hit testing
        bool Contains(int x, int y) const;

        // Virtual methods
        virtual Type GetType() const = 0;
        virtual void Update(float deltaTime) { (void)deltaTime; }
        virtual void Render(HDC hdc) = 0;
        virtual void HandleMouseEvent(int x, int y, int button, bool isDown) { (void)x; (void)y; (void)button; (void)isDown; }
        virtual void HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) { (void)message; (void)wParam; (void)lParam; }
        virtual void Shutdown() {}

    protected:
        std::string m_name;
        int m_x, m_y, m_width, m_height;
        bool m_visible;
        DirectX::XMFLOAT4 m_backgroundColor;
        DirectX::XMFLOAT4 m_textColor;
        DirectX::XMFLOAT4 m_borderColor;
        int m_borderWidth;
    };

    /**
     * @brief UIPanel class - Same as before, but with proper child management
     * 
     * Panel container for other UI components.
     * Same design as the previous implementation, but with proper child management.
     */
    class UIPanel : public UIComponent {
    public:
        UIPanel(const std::string& name, int x, int y, int width, int height);
        ~UIPanel();

        // Child management
        void AddChild(std::shared_ptr<UIComponent> child);
        void RemoveChild(std::shared_ptr<UIComponent> child);
        void RemoveChild(const std::string& name);
        void RemoveAllChildren();
        void RemoveAllChildrenOfType(UIComponent::Type type);
        
        // Window handle access
        HWND GetWindowHandle() const { return m_windowHandle; }
        void SetWindowHandle(HWND hwnd) { m_windowHandle = hwnd; }
        
        std::shared_ptr<UIComponent> GetChild(const std::string& name);
        const std::vector<std::shared_ptr<UIComponent>>& GetChildren() const { return m_children; }

        // Virtual methods
        Type GetType() const override { return Type::Panel; }
        void Update(float deltaTime) override;
        void Render(HDC hdc) override;
        void HandleMouseEvent(int x, int y, int button, bool isDown) override;
        void HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) override;

    private:
        std::vector<std::shared_ptr<UIComponent>> m_children;
        HWND m_windowHandle;
    };

    /**
     * @brief UIButton class - Same as before, but with proper event handling
     * 
     * Button UI component with click events.
     * Same design as the previous implementation, but with proper event handling.
     */
    class UIButton : public UIComponent {
    public:
        UIButton(const std::string& name, const std::string& text, int x, int y, int width, int height);
        ~UIButton();

        // Text
        const std::string& GetText() const { return m_text; }
        void SetText(const std::string& text) { m_text = text; }

        // Colors
        const DirectX::XMFLOAT4& GetHoverColor() const { return m_hoverColor; }
        void SetHoverColor(float r, float g, float b, float a);

        // Events
        using ClickCallback = std::function<void()>;
        void SetOnClick(ClickCallback callback) { m_onClickCallback = callback; }

        // State
        bool IsHovered() const { return m_hovered; }
        bool IsPressed() const { return m_pressed; }

        // Virtual methods
        Type GetType() const override { return Type::Button; }
        void Update(float deltaTime) override;
        void Render(HDC hdc) override;
        void HandleMouseEvent(int x, int y, int button, bool isDown) override;

    private:
        std::string m_text;
        DirectX::XMFLOAT4 m_hoverColor;
        bool m_hovered;
        bool m_pressed;
        ClickCallback m_onClickCallback;
    };

    /**
     * @brief UITextArea class - Same as before, but with proper text handling
     * 
     * Text area UI component for displaying and editing text.
     * Same design as the previous implementation, but with proper text handling.
     */
    class UITextArea : public UIComponent {
    public:
        UITextArea(const std::string& name, const std::string& text, int x, int y, int width, int height);
        ~UITextArea();

        // Text
        const std::string& GetText() const { return m_text; }
        void SetText(const std::string& text) { m_text = text; }
        void AppendText(const std::string& text);
        void ClearText();

        // Placeholder text
        const std::string& GetPlaceholderText() const { return m_placeholderText; }
        void SetPlaceholderText(const std::string& text) { m_placeholderText = text; }

        // Editing
        bool IsEditable() const { return m_editable; }
        void SetEditable(bool editable) { m_editable = editable; }
        bool IsFocused() const { return m_focused; }
        void SetFocused(bool focused) { m_focused = focused; }

        // Virtual methods
        Type GetType() const override { return Type::TextArea; }
        void Update(float deltaTime) override;
        void Render(HDC hdc) override;
        void HandleMouseEvent(int x, int y, int button, bool isDown) override;
        void HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) override;

    private:
        std::string m_text;
        std::string m_placeholderText;
        bool m_editable;
        bool m_focused;
        int m_cursorPosition;
    };

} // namespace UI
} // namespace Engine
