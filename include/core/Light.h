#pragma once

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>

namespace Engine {
namespace Core {

/**
 * @brief Light types for the AI Movie Studio
 */
enum class LightType {
    Directional,  // Sun light
    Point,        // Light bulb
    Spot,         // Flashlight
    Area,         // Soft area light
    Environment   // Sky/ambient light
};

/**
 * @brief Light attenuation for point and spot lights
 */
struct LightAttenuation {
    float constant;
    float linear;
    float quadratic;
    
    LightAttenuation() : constant(1.0f), linear(0.09f), quadratic(0.032f) {}
    LightAttenuation(float c, float l, float q) : constant(c), linear(l), quadratic(q) {}
};

/**
 * @brief Light class for AI Movie Studio
 * 
 * Supports all major light types needed for movie production:
 * - Directional (sun, moon)
 * - Point (light bulbs, candles)
 * - Spot (flashlights, stage lights)
 * - Area (soft lighting)
 * - Environment (sky, ambient)
 */
class Light {
public:
    Light(const std::string& name = "Light", LightType type = LightType::Point);
    ~Light();

    // Basic properties
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    LightType GetType() const { return m_type; }
    void SetType(LightType type) { m_type = type; }

    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    // Position and direction
    const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
    void SetPosition(const DirectX::XMFLOAT3& position) { m_position = position; }
    void SetPosition(float x, float y, float z) { m_position = {x, y, z}; }

    const DirectX::XMFLOAT3& GetDirection() const { return m_direction; }
    void SetDirection(const DirectX::XMFLOAT3& direction) { m_direction = direction; }
    void SetDirection(float x, float y, float z) { m_direction = {x, y, z}; }

    // Color and intensity
    const DirectX::XMFLOAT3& GetColor() const { return m_color; }
    void SetColor(const DirectX::XMFLOAT3& color) { m_color = color; }
    void SetColor(float r, float g, float b) { m_color = {r, g, b}; }

    float GetIntensity() const { return m_intensity; }
    void SetIntensity(float intensity) { m_intensity = std::max(0.0f, intensity); }

    // Range and attenuation (for point and spot lights)
    float GetRange() const { return m_range; }
    void SetRange(float range) { m_range = std::max(0.0f, range); }

    const LightAttenuation& GetAttenuation() const { return m_attenuation; }
    void SetAttenuation(const LightAttenuation& attenuation) { m_attenuation = attenuation; }

    // Spot light specific
    float GetInnerConeAngle() const { return m_innerConeAngle; }
    void SetInnerConeAngle(float angle) { m_innerConeAngle = std::clamp(angle, 0.0f, m_outerConeAngle); }

    float GetOuterConeAngle() const { return m_outerConeAngle; }
    void SetOuterConeAngle(float angle) { m_outerConeAngle = std::clamp(angle, m_innerConeAngle, 180.0f); }

    // Area light specific
    float GetAreaWidth() const { return m_areaWidth; }
    void SetAreaWidth(float width) { m_areaWidth = std::max(0.0f, width); }

    float GetAreaHeight() const { return m_areaHeight; }
    void SetAreaHeight(float height) { m_areaHeight = std::max(0.0f, height); }

    // Shadow properties
    bool CastsShadows() const { return m_castsShadows; }
    void SetCastsShadows(bool castsShadows) { m_castsShadows = castsShadows; }

    float GetShadowBias() const { return m_shadowBias; }
    void SetShadowBias(float bias) { m_shadowBias = bias; }

    // Movie-specific properties
    bool IsCinematic() const { return m_isCinematic; }
    void SetCinematic(bool cinematic) { m_isCinematic = cinematic; }

    const std::string& GetLightTemplate() const { return m_lightTemplate; }
    void SetLightTemplate(const std::string& lightTemplate) { m_lightTemplate = lightTemplate; }

    // Update and render
    void Update(float deltaTime);
    void Render(class Renderer* renderer);

private:
    std::string m_name;
    LightType m_type;
    bool m_enabled;

    // Transform
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT3 m_direction;

    // Visual properties
    DirectX::XMFLOAT3 m_color;
    float m_intensity;

    // Range and attenuation
    float m_range;
    LightAttenuation m_attenuation;

    // Spot light properties
    float m_innerConeAngle;  // In degrees
    float m_outerConeAngle;  // In degrees

    // Area light properties
    float m_areaWidth;
    float m_areaHeight;

    // Shadow properties
    bool m_castsShadows;
    float m_shadowBias;

    // Movie-specific
    bool m_isCinematic;
    std::string m_lightTemplate;  // "Dramatic", "Romantic", "Action", etc.
};

/**
 * @brief Light manager for AI Movie Studio
 * 
 * Manages all lights in the scene and provides:
 * - Light templates for movie production
 * - Cinematic lighting presets
 * - Performance optimization
 * - AI-friendly light management
 */
class LightManager {
public:
    LightManager();
    ~LightManager();

    // Light management
    std::shared_ptr<Light> CreateLight(const std::string& name, LightType type = LightType::Point);
    void AddLight(std::shared_ptr<Light> light);
    void RemoveLight(std::shared_ptr<Light> light);
    void RemoveLight(const std::string& name);

    std::shared_ptr<Light> GetLight(const std::string& name);
    const std::vector<std::shared_ptr<Light>>& GetAllLights() const { return m_lights; }

    // Light templates for movie production
    std::shared_ptr<Light> CreateDramaticLight(const std::string& name);
    std::shared_ptr<Light> CreateRomanticLight(const std::string& name);
    std::shared_ptr<Light> CreateActionLight(const std::string& name);
    std::shared_ptr<Light> CreateSoftLight(const std::string& name);
    std::shared_ptr<Light> CreateSunLight(const std::string& name);
    std::shared_ptr<Light> CreateMoonLight(const std::string& name);

    // Scene lighting
    void SetAmbientLight(const DirectX::XMFLOAT3& color, float intensity);
    const DirectX::XMFLOAT3& GetAmbientColor() const { return m_ambientColor; }
    float GetAmbientIntensity() const { return m_ambientIntensity; }

    // Performance
    void SetMaxLights(int maxLights) { m_maxLights = maxLights; }
    int GetMaxLights() const { return m_maxLights; }

    // Update and render
    void Update(float deltaTime);
    void Render(class Renderer* renderer);

    // AI-friendly methods
    std::vector<std::shared_ptr<Light>> GetLightsByType(LightType type) const;
    std::vector<std::shared_ptr<Light>> GetCinematicLights() const;
    std::vector<std::shared_ptr<Light>> GetLightsInRange(const DirectX::XMFLOAT3& position, float range) const;

private:
    std::vector<std::shared_ptr<Light>> m_lights;
    DirectX::XMFLOAT3 m_ambientColor;
    float m_ambientIntensity;
    int m_maxLights;

    // Helper methods
    void ApplyLightTemplate(std::shared_ptr<Light> light, const std::string& lightTemplate);
};

} // namespace Core
} // namespace Engine