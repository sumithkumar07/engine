#include "core/Light.h"
#include "core/Logger.h"
#include <algorithm>

namespace Engine {
namespace Core {

Light::Light(const std::string& name, LightType type)
    : m_name(name)
    , m_type(type)
    , m_enabled(true)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_direction(0.0f, -1.0f, 0.0f)
    , m_color(1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
    , m_range(10.0f)
    , m_attenuation()
    , m_innerConeAngle(30.0f)
    , m_outerConeAngle(45.0f)
    , m_areaWidth(1.0f)
    , m_areaHeight(1.0f)
    , m_castsShadows(false)
    , m_shadowBias(0.01f)
    , m_isCinematic(false)
    , m_lightTemplate("Default")
{
    Logger::Instance().Debug("Light created: " + m_name);
}

Light::~Light()
{
    Logger::Instance().Debug("Light destroyed: " + m_name);
}

void Light::Update(float deltaTime)
{
    (void)deltaTime; // Suppress unused parameter warning
    
    // Update light-specific logic here
    // For example, animated lights, flickering, etc.
}

void Light::Render(class Renderer* renderer)
{
    (void)renderer; // Suppress unused parameter warning
    
    // Render light visualization (gizmos, etc.)
    // This would be called by the renderer to draw light icons
}

// LightManager Implementation

LightManager::LightManager()
    : m_ambientColor(0.1f, 0.1f, 0.1f)
    , m_ambientIntensity(0.1f)
    , m_maxLights(32)
{
    Logger::Instance().Debug("LightManager created");
}

LightManager::~LightManager()
{
    m_lights.clear();
    Logger::Instance().Debug("LightManager destroyed");
}

std::shared_ptr<Light> LightManager::CreateLight(const std::string& name, LightType type)
{
    auto light = std::make_shared<Light>(name, type);
    AddLight(light);
    return light;
}

void LightManager::AddLight(std::shared_ptr<Light> light)
{
    if (!light) {
        Logger::Instance().Warning("Cannot add null light");
        return;
    }

    // Check if light with same name already exists
    for (const auto& existingLight : m_lights) {
        if (existingLight && existingLight->GetName() == light->GetName()) {
            Logger::Instance().Warning("Light with name '" + light->GetName() + "' already exists");
            return;
        }
    }

    m_lights.push_back(light);
    Logger::Instance().Info("Added light: " + light->GetName());
}

void LightManager::RemoveLight(std::shared_ptr<Light> light)
{
    if (!light) {
        return;
    }

    auto it = std::find(m_lights.begin(), m_lights.end(), light);
    if (it != m_lights.end()) {
        m_lights.erase(it);
        Logger::Instance().Info("Removed light: " + light->GetName());
    }
}

void LightManager::RemoveLight(const std::string& name)
{
    auto it = std::find_if(m_lights.begin(), m_lights.end(),
        [&name](const std::shared_ptr<Light>& light) {
            return light && light->GetName() == name;
        });

    if (it != m_lights.end()) {
        Logger::Instance().Info("Removed light: " + name);
        m_lights.erase(it);
    }
}

std::shared_ptr<Light> LightManager::GetLight(const std::string& name)
{
    auto it = std::find_if(m_lights.begin(), m_lights.end(),
        [&name](const std::shared_ptr<Light>& light) {
            return light && light->GetName() == name;
        });

    return (it != m_lights.end()) ? *it : nullptr;
}

std::shared_ptr<Light> LightManager::CreateDramaticLight(const std::string& name)
{
    auto light = CreateLight(name, LightType::Directional);
    light->SetColor(1.0f, 0.8f, 0.6f); // Warm orange
    light->SetIntensity(2.0f);
    light->SetDirection(0.3f, -0.8f, 0.5f);
    light->SetCastsShadows(true);
    light->SetLightTemplate("Dramatic");
    light->SetCinematic(true);
    return light;
}

std::shared_ptr<Light> LightManager::CreateRomanticLight(const std::string& name)
{
    auto light = CreateLight(name, LightType::Point);
    light->SetColor(1.0f, 0.9f, 0.8f); // Soft warm
    light->SetIntensity(1.5f);
    light->SetRange(15.0f);
    light->SetCastsShadows(false);
    light->SetLightTemplate("Romantic");
    light->SetCinematic(true);
    return light;
}

std::shared_ptr<Light> LightManager::CreateActionLight(const std::string& name)
{
    auto light = CreateLight(name, LightType::Spot);
    light->SetColor(1.0f, 1.0f, 0.9f); // Bright white
    light->SetIntensity(3.0f);
    light->SetRange(20.0f);
    light->SetInnerConeAngle(15.0f);
    light->SetOuterConeAngle(30.0f);
    light->SetCastsShadows(true);
    light->SetLightTemplate("Action");
    light->SetCinematic(true);
    return light;
}

std::shared_ptr<Light> LightManager::CreateSoftLight(const std::string& name)
{
    auto light = CreateLight(name, LightType::Area);
    light->SetColor(1.0f, 1.0f, 1.0f); // Pure white
    light->SetIntensity(0.8f);
    light->SetAreaWidth(2.0f);
    light->SetAreaHeight(2.0f);
    light->SetCastsShadows(false);
    light->SetLightTemplate("Soft");
    light->SetCinematic(true);
    return light;
}

std::shared_ptr<Light> LightManager::CreateSunLight(const std::string& name)
{
    auto light = CreateLight(name, LightType::Directional);
    light->SetColor(1.0f, 0.95f, 0.8f); // Sun color
    light->SetIntensity(1.2f);
    light->SetDirection(0.2f, -0.9f, 0.3f);
    light->SetCastsShadows(true);
    light->SetLightTemplate("Sun");
    light->SetCinematic(false);
    return light;
}

std::shared_ptr<Light> LightManager::CreateMoonLight(const std::string& name)
{
    auto light = CreateLight(name, LightType::Directional);
    light->SetColor(0.7f, 0.8f, 1.0f); // Cool blue
    light->SetIntensity(0.3f);
    light->SetDirection(-0.1f, -0.8f, 0.6f);
    light->SetCastsShadows(true);
    light->SetLightTemplate("Moon");
    light->SetCinematic(false);
    return light;
}

void LightManager::SetAmbientLight(const DirectX::XMFLOAT3& color, float intensity)
{
    m_ambientColor = color;
    m_ambientIntensity = std::max(0.0f, intensity);
    Logger::Instance().Info("Ambient light set: " + std::to_string(color.x) + ", " + 
                           std::to_string(color.y) + ", " + std::to_string(color.z) + 
                           " intensity: " + std::to_string(intensity));
}

void LightManager::Update(float deltaTime)
{
    for (auto& light : m_lights) {
        if (light) {
            light->Update(deltaTime);
        }
    }
}

void LightManager::Render(class Renderer* renderer)
{
    for (auto& light : m_lights) {
        if (light && light->IsEnabled()) {
            light->Render(renderer);
        }
    }
}

std::vector<std::shared_ptr<Light>> LightManager::GetLightsByType(LightType type) const
{
    std::vector<std::shared_ptr<Light>> result;
    for (const auto& light : m_lights) {
        if (light && light->GetType() == type) {
            result.push_back(light);
        }
    }
    return result;
}

std::vector<std::shared_ptr<Light>> LightManager::GetCinematicLights() const
{
    std::vector<std::shared_ptr<Light>> result;
    for (const auto& light : m_lights) {
        if (light && light->IsCinematic()) {
            result.push_back(light);
        }
    }
    return result;
}

std::vector<std::shared_ptr<Light>> LightManager::GetLightsInRange(const DirectX::XMFLOAT3& position, float range) const
{
    std::vector<std::shared_ptr<Light>> result;
    for (const auto& light : m_lights) {
        if (light && light->IsEnabled()) {
            DirectX::XMFLOAT3 lightPos = light->GetPosition();
            float distance = sqrtf(
                powf(position.x - lightPos.x, 2) +
                powf(position.y - lightPos.y, 2) +
                powf(position.z - lightPos.z, 2)
            );
            
            if (distance <= range) {
                result.push_back(light);
            }
        }
    }
    return result;
}

void LightManager::ApplyLightTemplate(std::shared_ptr<Light> light, const std::string& lightTemplate)
{
    if (!light) {
        return;
    }

    if (lightTemplate == "Dramatic") {
        light->SetColor(1.0f, 0.8f, 0.6f);
        light->SetIntensity(2.0f);
        light->SetCastsShadows(true);
    }
    else if (lightTemplate == "Romantic") {
        light->SetColor(1.0f, 0.9f, 0.8f);
        light->SetIntensity(1.5f);
        light->SetCastsShadows(false);
    }
    else if (lightTemplate == "Action") {
        light->SetColor(1.0f, 1.0f, 0.9f);
        light->SetIntensity(3.0f);
        light->SetCastsShadows(true);
    }
    else if (lightTemplate == "Soft") {
        light->SetColor(1.0f, 1.0f, 1.0f);
        light->SetIntensity(0.8f);
        light->SetCastsShadows(false);
    }
    else if (lightTemplate == "Sun") {
        light->SetColor(1.0f, 0.95f, 0.8f);
        light->SetIntensity(1.2f);
        light->SetCastsShadows(true);
    }
    else if (lightTemplate == "Moon") {
        light->SetColor(0.7f, 0.8f, 1.0f);
        light->SetIntensity(0.3f);
        light->SetCastsShadows(true);
    }
}

} // namespace Core
} // namespace Engine
