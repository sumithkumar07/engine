#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "UI.h"

namespace Engine {
namespace UI {

    /**
     * @brief Scene Hierarchy Manager for the left panel
     * 
     * This class manages the scene hierarchy display and object selection
     * in the left panel of the AI Movie Studio.
     */
    class SceneHierarchy : public UIComponent {
    public:
        SceneHierarchy();
        ~SceneHierarchy();

        // UIComponent overrides
        Type GetType() const override { return Type::Panel; }

        /**
         * @brief Initialize the scene hierarchy
         * @param viewportRenderer The viewport renderer for scene management
         * @return true if initialization successful, false otherwise
         */
        bool Initialize(class ViewportRenderer* viewportRenderer);

        /**
         * @brief Shutdown the scene hierarchy
         */
        void Shutdown();

        /**
         * @brief Update the scene hierarchy display
         * @param deltaTime Time elapsed since last frame
         */
        void Update(float deltaTime) override;

        /**
         * @brief Render the scene hierarchy
         * @param hdc Device context for rendering
         */
        void Render(HDC hdc) override;

        /**
         * @brief Handle mouse events for object selection
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

        // Object management
        void AddObject(const std::string& name, const std::string& type);
        void RemoveObject(const std::string& name);
        void SelectObject(const std::string& name);
        void ClearSelection();

        // Selection callbacks
        void SetSelectionCallback(std::function<void(const std::string&)> callback);
        void SetDoubleClickCallback(std::function<void(const std::string&)> callback);
        void SetDeleteCallback(std::function<void(const std::string&)> callback);

        // Getters
        const std::string& GetSelectedObject() const { return m_selectedObject; }
        const std::vector<std::string>& GetObjectList() const { return m_objectList; }

    private:
        class ViewportRenderer* m_viewportRenderer;
        std::vector<std::string> m_objectList;
        std::string m_selectedObject;
        int m_scrollOffset;
        int m_itemHeight;
        
        std::function<void(const std::string&)> m_selectionCallback;
        std::function<void(const std::string&)> m_doubleClickCallback;
        std::function<void(const std::string&)> m_deleteCallback;

        // Helper methods
        void RenderObjectList(HDC hdc);
        void RenderScrollBar(HDC hdc);
        std::string GetObjectAtPosition(int x, int y);
        void UpdateObjectList();
    };

} // namespace UI
} // namespace Engine
