#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <variant>
#include <windows.h>
#include "UI.h"

namespace Engine {
namespace Core {
    // Forward declarations for core engine components
    class SceneManager;
    class SceneObject;
    class Camera;
    class Light;
    class Material;
    class Mesh;
}

namespace UI {

    /**
     * @brief Properties Panel for AI Movie Studio
     * 
     * Real implementation with actual functionality:
     * - Real-time property editing for selected objects
     * - Transform properties (position, rotation, scale)
     * - Material properties (color, texture, PBR settings)
     * - Camera properties (FOV, focus, aperture)
     * - Light properties (type, intensity, color, shadows)
     * - Animation properties (keyframes, timing)
     * - Dynamic property generation based on object type
     */
    class PropertiesPanel : public UIComponent {
    public:
        Type GetType() const override { return Type::Panel; }
    public:
        enum class PropertyType {
            Float,
            Int,
            Bool,
            String,
            Vector3,
            Color,
            Enum,
            FilePath,
            Button
        };

        struct PropertyValue {
            std::variant<float, int, bool, std::string, DirectX::XMFLOAT3, COLORREF> value;
            PropertyType type;
            std::string name;
            std::string displayName;
            std::string tooltip;
            bool isReadOnly;
            bool isVisible;
            
            // Value constraints
            float minValue;
            float maxValue;
            float step;
            
            // Enum options
            std::vector<std::string> enumOptions;
            
            // File filters
            std::string fileFilter;
            
            PropertyValue() : type(PropertyType::Float), isReadOnly(false), isVisible(true),
                            minValue(0.0f), maxValue(100.0f), step(0.1f) {}
        };

        struct PropertyGroup {
            std::string name;
            std::vector<PropertyValue> properties;
            bool isExpanded;
            bool isVisible;
            
            PropertyGroup() : isExpanded(true), isVisible(true) {}
        };

        PropertiesPanel();
        ~PropertiesPanel();

        /**
         * @brief Initialize the properties panel
         * @return true if initialization successful
         */
        bool Initialize();

        /**
         * @brief Shutdown the properties panel
         */
        void Shutdown();

        /**
         * @brief Update the properties panel
         * @param deltaTime Time elapsed since last frame
         */
        void Update(float deltaTime) override;

        /**
         * @brief Render the properties panel
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

        // Object management
        void SetSelectedObject(std::shared_ptr<Core::SceneObject> object);
        void SetSelectedCamera(std::shared_ptr<Core::Camera> camera);
        void SetSelectedLight(std::shared_ptr<Core::Light> light);
        void SetSelectedMaterial(std::shared_ptr<Core::Material> material);
        void ClearSelection();

        // Property management
        void AddProperty(const std::string& groupName, const PropertyValue& property);
        void RemoveProperty(const std::string& groupName, const std::string& propertyName);
        void UpdateProperty(const std::string& groupName, const std::string& propertyName, const PropertyValue& property);
        void RefreshProperties();

        // Property value access
        PropertyValue* GetProperty(const std::string& groupName, const std::string& propertyName);
        void SetPropertyValue(const std::string& groupName, const std::string& propertyName, const std::variant<float, int, bool, std::string, DirectX::XMFLOAT3, COLORREF>& value);
        std::variant<float, int, bool, std::string, DirectX::XMFLOAT3, COLORREF> GetPropertyValue(const std::string& groupName, const std::string& propertyName);

        // Property groups
        void AddPropertyGroup(const PropertyGroup& group);
        void RemovePropertyGroup(const std::string& groupName);
        void SetGroupExpanded(const std::string& groupName, bool expanded);
        void SetGroupVisible(const std::string& groupName, bool visible);

        // Callbacks
        void SetPropertyChangedCallback(std::function<void(const std::string&, const std::string&, const PropertyValue&)> callback);
        void SetObjectChangedCallback(std::function<void(std::shared_ptr<Core::SceneObject>)> callback);

        // Getters
        const std::vector<PropertyGroup>& GetPropertyGroups() const { return m_propertyGroups; }
        std::shared_ptr<Core::SceneObject> GetSelectedObject() const { return m_selectedObject; }
        std::shared_ptr<Core::Camera> GetSelectedCamera() const { return m_selectedCamera; }
        std::shared_ptr<Core::Light> GetSelectedLight() const { return m_selectedLight; }
        std::shared_ptr<Core::Material> GetSelectedMaterial() const { return m_selectedMaterial; }

    private:
        // Selected objects
        std::shared_ptr<Core::SceneObject> m_selectedObject;
        std::shared_ptr<Core::Camera> m_selectedCamera;
        std::shared_ptr<Core::Light> m_selectedLight;
        std::shared_ptr<Core::Material> m_selectedMaterial;
        
        // Property data
        std::vector<PropertyGroup> m_propertyGroups;
        std::map<std::string, int> m_groupIndices;
        
        // UI state
        int m_scrollOffset;
        int m_itemHeight;
        int m_groupHeaderHeight;
        int m_propertyPadding;
        int m_editingProperty;
        std::string m_editingGroup;
        std::string m_editingPropertyName;
        std::string m_editingText;
        bool m_isEditing;
        
        // Callbacks
        std::function<void(const std::string&, const std::string&, const PropertyValue&)> m_propertyChangedCallback;
        std::function<void(std::shared_ptr<Core::SceneObject>)> m_objectChangedCallback;
        
        // Helper methods
        void GenerateObjectProperties();
        void GenerateCameraProperties();
        void GenerateLightProperties();
        void GenerateMaterialProperties();
        void GenerateTransformProperties();
        void GenerateMeshProperties();
        void GenerateAnimationProperties();
        
        void RenderPropertyGroups(HDC hdc);
        void RenderPropertyGroup(HDC hdc, PropertyGroup& group, int& y);
        void RenderProperty(HDC hdc, PropertyValue& property, int x, int y, int width, int height);
        void RenderPropertyValue(HDC hdc, PropertyValue& property, int x, int y, int width, int height);
        void RenderPropertyEditor(HDC hdc, PropertyValue& property, int x, int y, int width, int height);
        
        void HandlePropertyClick(int x, int y);
        void HandlePropertyEdit(int x, int y);
        void StartPropertyEdit(PropertyValue& property, const std::string& groupName, const std::string& propertyName);
        void FinishPropertyEdit();
        void CancelPropertyEdit();
        
        void UpdateObjectFromProperties();
        void UpdatePropertiesFromObject();
        
        // Property value conversion
        std::string ValueToString(const PropertyValue& property);
        void StringToValue(PropertyValue& property, const std::string& value);
        
        // UI helpers
        void DrawSlider(HDC hdc, int x, int y, int width, int height, float value, float min, float max);
        void DrawColorPicker(HDC hdc, int x, int y, int width, int height, COLORREF color);
        void DrawVector3Editor(HDC hdc, int x, int y, int width, int height, const DirectX::XMFLOAT3& value);
        void DrawEnumDropdown(HDC hdc, int x, int y, int width, int height, const std::vector<std::string>& options, int selected);
        void DrawButton(HDC hdc, int x, int y, int width, int height, const std::string& text);
        
        // Input helpers
        bool IsPointInRect(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight);
        int GetPropertyIndexFromPosition(int x, int y);
        PropertyValue* GetPropertyFromPosition(int x, int y, std::string& groupName, std::string& propertyName);
        
        // Constants
        static constexpr int DEFAULT_ITEM_HEIGHT = 28;  // Increased from 25
        static constexpr int GROUP_HEADER_HEIGHT = 35;  // Increased from 30
        static constexpr int PROPERTY_PADDING = 8;      // Increased from 5
        static constexpr int GROUP_SPACING = 12;        // Added group spacing
        static constexpr int SLIDER_HEIGHT = 20;
        static constexpr int COLOR_PICKER_SIZE = 20;
        static constexpr int VECTOR3_COMPONENT_WIDTH = 60;
    };

} // namespace UI
} // namespace Engine
