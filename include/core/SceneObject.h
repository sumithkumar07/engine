#pragma once

#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

namespace Engine {
namespace Core {

    // Forward declarations
    class Mesh;
    class Material;

    /**
     * @brief SceneObject class - Same as before, but with proper component system
     * 
     * Represents a 3D object in the scene.
     * Same design as the previous implementation, but with proper component architecture.
     */
    class SceneObject : public std::enable_shared_from_this<SceneObject> {
    public:
        SceneObject(const std::string& name, const std::string& type);
        ~SceneObject();

        // Basic properties
        const std::string& GetName() const { return m_name; }
        const std::string& GetType() const { return m_type; }
        void SetName(const std::string& name) { m_name = name; }

        // Transform
        const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
        const DirectX::XMFLOAT3& GetRotation() const { return m_rotation; }
        const DirectX::XMFLOAT3& GetScale() const { return m_scale; }
        
        void SetPosition(const DirectX::XMFLOAT3& position);
        void SetRotation(const DirectX::XMFLOAT3& rotation);
        void SetScale(const DirectX::XMFLOAT3& scale);
        
        void SetPosition(float x, float y, float z);
        void SetRotation(float x, float y, float z);
        void SetScale(float x, float y, float z);

        // Update
        void Update(float deltaTime);

        // World transform
        DirectX::XMMATRIX GetWorldMatrix() const;
        void UpdateWorldMatrix();

        // Visibility
        bool IsVisible() const { return m_visible; }
        void SetVisible(bool visible) { m_visible = visible; }

        // Mesh and material
        std::shared_ptr<Mesh> GetMesh() const { return m_mesh; }
        void SetMesh(std::shared_ptr<Mesh> mesh) { m_mesh = mesh; }
        
        std::shared_ptr<Material> GetMaterial() const { return m_material; }
        void SetMaterial(std::shared_ptr<Material> material) { m_material = material; }

        // Hierarchy
        std::shared_ptr<SceneObject> GetParent() const { return m_parent.lock(); }
        void SetParent(std::shared_ptr<SceneObject> parent);
        void RemoveParent();
        
        const std::vector<std::shared_ptr<SceneObject>>& GetChildren() const { return m_children; }
        void AddChild(std::shared_ptr<SceneObject> child);
        void RemoveChild(std::shared_ptr<SceneObject> child);

        // Tags for AI
        void AddTag(const std::string& tag);
        void RemoveTag(const std::string& tag);
        bool HasTag(const std::string& tag) const;
        const std::vector<std::string>& GetTags() const { return m_tags; }

    private:
        // Basic properties
        std::string m_name;
        std::string m_type;
        bool m_visible;

        // Transform
        DirectX::XMFLOAT3 m_position;
        DirectX::XMFLOAT3 m_rotation;
        DirectX::XMFLOAT3 m_scale;
        DirectX::XMMATRIX m_worldMatrix;
        bool m_worldMatrixDirty;

        // Components
        std::shared_ptr<Mesh> m_mesh;
        std::shared_ptr<Material> m_material;

        // Hierarchy
        std::weak_ptr<SceneObject> m_parent;
        std::vector<std::shared_ptr<SceneObject>> m_children;

        // AI tags
        std::vector<std::string> m_tags;
    };

} // namespace Core
} // namespace Engine
