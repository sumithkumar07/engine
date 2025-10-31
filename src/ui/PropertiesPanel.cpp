#include "ui/PropertiesPanel.h"
#include <iostream>
#include "core/Logger.h"
#include "core/SceneObject.h"
#include "core/Camera.h"
#include "core/Light.h"
#include "core/Material.h"
#include "core/Mesh.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace Engine {
namespace UI {

    PropertiesPanel::PropertiesPanel()
        : UIComponent("PropertiesPanel", 0, 0, 300, 400)
        , m_scrollOffset(0)
        , m_itemHeight(DEFAULT_ITEM_HEIGHT)
        , m_groupHeaderHeight(GROUP_HEADER_HEIGHT)
        , m_propertyPadding(PROPERTY_PADDING)
        , m_editingProperty(-1)
        , m_isEditing(false)
    {
        Engine::Core::Logger::Instance().Debug("PropertiesPanel created");
    }

    PropertiesPanel::~PropertiesPanel()
    {
        Shutdown();
        Engine::Core::Logger::Instance().Debug("PropertiesPanel destroyed");
    }

    bool PropertiesPanel::Initialize()
    {
        // Create default property groups with sample properties
        PropertyGroup transformGroup;
        transformGroup.name = "Transform";
        transformGroup.isExpanded = true;
        transformGroup.isVisible = true;
        
        // Add transform properties
        PropertyValue posX;
        posX.name = "Position X";
        posX.type = PropertyType::Float;
        posX.value = 0.0f;
        posX.displayName = "Position X";
        transformGroup.properties.push_back(posX);
        
        PropertyValue posY;
        posY.name = "Position Y";
        posY.type = PropertyType::Float;
        posY.value = 0.0f;
        posY.displayName = "Position Y";
        transformGroup.properties.push_back(posY);
        
        PropertyValue posZ;
        posZ.name = "Position Z";
        posZ.type = PropertyType::Float;
        posZ.value = 0.0f;
        posZ.displayName = "Position Z";
        transformGroup.properties.push_back(posZ);
        
        PropertyValue rotX;
        rotX.name = "Rotation X";
        rotX.type = PropertyType::Float;
        rotX.value = 0.0f;
        rotX.displayName = "Rotation X";
        transformGroup.properties.push_back(rotX);
        
        PropertyValue rotY;
        rotY.name = "Rotation Y";
        rotY.type = PropertyType::Float;
        rotY.value = 0.0f;
        rotY.displayName = "Rotation Y";
        transformGroup.properties.push_back(rotY);
        
        PropertyValue rotZ;
        rotZ.name = "Rotation Z";
        rotZ.type = PropertyType::Float;
        rotZ.value = 0.0f;
        rotZ.displayName = "Rotation Z";
        transformGroup.properties.push_back(rotZ);
        
        m_propertyGroups.push_back(transformGroup);
        m_groupIndices["Transform"] = 0;
        
        PropertyGroup materialGroup;
        materialGroup.name = "Material";
        materialGroup.isExpanded = true;
        materialGroup.isVisible = true;
        
        // Add material properties
        PropertyValue colorR;
        colorR.name = "Color R";
        colorR.type = PropertyType::Float;
        colorR.value = 1.0f;
        colorR.displayName = "Color R";
        materialGroup.properties.push_back(colorR);
        
        PropertyValue colorG;
        colorG.name = "Color G";
        colorG.type = PropertyType::Float;
        colorG.value = 1.0f;
        colorG.displayName = "Color G";
        materialGroup.properties.push_back(colorG);
        
        PropertyValue colorB;
        colorB.name = "Color B";
        colorB.type = PropertyType::Float;
        colorB.value = 1.0f;
        colorB.displayName = "Color B";
        materialGroup.properties.push_back(colorB);
        
        m_propertyGroups.push_back(materialGroup);
        m_groupIndices["Material"] = 1;
        
        PropertyGroup animationGroup;
        animationGroup.name = "Animation";
        animationGroup.isExpanded = false;
        animationGroup.isVisible = true;
        m_propertyGroups.push_back(animationGroup);
        m_groupIndices["Animation"] = 2;
        
        Engine::Core::Logger::Instance().Info("PropertiesPanel initialized with default properties");
        return true;
    }

    void PropertiesPanel::Shutdown()
    {
        m_propertyGroups.clear();
        m_groupIndices.clear();
        m_selectedObject.reset();
        m_selectedCamera.reset();
        m_selectedLight.reset();
        m_selectedMaterial.reset();
        
        Engine::Core::Logger::Instance().Info("PropertiesPanel shutdown");
    }

    void PropertiesPanel::Update(float deltaTime)
    {
        (void)deltaTime; // Suppress unused parameter warning
        
        // Update properties from selected object if needed
        if (m_selectedObject) {
            UpdatePropertiesFromObject();
        }
    }

    void PropertiesPanel::Render(HDC hdc)
    {
        if (!m_visible) {
            return;
        }

        // Draw background
        HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
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
        DrawText(hdc, L"Properties", -1, &titleRect, DT_LEFT | DT_VCENTER);

        // Render property groups
        RenderPropertyGroups(hdc);
    }

    void PropertiesPanel::HandleMouseEvent(int x, int y, int button, bool pressed)
    {
        // DEBUG: Log all clicks
        static int eventCount = 0;
        if (pressed && button == 0) {
            eventCount++;
            if (eventCount <= 10) {
                std::cout << "[PropertiesPanel] HandleMouseEvent #" << eventCount << ": x=" << x << ", y=" << y 
                          << ", visible=" << m_visible << ", bounds: x=[" << m_x << "," << (m_x + m_width) 
                          << "], y=[" << m_y << "," << (m_y + m_height) << "]" << std::endl;
            }
        }
        
        if (!m_visible) {
            if (pressed && button == 0 && eventCount <= 5) {
                std::cout << "[PropertiesPanel] Rejected: not visible" << std::endl;
            }
            return;
        }

        // Convert coordinates from parent-relative to component-relative
        int localX = x - m_x;
        int localY = y - m_y;
        
        // Check if click is within bounds (using local coordinates)
        if (localX < 0 || localX >= m_width || localY < 0 || localY >= m_height) {
            if (pressed && button == 0 && eventCount <= 5) {
                std::cout << "[PropertiesPanel] Click outside bounds: localX=" << localX << " not in [0," << m_width 
                          << "], localY=" << localY << " not in [0," << m_height << "] (received x=" << x << ", y=" << y 
                          << ", m_x=" << m_x << ", m_y=" << m_y << ")" << std::endl;
            }
            return;
        }

        if (button == 0 && pressed) { // Left mouse button press
            if (eventCount <= 10) {
                std::cout << "[PropertiesPanel] Click ACCEPTED, calling HandlePropertyClick(" << localX << "," << localY << ")" << std::endl;
            }
            HandlePropertyClick(localX, localY);
        }
    }

    void PropertiesPanel::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam)
    {
        (void)lParam; // Suppress unused parameter warning
        
        if (!m_visible) {
            return;
        }

        if (message == WM_KEYDOWN) {
            if (m_isEditing) {
                switch (wParam) {
                    case VK_RETURN:
                        FinishPropertyEdit();
                        break;
                    case VK_ESCAPE:
                        CancelPropertyEdit();
                        break;
                }
            }
        } else if (message == WM_CHAR) {
            if (m_isEditing) {
                char c = static_cast<char>(wParam);
                if (c >= 32 && c <= 126) { // Printable characters
                    m_editingText += c;
                } else if (c == 8) { // Backspace
                    if (!m_editingText.empty()) {
                        m_editingText.pop_back();
                    }
                }
            }
        }
    }

    void PropertiesPanel::SetSelectedObject(std::shared_ptr<Core::SceneObject> object)
    {
        m_selectedObject = object;
        m_selectedCamera.reset();
        m_selectedLight.reset();
        m_selectedMaterial.reset();
        
        GenerateObjectProperties();
        
        if (m_objectChangedCallback) {
            m_objectChangedCallback(object);
        }
        
        Engine::Core::Logger::Instance().Info("Selected object: " + (object ? object->GetName() : "None"));
    }

    void PropertiesPanel::SetSelectedCamera(std::shared_ptr<Core::Camera> camera)
    {
        m_selectedCamera = camera;
        m_selectedObject.reset();
        m_selectedLight.reset();
        m_selectedMaterial.reset();
        
        GenerateCameraProperties();
        
        Engine::Core::Logger::Instance().Info("Selected camera: " + std::string(camera ? "Camera" : "None"));
    }

    void PropertiesPanel::SetSelectedLight(std::shared_ptr<Core::Light> light)
    {
        m_selectedLight = light;
        m_selectedObject.reset();
        m_selectedCamera.reset();
        m_selectedMaterial.reset();
        
        GenerateLightProperties();
        
        Engine::Core::Logger::Instance().Info("Selected light: " + (light ? light->GetName() : "None"));
    }

    void PropertiesPanel::SetSelectedMaterial(std::shared_ptr<Core::Material> material)
    {
        m_selectedMaterial = material;
        m_selectedObject.reset();
        m_selectedCamera.reset();
        m_selectedLight.reset();
        
        GenerateMaterialProperties();
        
        Engine::Core::Logger::Instance().Info("Selected material: " + (material ? material->GetName() : "None"));
    }

    void PropertiesPanel::ClearSelection()
    {
        m_selectedObject.reset();
        m_selectedCamera.reset();
        m_selectedLight.reset();
        m_selectedMaterial.reset();
        
        // Clear all properties
        for (auto& group : m_propertyGroups) {
            group.properties.clear();
        }
        
        Engine::Core::Logger::Instance().Info("Selection cleared");
    }

    void PropertiesPanel::AddProperty(const std::string& groupName, const PropertyValue& property)
    {
        auto it = m_groupIndices.find(groupName);
        if (it != m_groupIndices.end()) {
            int groupIndex = it->second;
            m_propertyGroups[groupIndex].properties.push_back(property);
        }
    }

    void PropertiesPanel::RemoveProperty(const std::string& groupName, const std::string& propertyName)
    {
        auto it = m_groupIndices.find(groupName);
        if (it != m_groupIndices.end()) {
            int groupIndex = it->second;
            auto& properties = m_propertyGroups[groupIndex].properties;
            properties.erase(
                std::remove_if(properties.begin(), properties.end(),
                    [&propertyName](const PropertyValue& prop) { return prop.name == propertyName; }),
                properties.end());
        }
    }

    void PropertiesPanel::UpdateProperty(const std::string& groupName, const std::string& propertyName, const PropertyValue& property)
    {
        auto it = m_groupIndices.find(groupName);
        if (it != m_groupIndices.end()) {
            int groupIndex = it->second;
            auto& properties = m_propertyGroups[groupIndex].properties;
            for (auto& prop : properties) {
                if (prop.name == propertyName) {
                    prop = property;
                    break;
                }
            }
        }
    }

    void PropertiesPanel::RefreshProperties()
    {
        if (m_selectedObject) {
            GenerateObjectProperties();
        } else if (m_selectedCamera) {
            GenerateCameraProperties();
        } else if (m_selectedLight) {
            GenerateLightProperties();
        } else if (m_selectedMaterial) {
            GenerateMaterialProperties();
        }
    }

    PropertiesPanel::PropertyValue* PropertiesPanel::GetProperty(const std::string& groupName, const std::string& propertyName)
    {
        auto it = m_groupIndices.find(groupName);
        if (it != m_groupIndices.end()) {
            int groupIndex = it->second;
            auto& properties = m_propertyGroups[groupIndex].properties;
            for (auto& prop : properties) {
                if (prop.name == propertyName) {
                    return &prop;
                }
            }
        }
        return nullptr;
    }

    void PropertiesPanel::SetPropertyValue(const std::string& groupName, const std::string& propertyName, const std::variant<float, int, bool, std::string, DirectX::XMFLOAT3, COLORREF>& value)
    {
        PropertyValue* prop = GetProperty(groupName, propertyName);
        if (prop) {
            prop->value = value;
            UpdateObjectFromProperties();
        }
    }

    std::variant<float, int, bool, std::string, DirectX::XMFLOAT3, COLORREF> PropertiesPanel::GetPropertyValue(const std::string& groupName, const std::string& propertyName)
    {
        PropertyValue* prop = GetProperty(groupName, propertyName);
        if (prop) {
            return prop->value;
        }
        return std::variant<float, int, bool, std::string, DirectX::XMFLOAT3, COLORREF>();
    }

    void PropertiesPanel::AddPropertyGroup(const PropertyGroup& group)
    {
        m_propertyGroups.push_back(group);
        m_groupIndices[group.name] = static_cast<int>(m_propertyGroups.size()) - 1;
    }

    void PropertiesPanel::RemovePropertyGroup(const std::string& groupName)
    {
        auto it = m_groupIndices.find(groupName);
        if (it != m_groupIndices.end()) {
            int groupIndex = it->second;
            m_propertyGroups.erase(m_propertyGroups.begin() + groupIndex);
            m_groupIndices.erase(it);
            
            // Update indices
            for (auto& pair : m_groupIndices) {
                if (pair.second > groupIndex) {
                    pair.second--;
                }
            }
        }
    }

    void PropertiesPanel::SetGroupExpanded(const std::string& groupName, bool expanded)
    {
        auto it = m_groupIndices.find(groupName);
        if (it != m_groupIndices.end()) {
            int groupIndex = it->second;
            m_propertyGroups[groupIndex].isExpanded = expanded;
        }
    }

    void PropertiesPanel::SetGroupVisible(const std::string& groupName, bool visible)
    {
        auto it = m_groupIndices.find(groupName);
        if (it != m_groupIndices.end()) {
            int groupIndex = it->second;
            m_propertyGroups[groupIndex].isVisible = visible;
        }
    }

    void PropertiesPanel::SetPropertyChangedCallback(std::function<void(const std::string&, const std::string&, const PropertyValue&)> callback)
    {
        m_propertyChangedCallback = callback;
    }

    void PropertiesPanel::SetObjectChangedCallback(std::function<void(std::shared_ptr<Core::SceneObject>)> callback)
    {
        m_objectChangedCallback = callback;
    }

    void PropertiesPanel::GenerateObjectProperties()
    {
        if (!m_selectedObject) {
            return;
        }

        // Clear existing properties
        for (auto& group : m_propertyGroups) {
            group.properties.clear();
        }

        // Generate transform properties
        GenerateTransformProperties();
        
        // Generate mesh properties
        GenerateMeshProperties();
        
        // Generate animation properties
        GenerateAnimationProperties();
    }

    void PropertiesPanel::GenerateCameraProperties()
    {
        if (!m_selectedCamera) {
            return;
        }

        // Clear existing properties
        for (auto& group : m_propertyGroups) {
            group.properties.clear();
        }

        // Generate transform properties
        GenerateTransformProperties();
        
        // Generate camera-specific properties
        PropertyGroup cameraGroup;
        cameraGroup.name = "Camera";
        cameraGroup.isExpanded = true;
        
        PropertyValue fov;
        fov.name = "fov";
        fov.displayName = "Field of View";
        fov.type = PropertyType::Float;
        fov.value = m_selectedCamera->GetFieldOfView();
        fov.minValue = 10.0f;
        fov.maxValue = 120.0f;
        fov.step = 1.0f;
        cameraGroup.properties.push_back(fov);
        
        PropertyValue nearPlane;
        nearPlane.name = "nearPlane";
        nearPlane.displayName = "Near Plane";
        nearPlane.type = PropertyType::Float;
        nearPlane.value = m_selectedCamera->GetNearPlane();
        nearPlane.minValue = 0.01f;
        nearPlane.maxValue = 10.0f;
        nearPlane.step = 0.01f;
        cameraGroup.properties.push_back(nearPlane);
        
        PropertyValue farPlane;
        farPlane.name = "farPlane";
        farPlane.displayName = "Far Plane";
        farPlane.type = PropertyType::Float;
        farPlane.value = m_selectedCamera->GetFarPlane();
        farPlane.minValue = 100.0f;
        farPlane.maxValue = 10000.0f;
        farPlane.step = 10.0f;
        cameraGroup.properties.push_back(farPlane);
        
        m_propertyGroups.push_back(cameraGroup);
        m_groupIndices["Camera"] = static_cast<int>(m_propertyGroups.size()) - 1;
    }

    void PropertiesPanel::GenerateLightProperties()
    {
        if (!m_selectedLight) {
            return;
        }

        // Clear existing properties
        for (auto& group : m_propertyGroups) {
            group.properties.clear();
        }

        // Generate transform properties
        GenerateTransformProperties();
        
        // Generate light-specific properties
        PropertyGroup lightGroup;
        lightGroup.name = "Light";
        lightGroup.isExpanded = true;
        
        PropertyValue intensity;
        intensity.name = "intensity";
        intensity.displayName = "Intensity";
        intensity.type = PropertyType::Float;
        intensity.value = m_selectedLight->GetIntensity();
        intensity.minValue = 0.0f;
        intensity.maxValue = 10.0f;
        intensity.step = 0.1f;
        lightGroup.properties.push_back(intensity);
        
        PropertyValue color;
        color.name = "color";
        color.displayName = "Color";
        color.type = PropertyType::Color;
        color.value = m_selectedLight->GetColor();
        lightGroup.properties.push_back(color);
        
        PropertyValue enabled;
        enabled.name = "enabled";
        enabled.displayName = "Enabled";
        enabled.type = PropertyType::Bool;
        enabled.value = m_selectedLight->IsEnabled();
        lightGroup.properties.push_back(enabled);
        
        m_propertyGroups.push_back(lightGroup);
        m_groupIndices["Light"] = static_cast<int>(m_propertyGroups.size()) - 1;
    }

    void PropertiesPanel::GenerateMaterialProperties()
    {
        if (!m_selectedMaterial) {
            return;
        }

        // Clear existing properties
        for (auto& group : m_propertyGroups) {
            group.properties.clear();
        }

        // Generate material-specific properties
        PropertyGroup materialGroup;
        materialGroup.name = "Material";
        materialGroup.isExpanded = true;
        
        PropertyValue albedo;
        albedo.name = "albedo";
        albedo.displayName = "Albedo";
        albedo.type = PropertyType::Color;
        albedo.value = m_selectedMaterial->GetAlbedo();
        materialGroup.properties.push_back(albedo);
        
        PropertyValue metallic;
        metallic.name = "metallic";
        metallic.displayName = "Metallic";
        metallic.type = PropertyType::Float;
        metallic.value = m_selectedMaterial->GetMetallic();
        metallic.minValue = 0.0f;
        metallic.maxValue = 1.0f;
        metallic.step = 0.01f;
        materialGroup.properties.push_back(metallic);
        
        PropertyValue roughness;
        roughness.name = "roughness";
        roughness.displayName = "Roughness";
        roughness.type = PropertyType::Float;
        roughness.value = m_selectedMaterial->GetRoughness();
        roughness.minValue = 0.0f;
        roughness.maxValue = 1.0f;
        roughness.step = 0.01f;
        materialGroup.properties.push_back(roughness);
        
        m_propertyGroups.push_back(materialGroup);
        m_groupIndices["Material"] = static_cast<int>(m_propertyGroups.size()) - 1;
    }

    void PropertiesPanel::GenerateTransformProperties()
    {
        PropertyGroup* transformGroup = nullptr;
        auto it = m_groupIndices.find("Transform");
        if (it != m_groupIndices.end()) {
            transformGroup = &m_propertyGroups[it->second];
        } else {
            PropertyGroup newGroup;
            newGroup.name = "Transform";
            newGroup.isExpanded = true;
            m_propertyGroups.push_back(newGroup);
            m_groupIndices["Transform"] = static_cast<int>(m_propertyGroups.size()) - 1;
            transformGroup = &m_propertyGroups.back();
        }

        if (m_selectedObject) {
            DirectX::XMFLOAT3 position = m_selectedObject->GetPosition();
            DirectX::XMFLOAT3 rotation = m_selectedObject->GetRotation();
            DirectX::XMFLOAT3 scale = m_selectedObject->GetScale();
            
            PropertyValue pos;
            pos.name = "position";
            pos.displayName = "Position";
            pos.type = PropertyType::Vector3;
            pos.value = position;
            transformGroup->properties.push_back(pos);
            
            PropertyValue rot;
            rot.name = "rotation";
            rot.displayName = "Rotation";
            rot.type = PropertyType::Vector3;
            rot.value = rotation;
            transformGroup->properties.push_back(rot);
            
            PropertyValue scl;
            scl.name = "scale";
            scl.displayName = "Scale";
            scl.type = PropertyType::Vector3;
            scl.value = scale;
            transformGroup->properties.push_back(scl);
        }
    }

    void PropertiesPanel::GenerateMeshProperties()
    {
        if (!m_selectedObject || !m_selectedObject->GetMesh()) {
            return;
        }

        PropertyGroup meshGroup;
        meshGroup.name = "Mesh";
        meshGroup.isExpanded = true;
        
        PropertyValue visible;
        visible.name = "visible";
        visible.displayName = "Visible";
        visible.type = PropertyType::Bool;
        visible.value = m_selectedObject->IsVisible();
        meshGroup.properties.push_back(visible);
        
        m_propertyGroups.push_back(meshGroup);
        m_groupIndices["Mesh"] = static_cast<int>(m_propertyGroups.size()) - 1;
    }

    void PropertiesPanel::GenerateAnimationProperties()
    {
        PropertyGroup animationGroup;
        animationGroup.name = "Animation";
        animationGroup.isExpanded = false;
        
        PropertyValue playAnimation;
        playAnimation.name = "playAnimation";
        playAnimation.displayName = "Play Animation";
        playAnimation.type = PropertyType::Bool;
        playAnimation.value = false;
        animationGroup.properties.push_back(playAnimation);
        
        PropertyValue animationSpeed;
        animationSpeed.name = "animationSpeed";
        animationSpeed.displayName = "Animation Speed";
        animationSpeed.type = PropertyType::Float;
        animationSpeed.value = 1.0f;
        animationSpeed.minValue = 0.0f;
        animationSpeed.maxValue = 5.0f;
        animationSpeed.step = 0.1f;
        animationGroup.properties.push_back(animationSpeed);
        
        m_propertyGroups.push_back(animationGroup);
        m_groupIndices["Animation"] = static_cast<int>(m_propertyGroups.size()) - 1;
    }

    void PropertiesPanel::RenderPropertyGroups(HDC hdc)
    {
        int currentY = m_y + 30;
        
        for (auto& group : m_propertyGroups) {
            if (!group.isVisible) continue;
            
            RenderPropertyGroup(hdc, group, currentY);
            currentY += GROUP_SPACING; // Add spacing between groups
        }
    }

    void PropertiesPanel::RenderPropertyGroup(HDC hdc, PropertyGroup& group, int& y)
    {
        // Draw group header
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT headerRect = { m_x + 5, y, m_x + m_width - 5, y + m_groupHeaderHeight };
        FillRect(hdc, &headerRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw group name
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT nameRect = { m_x + 10, y, m_x + m_width - 10, y + m_groupHeaderHeight };
        std::wstring wideName(group.name.begin(), group.name.end());
        DrawText(hdc, wideName.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER);
        
        // Draw expand/collapse arrow
        std::string arrow = group.isExpanded ? "▼" : "▶";
        std::wstring wideArrow(arrow.begin(), arrow.end());
        RECT arrowRect = { m_x + m_width - 25, y, m_x + m_width - 5, y + m_groupHeaderHeight };
        DrawText(hdc, wideArrow.c_str(), -1, &arrowRect, DT_CENTER | DT_VCENTER);
        
        y += m_groupHeaderHeight;
        
        // Draw properties if expanded
        if (group.isExpanded) {
            for (auto& property : group.properties) {
                if (!property.isVisible) continue;
                
                RenderProperty(hdc, property, m_x + 10, y, m_width - 20, m_itemHeight);
                y += m_itemHeight + 2;
            }
        }
    }

    void PropertiesPanel::RenderProperty(HDC hdc, PropertyValue& property, int x, int y, int width, int height)
    {
        // Draw property name
        RECT nameRect = { x, y, x + width / 2, y + height };
        ::SetTextColor(hdc, RGB(200, 200, 200));
        SetBkMode(hdc, TRANSPARENT);
        std::wstring wideName(property.displayName.begin(), property.displayName.end());
        DrawText(hdc, wideName.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER);
        
        // Draw property value
        RECT valueRect = { x + width / 2, y, x + width, y + height };
        RenderPropertyValue(hdc, property, valueRect.left, valueRect.top, valueRect.right - valueRect.left, valueRect.bottom - valueRect.top);
    }

    void PropertiesPanel::RenderPropertyValue(HDC hdc, PropertyValue& property, int x, int y, int width, int height)
    {
        if (m_isEditing && m_editingGroup == property.name) {
            // Draw editing text
            ::SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            RECT editRect = { x, y, x + width, y + height };
            std::wstring wideText(m_editingText.begin(), m_editingText.end());
            DrawText(hdc, wideText.c_str(), -1, &editRect, DT_LEFT | DT_VCENTER);
        } else {
            // Draw property value based on type
            switch (property.type) {
                case PropertyType::Float:
                case PropertyType::Int:
                case PropertyType::String:
                    {
                        std::string valueStr = ValueToString(property);
                        std::wstring wideValue(valueStr.begin(), valueStr.end());
                        ::SetTextColor(hdc, RGB(255, 255, 255));
                        SetBkMode(hdc, TRANSPARENT);
                        RECT valueRect = { x, y, x + width, y + height };
                        DrawText(hdc, wideValue.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER);
                    }
                    break;
                    
                case PropertyType::Bool:
                    {
                        bool value = std::get<bool>(property.value);
                        std::string valueStr = value ? "True" : "False";
                        std::wstring wideValue(valueStr.begin(), valueStr.end());
                        ::SetTextColor(hdc, RGB(255, 255, 255));
                        SetBkMode(hdc, TRANSPARENT);
                        RECT valueRect = { x, y, x + width, y + height };
                        DrawText(hdc, wideValue.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER);
                    }
                    break;
                    
                case PropertyType::Vector3:
                    {
                        DirectX::XMFLOAT3 value = std::get<DirectX::XMFLOAT3>(property.value);
                        DrawVector3Editor(hdc, x, y, width, height, value);
                    }
                    break;
                    
                case PropertyType::Color:
                    {
                        COLORREF value = std::get<COLORREF>(property.value);
                        DrawColorPicker(hdc, x, y, width, height, value);
                    }
                    break;
                    
                case PropertyType::Enum:
                    {
                        // TODO: Implement enum dropdown
                        std::string valueStr = ValueToString(property);
                        std::wstring wideValue(valueStr.begin(), valueStr.end());
                        ::SetTextColor(hdc, RGB(255, 255, 255));
                        SetBkMode(hdc, TRANSPARENT);
                        RECT valueRect = { x, y, x + width, y + height };
                        DrawText(hdc, wideValue.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER);
                    }
                    break;
                    
                case PropertyType::Button:
                    {
                        DrawButton(hdc, x, y, width, height, property.displayName);
                    }
                    break;
            }
        }
    }

    void PropertiesPanel::HandlePropertyClick(int x, int y)
    {
        // x, y are already local coordinates relative to PropertiesPanel
        std::cout << "[PropertiesPanel] ========================================" << std::endl;
        std::cout << "[PropertiesPanel] HandlePropertyClick START: x=" << x << ", y=" << y << std::endl;
        std::cout << "[PropertiesPanel] PropertyGroups count: " << m_propertyGroups.size() << std::endl;
        
        // Check if clicking on group header
        int currentY = 30; // Start below title (local coordinates start at 0)
        std::cout << "[PropertiesPanel] Starting Y position: " << currentY << std::endl;
        
        for (size_t g = 0; g < m_propertyGroups.size(); ++g) {
            auto& group = m_propertyGroups[g];
            std::cout << "[PropertiesPanel] Checking group #" << g << ": '" << group.name 
                      << "' (visible=" << group.isVisible << ", expanded=" << group.isExpanded 
                      << ", properties=" << group.properties.size() << ")" << std::endl;
            
            if (!group.isVisible) {
                std::cout << "[PropertiesPanel]   Group not visible, skipping" << std::endl;
                continue;
            }
            
            std::cout << "[PropertiesPanel]   Group header at Y: [" << currentY << "," << (currentY + m_groupHeaderHeight) << "]" << std::endl;
            
            if (y >= currentY && y <= currentY + m_groupHeaderHeight) {
                std::cout << "[PropertiesPanel] ✓ CLICK ON GROUP HEADER: '" << group.name << "'" << std::endl;
                group.isExpanded = !group.isExpanded;
                std::cout << "[PropertiesPanel]   Toggled to " << (group.isExpanded ? "EXPANDED" : "COLLAPSED") << std::endl;
                Engine::Core::Logger::Instance().Info("PropertiesPanel: Toggled group '" + group.name + "'");
                return;
            }
            currentY += m_groupHeaderHeight;
            
            if (group.isExpanded) {
                std::cout << "[PropertiesPanel]   Group is expanded, checking " << group.properties.size() << " properties..." << std::endl;
                for (size_t p = 0; p < group.properties.size(); ++p) {
                    auto& property = group.properties[p];
                    if (!property.isVisible) {
                        std::cout << "[PropertiesPanel]     Property #" << p << " '" << property.name << "' not visible, skipping" << std::endl;
                        continue;
                    }
                    
                    std::cout << "[PropertiesPanel]     Property #" << p << ": '" << property.name 
                              << "' at Y: [" << currentY << "," << (currentY + m_itemHeight) << "]" << std::endl;
                    
                    if (y >= currentY && y <= currentY + m_itemHeight) {
                        std::cout << "[PropertiesPanel] ✓ CLICK ON PROPERTY: '" << property.name << "' in group '" << group.name << "'" << std::endl;
                        Engine::Core::Logger::Instance().Info("PropertiesPanel: Clicked property '" + property.name + "'");
                        HandlePropertyEdit(x, y);
                        return;
                    }
                    currentY += m_itemHeight + 2;
                }
            } else {
                std::cout << "[PropertiesPanel]   Group is collapsed, skipping properties" << std::endl;
            }
        }
        
        std::cout << "[PropertiesPanel] ✗ Click at (" << x << "," << y << ") didn't hit any group header or property" << std::endl;
        std::cout << "[PropertiesPanel] Final Y position: " << currentY << std::endl;
        std::cout << "[PropertiesPanel] HandlePropertyClick END" << std::endl;
        std::cout << "[PropertiesPanel] ========================================" << std::endl;
    }

    void PropertiesPanel::HandlePropertyEdit(int x, int y)
    {
        // Find which property was clicked and start editing
        int currentY = 30;
        for (auto& group : m_propertyGroups) {
            if (!group.isVisible) continue;
            currentY += m_groupHeaderHeight;
            
            if (group.isExpanded) {
                for (auto& property : group.properties) {
                    if (!property.isVisible) continue;
                    
                    if (y >= currentY && y <= currentY + m_itemHeight) {
                        if (!property.isReadOnly) {
                            StartPropertyEdit(property, group.name, property.name);
                            std::cout << "[PropertiesPanel] Started editing property '" << property.name << "'" << std::endl;
                            Engine::Core::Logger::Instance().Info("PropertiesPanel: Started editing property '" + property.name + "'");
                        } else {
                            std::cout << "[PropertiesPanel] Property '" << property.name << "' is read-only" << std::endl;
                        }
                        return;
                    }
                    currentY += m_itemHeight + 2;
                }
            }
        }
        
        (void)x; (void)y; // Suppress unused parameter warnings
    }

    void PropertiesPanel::StartPropertyEdit(PropertyValue& property, const std::string& groupName, const std::string& propertyName)
    {
        m_isEditing = true;
        m_editingGroup = groupName;
        m_editingPropertyName = propertyName;
        m_editingText = ValueToString(property);
    }

    void PropertiesPanel::FinishPropertyEdit()
    {
        if (m_isEditing) {
            // Update property value
            auto it = m_groupIndices.find(m_editingGroup);
            if (it != m_groupIndices.end()) {
                int groupIndex = it->second;
                auto& properties = m_propertyGroups[groupIndex].properties;
                for (auto& prop : properties) {
                    if (prop.name == m_editingPropertyName) {
                        StringToValue(prop, m_editingText);
                        UpdateObjectFromProperties();
                        
                        if (m_propertyChangedCallback) {
                            m_propertyChangedCallback(m_editingGroup, m_editingPropertyName, prop);
                        }
                        break;
                    }
                }
            }
            
            m_isEditing = false;
            m_editingGroup.clear();
            m_editingPropertyName.clear();
            m_editingText.clear();
        }
    }

    void PropertiesPanel::CancelPropertyEdit()
    {
        m_isEditing = false;
        m_editingGroup.clear();
        m_editingPropertyName.clear();
        m_editingText.clear();
    }

    void PropertiesPanel::UpdateObjectFromProperties()
    {
        if (!m_selectedObject) {
            return;
        }

        // Update transform properties
        auto it = m_groupIndices.find("Transform");
        if (it != m_groupIndices.end()) {
            int groupIndex = it->second;
            auto& properties = m_propertyGroups[groupIndex].properties;
            
            for (auto& prop : properties) {
                if (prop.name == "position") {
                    DirectX::XMFLOAT3 position = std::get<DirectX::XMFLOAT3>(prop.value);
                    m_selectedObject->SetPosition(position);
                } else if (prop.name == "rotation") {
                    DirectX::XMFLOAT3 rotation = std::get<DirectX::XMFLOAT3>(prop.value);
                    m_selectedObject->SetRotation(rotation);
                } else if (prop.name == "scale") {
                    DirectX::XMFLOAT3 scale = std::get<DirectX::XMFLOAT3>(prop.value);
                    m_selectedObject->SetScale(scale);
                }
            }
        }
    }

    void PropertiesPanel::UpdatePropertiesFromObject()
    {
        if (!m_selectedObject) {
            return;
        }

        // Update transform properties
        auto it = m_groupIndices.find("Transform");
        if (it != m_groupIndices.end()) {
            int groupIndex = it->second;
            auto& properties = m_propertyGroups[groupIndex].properties;
            
            for (auto& prop : properties) {
                if (prop.name == "position") {
                    prop.value = m_selectedObject->GetPosition();
                } else if (prop.name == "rotation") {
                    prop.value = m_selectedObject->GetRotation();
                } else if (prop.name == "scale") {
                    prop.value = m_selectedObject->GetScale();
                }
            }
        }
    }

    std::string PropertiesPanel::ValueToString(const PropertyValue& property)
    {
        switch (property.type) {
            case PropertyType::Float:
                return std::to_string(std::get<float>(property.value));
            case PropertyType::Int:
                return std::to_string(std::get<int>(property.value));
            case PropertyType::Bool:
                return std::get<bool>(property.value) ? "True" : "False";
            case PropertyType::String:
                return std::get<std::string>(property.value);
            case PropertyType::Vector3:
                {
                    DirectX::XMFLOAT3 v = std::get<DirectX::XMFLOAT3>(property.value);
                    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
                }
            case PropertyType::Color:
                {
                    COLORREF c = std::get<COLORREF>(property.value);
                    return "RGB(" + std::to_string(GetRValue(c)) + ", " + std::to_string(GetGValue(c)) + ", " + std::to_string(GetBValue(c)) + ")";
                }
            default:
                return "";
        }
    }

    void PropertiesPanel::StringToValue(PropertyValue& property, const std::string& value)
    {
        switch (property.type) {
            case PropertyType::Float:
                property.value = std::stof(value);
                break;
            case PropertyType::Int:
                property.value = std::stoi(value);
                break;
            case PropertyType::Bool:
                property.value = (value == "True" || value == "true" || value == "1");
                break;
            case PropertyType::String:
                property.value = value;
                break;
            case PropertyType::Vector3:
                {
                    // Parse "(x, y, z)" format
                    if (value.length() > 2 && value[0] == '(' && value[value.length()-1] == ')') {
                        std::string content = value.substr(1, value.length()-2);
                        std::stringstream ss(content);
                        std::string xStr, yStr, zStr;
                        std::getline(ss, xStr, ',');
                        std::getline(ss, yStr, ',');
                        std::getline(ss, zStr);
                        
                        DirectX::XMFLOAT3 v;
                        v.x = std::stof(xStr);
                        v.y = std::stof(yStr);
                        v.z = std::stof(zStr);
                        property.value = v;
                    }
                }
                break;
            case PropertyType::Color:
                {
                    // Parse "RGB(r, g, b)" format
                    if (value.length() > 5 && value.substr(0, 4) == "RGB(" && value[value.length()-1] == ')') {
                        std::string content = value.substr(4, value.length()-5);
                        std::stringstream ss(content);
                        std::string rStr, gStr, bStr;
                        std::getline(ss, rStr, ',');
                        std::getline(ss, gStr, ',');
                        std::getline(ss, bStr);
                        
                        COLORREF c = RGB(std::stoi(rStr), std::stoi(gStr), std::stoi(bStr));
                        property.value = c;
                    }
                }
                break;
        }
    }

    void PropertiesPanel::DrawSlider(HDC hdc, int x, int y, int width, int height, float value, float min, float max)
    {
        // Draw slider background
        HBRUSH hBrush = CreateSolidBrush(RGB(80, 80, 80));
        RECT sliderRect = { x, y, x + width, y + height };
        FillRect(hdc, &sliderRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw slider handle
        float normalizedValue = (value - min) / (max - min);
        int handleX = x + static_cast<int>(normalizedValue * (width - 10));
        HBRUSH handleBrush = CreateSolidBrush(RGB(100, 150, 255));
        RECT handleRect = { handleX, y, handleX + 10, y + height };
        FillRect(hdc, &handleRect, handleBrush);
        DeleteObject(handleBrush);
    }

    void PropertiesPanel::DrawColorPicker(HDC hdc, int x, int y, int width, int height, COLORREF color)
    {
        // Draw color rectangle
        HBRUSH hBrush = CreateSolidBrush(color);
        RECT colorRect = { x, y, x + COLOR_PICKER_SIZE, y + height };
        FillRect(hdc, &colorRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw color text
        std::string colorStr = "RGB(" + std::to_string(GetRValue(color)) + ", " + std::to_string(GetGValue(color)) + ", " + std::to_string(GetBValue(color)) + ")";
        std::wstring wideColor(colorStr.begin(), colorStr.end());
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT textRect = { x + COLOR_PICKER_SIZE + 5, y, x + width, y + height };
        DrawText(hdc, wideColor.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
    }

    void PropertiesPanel::DrawVector3Editor(HDC hdc, int x, int y, int width, int height, const DirectX::XMFLOAT3& value)
    {
        (void)width; // Suppress unused parameter warning
        int componentWidth = VECTOR3_COMPONENT_WIDTH;
        int spacing = 5;
        
        // Draw X component
        RECT xRect = { x, y, x + componentWidth, y + height };
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        FillRect(hdc, &xRect, hBrush);
        DeleteObject(hBrush);
        
        std::string xStr = "X: " + std::to_string(value.x);
        std::wstring wideX(xStr.begin(), xStr.end());
        ::SetTextColor(hdc, RGB(255, 100, 100));
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, wideX.c_str(), -1, &xRect, DT_CENTER | DT_VCENTER);
        
        // Draw Y component
        RECT yRect = { x + componentWidth + spacing, y, x + componentWidth * 2 + spacing, y + height };
        hBrush = CreateSolidBrush(RGB(60, 60, 60));
        FillRect(hdc, &yRect, hBrush);
        DeleteObject(hBrush);
        
        std::string yStr = "Y: " + std::to_string(value.y);
        std::wstring wideY(yStr.begin(), yStr.end());
        ::SetTextColor(hdc, RGB(100, 255, 100));
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, wideY.c_str(), -1, &yRect, DT_CENTER | DT_VCENTER);
        
        // Draw Z component
        RECT zRect = { x + componentWidth * 2 + spacing * 2, y, x + componentWidth * 3 + spacing * 2, y + height };
        hBrush = CreateSolidBrush(RGB(60, 60, 60));
        FillRect(hdc, &zRect, hBrush);
        DeleteObject(hBrush);
        
        std::string zStr = "Z: " + std::to_string(value.z);
        std::wstring wideZ(zStr.begin(), zStr.end());
        ::SetTextColor(hdc, RGB(100, 100, 255));
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, wideZ.c_str(), -1, &zRect, DT_CENTER | DT_VCENTER);
    }

    void PropertiesPanel::DrawEnumDropdown(HDC hdc, int x, int y, int width, int height, const std::vector<std::string>& options, int selected)
    {
        // Draw dropdown background
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT dropdownRect = { x, y, x + width, y + height };
        FillRect(hdc, &dropdownRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw selected option
        if (selected >= 0 && selected < static_cast<int>(options.size())) {
            std::wstring wideOption(options[selected].begin(), options[selected].end());
            ::SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            RECT textRect = { x + 5, y, x + width - 20, y + height };
            DrawText(hdc, wideOption.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
        }
        
        // Draw dropdown arrow
        std::wstring arrow = L"▼";
        RECT arrowRect = { x + width - 20, y, x + width, y + height };
        DrawText(hdc, arrow.c_str(), -1, &arrowRect, DT_CENTER | DT_VCENTER);
    }

    void PropertiesPanel::DrawButton(HDC hdc, int x, int y, int width, int height, const std::string& text)
    {
        // Draw button background
        HBRUSH hBrush = CreateSolidBrush(RGB(80, 120, 80));
        RECT buttonRect = { x, y, x + width, y + height };
        FillRect(hdc, &buttonRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw button text
        std::wstring wideText(text.begin(), text.end());
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, wideText.c_str(), -1, &buttonRect, DT_CENTER | DT_VCENTER);
    }

    bool PropertiesPanel::IsPointInRect(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight)
    {
        return x >= rectX && x <= rectX + rectWidth && y >= rectY && y <= rectY + rectHeight;
    }

    int PropertiesPanel::GetPropertyIndexFromPosition(int x, int y)
    {
        // TODO: Implement property index calculation
        (void)x; (void)y; // Suppress unused parameter warnings
        return -1;
    }

    PropertiesPanel::PropertyValue* PropertiesPanel::GetPropertyFromPosition(int x, int y, std::string& groupName, std::string& propertyName)
    {
        // TODO: Implement property lookup from position
        (void)x; (void)y; (void)groupName; (void)propertyName; // Suppress unused parameter warnings
        return nullptr;
    }

} // namespace UI
} // namespace Engine
