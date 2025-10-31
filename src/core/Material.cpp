#include "core/Material.h"
#include "core/Logger.h"
#include "core/ResourceManager.h"
#include <algorithm>
#include <filesystem>

namespace Engine {
namespace Core {

Material::Material(const std::string& name)
    : m_name(name)
    , m_albedo(0.8f, 0.8f, 0.8f)
    , m_metallic(0.0f)
    , m_roughness(0.5f)
    , m_specular(0.5f)
    , m_emission(0.0f)
    , m_opacity(1.0f)
    , m_shaderName("PBR")
    , m_materialTemplate("Default")
    , m_isCinematic(false)
{
    Logger::Instance().Debug("Material created: " + m_name);
}

Material::~Material()
{
    Logger::Instance().Debug("Material destroyed: " + m_name);
}

void Material::SetCustomProperty(const std::string& name, float value)
{
    m_customProperties[name] = value;
}

float Material::GetCustomProperty(const std::string& name) const
{
    auto it = m_customProperties.find(name);
    return (it != m_customProperties.end()) ? it->second : 0.0f;
}

bool Material::HasCustomProperty(const std::string& name) const
{
    return m_customProperties.find(name) != m_customProperties.end();
}

void Material::Update(float deltaTime)
{
    (void)deltaTime; // Suppress unused parameter warning
    // Update material-specific logic here
    // For example, animated materials, time-based effects, etc.
}

void Material::Apply(class Renderer* renderer)
{
    (void)renderer; // Suppress unused parameter warning
    // Apply material properties to the renderer
    // This would set shader parameters, bind textures, etc.
}

std::shared_ptr<Material> Material::CreateMetal(const std::string& name, const DirectX::XMFLOAT3& color)
{
    auto material = std::make_shared<Material>(name);
    material->SetAlbedo(color);
    material->SetMetallic(1.0f);
    material->SetRoughness(0.1f);
    material->SetSpecular(0.9f);
    material->SetMaterialTemplate("Metal");
    return material;
}

std::shared_ptr<Material> Material::CreatePlastic(const std::string& name, const DirectX::XMFLOAT3& color)
{
    auto material = std::make_shared<Material>(name);
    material->SetAlbedo(color);
    material->SetMetallic(0.0f);
    material->SetRoughness(0.3f);
    material->SetSpecular(0.5f);
    material->SetMaterialTemplate("Plastic");
    return material;
}

std::shared_ptr<Material> Material::CreateGlass(const std::string& name, const DirectX::XMFLOAT3& color)
{
    auto material = std::make_shared<Material>(name);
    material->SetAlbedo(color);
    material->SetMetallic(0.0f);
    material->SetRoughness(0.0f);
    material->SetSpecular(0.9f);
    material->SetOpacity(0.8f);
    material->SetMaterialTemplate("Glass");
    return material;
}

std::shared_ptr<Material> Material::CreateFabric(const std::string& name, const DirectX::XMFLOAT3& color)
{
    auto material = std::make_shared<Material>(name);
    material->SetAlbedo(color);
    material->SetMetallic(0.0f);
    material->SetRoughness(0.8f);
    material->SetSpecular(0.1f);
    material->SetMaterialTemplate("Fabric");
    return material;
}

std::shared_ptr<Material> Material::CreateWood(const std::string& name, const DirectX::XMFLOAT3& color)
{
    auto material = std::make_shared<Material>(name);
    material->SetAlbedo(color);
    material->SetMetallic(0.0f);
    material->SetRoughness(0.7f);
    material->SetSpecular(0.2f);
    material->SetMaterialTemplate("Wood");
    return material;
}

bool Material::LoadDiffuseTexture(const std::string& filePath, ResourceManager* resourceMgr)
{
    if (!resourceMgr) {
        Logger::Instance().Error("Material::LoadDiffuseTexture: ResourceManager is null");
        return false;
    }

    // Extract texture name from file path
    std::filesystem::path path(filePath);
    std::string textureName = path.stem().string();

    // Load texture via ResourceManager
    if (resourceMgr->LoadTexture(filePath, textureName)) {
        m_diffuseTexture = textureName; // Store texture name (used to retrieve from ResourceManager)
        Logger::Instance().Info("Material '" + m_name + "': Loaded diffuse texture: " + textureName);
        return true;
    }

    Logger::Instance().Error("Material '" + m_name + "': Failed to load diffuse texture: " + filePath);
    return false;
}

// MaterialManager Implementation

MaterialManager::MaterialManager()
{
    Logger::Instance().Debug("MaterialManager created");
}

MaterialManager::~MaterialManager()
{
    m_materials.clear();
    Logger::Instance().Debug("MaterialManager destroyed");
}

std::shared_ptr<Material> MaterialManager::CreateMaterial(const std::string& name)
{
    auto material = std::make_shared<Material>(name);
    AddMaterial(material);
    return material;
}

void MaterialManager::AddMaterial(std::shared_ptr<Material> material)
{
    if (!material) {
        Logger::Instance().Warning("Cannot add null material");
        return;
    }

    m_materials[material->GetName()] = material;
    Logger::Instance().Info("Added material: " + material->GetName());
}

void MaterialManager::RemoveMaterial(const std::string& name)
{
    auto it = m_materials.find(name);
    if (it != m_materials.end()) {
        m_materials.erase(it);
        Logger::Instance().Info("Removed material: " + name);
    }
}

std::shared_ptr<Material> MaterialManager::GetMaterial(const std::string& name)
{
    auto it = m_materials.find(name);
    return (it != m_materials.end()) ? it->second : nullptr;
}

std::shared_ptr<Material> MaterialManager::CreateDramaticMaterial(const std::string& name)
{
    auto material = CreateMaterial(name);
    material->SetAlbedo(0.2f, 0.1f, 0.3f); // Dark purple
    material->SetMetallic(0.8f);
    material->SetRoughness(0.2f);
    material->SetEmission(0.1f);
    material->SetMaterialTemplate("Dramatic");
    material->SetCinematic(true);
    return material;
}

std::shared_ptr<Material> MaterialManager::CreateRomanticMaterial(const std::string& name)
{
    auto material = CreateMaterial(name);
    material->SetAlbedo(0.9f, 0.7f, 0.8f); // Soft pink
    material->SetMetallic(0.1f);
    material->SetRoughness(0.4f);
    material->SetMaterialTemplate("Romantic");
    material->SetCinematic(true);
    return material;
}

std::shared_ptr<Material> MaterialManager::CreateActionMaterial(const std::string& name)
{
    auto material = CreateMaterial(name);
    material->SetAlbedo(0.8f, 0.2f, 0.2f); // Bright red
    material->SetMetallic(0.9f);
    material->SetRoughness(0.1f);
    material->SetEmission(0.2f);
    material->SetMaterialTemplate("Action");
    material->SetCinematic(true);
    return material;
}

std::shared_ptr<Material> MaterialManager::CreateSciFiMaterial(const std::string& name)
{
    auto material = CreateMaterial(name);
    material->SetAlbedo(0.1f, 0.8f, 0.9f); // Cyan
    material->SetMetallic(1.0f);
    material->SetRoughness(0.0f);
    material->SetEmission(0.5f);
    material->SetMaterialTemplate("SciFi");
    material->SetCinematic(true);
    return material;
}

std::shared_ptr<Material> MaterialManager::CreateFantasyMaterial(const std::string& name)
{
    auto material = CreateMaterial(name);
    material->SetAlbedo(0.6f, 0.4f, 0.8f); // Purple
    material->SetMetallic(0.3f);
    material->SetRoughness(0.6f);
    material->SetEmission(0.3f);
    material->SetMaterialTemplate("Fantasy");
    material->SetCinematic(true);
    return material;
}

void MaterialManager::Update(float deltaTime)
{
    for (auto& material : m_materials) {
        if (material.second) {
            material.second->Update(deltaTime);
        }
    }
}

void MaterialManager::ApplyMaterials(class Renderer* renderer)
{
    for (auto& material : m_materials) {
        if (material.second) {
            material.second->Apply(renderer);
        }
    }
}

void MaterialManager::ApplyMaterialTemplate(std::shared_ptr<Material> material, const std::string& templateName)
{
    if (!material) {
        return;
    }

    if (templateName == "Dramatic") {
        material->SetAlbedo(0.2f, 0.1f, 0.3f);
        material->SetMetallic(0.8f);
        material->SetRoughness(0.2f);
        material->SetEmission(0.1f);
    }
    else if (templateName == "Romantic") {
        material->SetAlbedo(0.9f, 0.7f, 0.8f);
        material->SetMetallic(0.1f);
        material->SetRoughness(0.4f);
    }
    else if (templateName == "Action") {
        material->SetAlbedo(0.8f, 0.2f, 0.2f);
        material->SetMetallic(0.9f);
        material->SetRoughness(0.1f);
        material->SetEmission(0.2f);
    }
    else if (templateName == "SciFi") {
        material->SetAlbedo(0.1f, 0.8f, 0.9f);
        material->SetMetallic(1.0f);
        material->SetRoughness(0.0f);
        material->SetEmission(0.5f);
    }
    else if (templateName == "Fantasy") {
        material->SetAlbedo(0.6f, 0.4f, 0.8f);
        material->SetMetallic(0.3f);
        material->SetRoughness(0.6f);
        material->SetEmission(0.3f);
    }
}

} // namespace Core
} // namespace Engine
