#pragma once

#include <memory>
#include <string>
#include <DirectXMath.h>
#include <unordered_map>
#include <algorithm>

namespace Engine {
namespace Core {

/**
 * @brief Material class for AI Movie Studio
 * 
 * Represents material properties for 3D objects including:
 * - PBR (Physically Based Rendering) properties
 * - Textures (diffuse, normal, metallic, roughness, etc.)
 * - Shader parameters
 * - Movie-specific material templates
 */
class Material {
public:
    Material(const std::string& name = "DefaultMaterial");
    ~Material();

    // Basic properties
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    // PBR Properties
    const DirectX::XMFLOAT3& GetAlbedo() const { return m_albedo; }
    void SetAlbedo(const DirectX::XMFLOAT3& albedo) { m_albedo = albedo; }
    void SetAlbedo(float r, float g, float b) { m_albedo = {r, g, b}; }

    float GetMetallic() const { return m_metallic; }
    void SetMetallic(float metallic) { m_metallic = (metallic < 0.0f) ? 0.0f : (metallic > 1.0f) ? 1.0f : metallic; }

    float GetRoughness() const { return m_roughness; }
    void SetRoughness(float roughness) { m_roughness = (roughness < 0.0f) ? 0.0f : (roughness > 1.0f) ? 1.0f : roughness; }

    float GetSpecular() const { return m_specular; }
    void SetSpecular(float specular) { m_specular = (specular < 0.0f) ? 0.0f : (specular > 1.0f) ? 1.0f : specular; }

    float GetEmission() const { return m_emission; }
    void SetEmission(float emission) { m_emission = std::max(0.0f, emission); }

    // Transparency
    float GetOpacity() const { return m_opacity; }
    void SetOpacity(float opacity) { m_opacity = (opacity < 0.0f) ? 0.0f : (opacity > 1.0f) ? 1.0f : opacity; }

    bool IsTransparent() const { return m_opacity < 1.0f; }

    // Textures
    const std::string& GetDiffuseTexture() const { return m_diffuseTexture; }
    void SetDiffuseTexture(const std::string& texture) { m_diffuseTexture = texture; }
    bool LoadDiffuseTexture(const std::string& filePath, class ResourceManager* resourceMgr);

    const std::string& GetNormalTexture() const { return m_normalTexture; }
    void SetNormalTexture(const std::string& texture) { m_normalTexture = texture; }

    const std::string& GetMetallicTexture() const { return m_metallicTexture; }
    void SetMetallicTexture(const std::string& texture) { m_metallicTexture = texture; }

    const std::string& GetRoughnessTexture() const { return m_roughnessTexture; }
    void SetRoughnessTexture(const std::string& texture) { m_roughnessTexture = texture; }

    // Shader properties
    const std::string& GetShaderName() const { return m_shaderName; }
    void SetShaderName(const std::string& shader) { m_shaderName = shader; }

    // Movie-specific properties
    const std::string& GetMaterialTemplate() const { return m_materialTemplate; }
    void SetMaterialTemplate(const std::string& templateName) { m_materialTemplate = templateName; }

    bool IsCinematic() const { return m_isCinematic; }
    void SetCinematic(bool cinematic) { m_isCinematic = cinematic; }

    // Custom properties
    void SetCustomProperty(const std::string& name, float value);
    float GetCustomProperty(const std::string& name) const;
    bool HasCustomProperty(const std::string& name) const;

    // Update and render
    void Update(float deltaTime);
    void Apply(class Renderer* renderer);

    // Material templates
    static std::shared_ptr<Material> CreateMetal(const std::string& name, const DirectX::XMFLOAT3& color);
    static std::shared_ptr<Material> CreatePlastic(const std::string& name, const DirectX::XMFLOAT3& color);
    static std::shared_ptr<Material> CreateGlass(const std::string& name, const DirectX::XMFLOAT3& color);
    static std::shared_ptr<Material> CreateFabric(const std::string& name, const DirectX::XMFLOAT3& color);
    static std::shared_ptr<Material> CreateWood(const std::string& name, const DirectX::XMFLOAT3& color);

private:
    std::string m_name;
    
    // PBR Properties
    DirectX::XMFLOAT3 m_albedo;
    float m_metallic;
    float m_roughness;
    float m_specular;
    float m_emission;
    float m_opacity;

    // Textures
    std::string m_diffuseTexture;
    std::string m_normalTexture;
    std::string m_metallicTexture;
    std::string m_roughnessTexture;

    // Shader
    std::string m_shaderName;

    // Movie-specific
    std::string m_materialTemplate;
    bool m_isCinematic;

    // Custom properties
    std::unordered_map<std::string, float> m_customProperties;
};

/**
 * @brief Material Manager for AI Movie Studio
 * 
 * Manages all materials in the scene and provides:
 * - Material creation and management
 * - Material templates for movie production
 * - Performance optimization
 * - AI-friendly material management
 */
class MaterialManager {
public:
    MaterialManager();
    ~MaterialManager();

    // Material management
    std::shared_ptr<Material> CreateMaterial(const std::string& name);
    void AddMaterial(std::shared_ptr<Material> material);
    void RemoveMaterial(const std::string& name);
    std::shared_ptr<Material> GetMaterial(const std::string& name);

    // Material templates
    std::shared_ptr<Material> CreateDramaticMaterial(const std::string& name);
    std::shared_ptr<Material> CreateRomanticMaterial(const std::string& name);
    std::shared_ptr<Material> CreateActionMaterial(const std::string& name);
    std::shared_ptr<Material> CreateSciFiMaterial(const std::string& name);
    std::shared_ptr<Material> CreateFantasyMaterial(const std::string& name);

    // Update and render
    void Update(float deltaTime);
    void ApplyMaterials(class Renderer* renderer);

    // Getters
    const std::unordered_map<std::string, std::shared_ptr<Material>>& GetAllMaterials() const { return m_materials; }

private:
    std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;

    // Helper methods
    void ApplyMaterialTemplate(std::shared_ptr<Material> material, const std::string& templateName);
};

} // namespace Core
} // namespace Engine
