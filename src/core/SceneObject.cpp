#include "core/SceneObject.h"
#include "core/Mesh.h"
#include "core/Logger.h"
#include <algorithm>

namespace Engine {
namespace Core {

    SceneObject::SceneObject(const std::string& name, const std::string& type)
        : m_name(name), m_type(type), m_visible(true),
          m_position(0.0f, 0.0f, 0.0f),
          m_rotation(0.0f, 0.0f, 0.0f),
          m_scale(1.0f, 1.0f, 1.0f),
          m_worldMatrixDirty(true) {
        
        // Initialize world matrix to identity
        m_worldMatrix = DirectX::XMMatrixIdentity();
        
        Logger::Instance().Debug("SceneObject created: " + name + " (" + type + ")");
    }

    SceneObject::~SceneObject() {
        Logger::Instance().Debug("SceneObject destroyed: " + m_name);
    }

    void SceneObject::SetPosition(const DirectX::XMFLOAT3& position) {
        m_position = position;
        m_worldMatrixDirty = true;
        Logger::Instance().Debug("Set position for " + m_name + ": (" + 
                 std::to_string(position.x) + ", " + 
                 std::to_string(position.y) + ", " + 
                 std::to_string(position.z) + ")");
    }

    void SceneObject::SetRotation(const DirectX::XMFLOAT3& rotation) {
        m_rotation = rotation;
        m_worldMatrixDirty = true;
        Logger::Instance().Debug("Set rotation for " + m_name + ": (" + 
                 std::to_string(rotation.x) + ", " + 
                 std::to_string(rotation.y) + ", " + 
                 std::to_string(rotation.z) + ")");
    }

    void SceneObject::SetScale(const DirectX::XMFLOAT3& scale) {
        m_scale = scale;
        m_worldMatrixDirty = true;
        Logger::Instance().Debug("Set scale for " + m_name + ": (" + 
                 std::to_string(scale.x) + ", " + 
                 std::to_string(scale.y) + ", " + 
                 std::to_string(scale.z) + ")");
    }

    void SceneObject::SetPosition(float x, float y, float z) {
        SetPosition(DirectX::XMFLOAT3(x, y, z));
    }

    void SceneObject::SetRotation(float x, float y, float z) {
        SetRotation(DirectX::XMFLOAT3(x, y, z));
    }

    void SceneObject::SetScale(float x, float y, float z) {
        SetScale(DirectX::XMFLOAT3(x, y, z));
    }

    void SceneObject::Update(float deltaTime) {
        // Update world matrix if needed
        if (m_worldMatrixDirty) {
            UpdateWorldMatrix();
        }
        
        // Update children
        for (auto& child : m_children) {
            if (child) {
                child->Update(deltaTime);
            }
        }
    }

    DirectX::XMMATRIX SceneObject::GetWorldMatrix() const {
        if (m_worldMatrixDirty) {
            const_cast<SceneObject*>(this)->UpdateWorldMatrix();
        }
        return m_worldMatrix;
    }

    void SceneObject::UpdateWorldMatrix() {
        // Create transformation matrices
        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
        
        // Combine transformations: Scale * Rotation * Translation
        m_worldMatrix = scale * rotation * translation;
        
        // Apply parent transform if exists
        auto parent = m_parent.lock();
        if (parent) {
            m_worldMatrix = DirectX::XMMatrixMultiply(m_worldMatrix, parent->GetWorldMatrix());
        }
        
        m_worldMatrixDirty = false;
    }

    void SceneObject::SetParent(std::shared_ptr<SceneObject> parent) {
        // Remove from old parent's children
        auto oldParent = m_parent.lock();
        if (oldParent) {
            oldParent->RemoveChild(shared_from_this());
        }
        
        // Set new parent
        m_parent = parent;
        m_worldMatrixDirty = true;
        
        // Add to new parent's children
        if (parent) {
            parent->AddChild(shared_from_this());
            Logger::Instance().Debug("Set parent for " + m_name + " to " + parent->GetName());
        } else {
            Logger::Instance().Debug("Removed parent for " + m_name);
        }
    }

    void SceneObject::RemoveParent() {
        SetParent(nullptr);
    }

    void SceneObject::AddChild(std::shared_ptr<SceneObject> child) {
        if (!child) {
            Logger::Instance().Warning("Attempted to add null child to " + m_name);
            return;
        }
        
        // Check if already a child
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end()) {
            Logger::Instance().Warning("Child " + child->GetName() + " already exists in " + m_name);
            return;
        }
        
        m_children.push_back(child);
        child->m_worldMatrixDirty = true; // Mark child for world matrix update
        Logger::Instance().Debug("Added child " + child->GetName() + " to " + m_name);
    }

    void SceneObject::RemoveChild(std::shared_ptr<SceneObject> child) {
        if (!child) {
            Logger::Instance().Warning("Attempted to remove null child from " + m_name);
            return;
        }
        
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end()) {
            m_children.erase(it);
            child->m_worldMatrixDirty = true; // Mark child for world matrix update
            Logger::Instance().Debug("Removed child " + child->GetName() + " from " + m_name);
        } else {
            Logger::Instance().Warning("Child " + child->GetName() + " not found in " + m_name);
        }
    }

    void SceneObject::AddTag(const std::string& tag) {
        // Check if tag already exists
        auto it = std::find(m_tags.begin(), m_tags.end(), tag);
        if (it == m_tags.end()) {
            m_tags.push_back(tag);
            Logger::Instance().Debug("Added tag '" + tag + "' to " + m_name);
        } else {
            Logger::Instance().Warning("Tag '" + tag + "' already exists on " + m_name);
        }
    }

    void SceneObject::RemoveTag(const std::string& tag) {
        auto it = std::find(m_tags.begin(), m_tags.end(), tag);
        if (it != m_tags.end()) {
            m_tags.erase(it);
            Logger::Instance().Debug("Removed tag '" + tag + "' from " + m_name);
        } else {
            Logger::Instance().Warning("Tag '" + tag + "' not found on " + m_name);
        }
    }

    bool SceneObject::HasTag(const std::string& tag) const {
        return std::find(m_tags.begin(), m_tags.end(), tag) != m_tags.end();
    }

} // namespace Core
} // namespace Engine
