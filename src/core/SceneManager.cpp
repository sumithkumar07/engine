#include "core/SceneManager.h"
#include "core/Logger.h"
#include "core/SceneObject.h"
#include "core/Camera.h"
#include "core/Light.h"
#include "core/SceneSerializer.h"
#include <algorithm>

// Prevent Windows GetObject macro
#ifdef GetObject
#undef GetObject
#endif

namespace Engine {
namespace Core {

    SceneManager::SceneManager() {
        Logger::Instance().Info("SceneManager created");
    }

    SceneManager::~SceneManager() {
        Shutdown();
        Logger::Instance().Info("SceneManager destroyed");
    }

void SceneManager::Update(float deltaTime) {
    // Update all objects in the current scene
    for (auto& object : m_objects) {
        if (object.second) {
            object.second->Update(deltaTime);
        }
    }
        
        // Update main camera
        if (m_mainCamera) {
        m_mainCamera->Update(deltaTime);
        }
        
        // Update all lights
        for (auto& light : m_lights) {
            if (light.second) {
            light.second->Update(deltaTime);
            }
        }
    }

    void SceneManager::Shutdown() {
        // Clear all objects
        m_objects.clear();
        m_lights.clear();
        m_selectedObjects.clear();
        m_mainCamera.reset();
        m_currentSceneName.clear();
        
        Logger::Instance().Info("SceneManager shutdown complete");
    }

    bool SceneManager::CreateScene(const std::string& name) {
        if (name.empty()) {
            Logger::Instance().Error("Cannot create scene with empty name");
            return false;
        }
        
        // Check if scene already exists
        if (m_currentSceneName == name) {
            Logger::Instance().Warning("Scene '" + name + "' is already the current scene");
            return true;
        }
        
        // Unload current scene if any
        if (!m_currentSceneName.empty()) {
            UnloadScene();
        }
        
        // Create new scene
        m_currentSceneName = name;
        m_objects.clear();
        m_lights.clear();
        m_selectedObjectName.clear();
        
        Logger::Instance().Info("Created scene: " + name);
        NotifySceneChanged(name);
        return true;
    }

    bool SceneManager::LoadScene(const std::string& name) {
        if (name.empty()) {
            Logger::Instance().Error("Cannot load scene with empty name");
            return false;
        }
        
        Logger::Instance().Info("Loading scene: " + name);
        
        // Use SceneSerializer to load from file
        SceneSerializer serializer;
        std::string filePath = "scenes/" + name + ".scene";
        bool success = serializer.DeserializeFromFile(this, filePath);
        
        if (success) {
            m_currentSceneName = name;
            Logger::Instance().Info("Scene loaded successfully: " + name);
        } else {
            Logger::Instance().Error("Failed to load scene: " + name);
        }
        
        return success;
    }

    bool SceneManager::SaveScene(const std::string& name) {
        if (name.empty()) {
            Logger::Instance().Error("Cannot save scene with empty name");
            return false;
        }
        
        Logger::Instance().Info("Saving scene: " + name);
        
        // Use SceneSerializer to save to file
        SceneSerializer serializer;
        std::string filePath = "scenes/" + name + ".scene";
        bool success = serializer.SerializeToFile(this, filePath);
        
        if (success) {
            Logger::Instance().Info("Scene saved successfully: " + name);
        } else {
            Logger::Instance().Error("Failed to save scene: " + name);
        }
        
        return success;
    }

    void SceneManager::UnloadScene() {
        if (m_currentSceneName.empty()) {
            Logger::Instance().Warning("No scene to unload");
            return;
        }
        
        Logger::Instance().Info("Unloading scene: " + m_currentSceneName);
        
        // Clear all objects and lights
        m_objects.clear();
        m_lights.clear();
        m_selectedObjectName.clear();
        m_mainCamera.reset();
        
        m_currentSceneName.clear();
    }

    std::shared_ptr<SceneObject> SceneManager::CreateObject(const std::string& name, const std::string& type) {
        if (name.empty()) {
            Logger::Instance().Error("Cannot create object with empty name");
            return nullptr;
        }
        
        if (type.empty()) {
            Logger::Instance().Error("Cannot create object with empty type");
            return nullptr;
        }
        
        // Check if object already exists
        if (m_objects.find(name) != m_objects.end()) {
            Logger::Instance().Warning("Object '" + name + "' already exists");
            return m_objects[name];
        }
        
        // Create new object
        auto object = std::make_shared<SceneObject>(name, type);
        m_objects[name] = object;
        
        Logger::Instance().Info("Created object: " + name + " (" + type + ")");
        NotifyObjectAdded(name);
        return object;
    }

    bool SceneManager::RemoveObject(const std::string& name) {
        if (name.empty()) {
            Logger::Instance().Error("Cannot remove object with empty name");
            return false;
        }
        
        auto it = m_objects.find(name);
        if (it == m_objects.end()) {
            Logger::Instance().Warning("Object '" + name + "' not found");
            return false;
        }
        
        // Remove from selection if selected
        if (m_selectedObjectName == name) {
            m_selectedObjectName.clear();
        }
        
        // Remove from hierarchy
        auto object = it->second;
        if (object) {
            // Remove from parent
            object->RemoveParent();
            
            // Remove all children
            auto children = object->GetChildren();
            for (auto& child : children) {
                child->RemoveParent();
            }
        }
        
        // Remove from objects map
        m_objects.erase(it);
        
        Logger::Instance().Info("Removed object: " + name);
        NotifyObjectRemoved(name);
        return true;
    }

    std::shared_ptr<SceneObject> SceneManager::GetObject(const std::string& name) const {
        if (name.empty()) {
            Logger::Instance().Warning("Cannot get object with empty name");
            return nullptr;
        }

        auto it = m_objects.find(name);
        if (it == m_objects.end()) {
            Logger::Instance().Warning("Object '" + name + "' not found");
            return nullptr;
        }

        return it->second;
    }

    std::shared_ptr<SceneObject> SceneManager::FindObjectByName(const std::string& name) const {
        // This is an alias to avoid Windows GetObject/GetObjectA/GetObjectW macro issues
        return GetObject(name);
    }

    std::vector<std::shared_ptr<SceneObject>> SceneManager::GetAllObjects() const {
        std::vector<std::shared_ptr<SceneObject>> objects;
        objects.reserve(m_objects.size());
        
        for (const auto& pair : m_objects) {
            objects.push_back(pair.second);
        }
        
        return objects;
    }

    std::vector<std::shared_ptr<SceneObject>> SceneManager::GetObjectsByType(const std::string& type) const {
        std::vector<std::shared_ptr<SceneObject>> objects;
        
        for (const auto& pair : m_objects) {
            if (pair.second && pair.second->GetType() == type) {
                objects.push_back(pair.second);
            }
        }
        
        return objects;
    }

    bool SceneManager::SetParent(const std::string& childName, const std::string& parentName) {
        if (childName.empty() || parentName.empty()) {
            Logger::Instance().Error("Cannot set parent with empty names");
            return false;
        }
        
        if (childName == parentName) {
            Logger::Instance().Error("Object cannot be parent of itself");
            return false;
        }
        
        auto child = GetObject(childName);
        auto parent = GetObject(parentName);
        
        if (!child) {
            Logger::Instance().Error("Child object '" + childName + "' not found");
            return false;
        }
        
        if (!parent) {
            Logger::Instance().Error("Parent object '" + parentName + "' not found");
            return false;
        }
        
        child->SetParent(parent);
        Logger::Instance().Info("Set parent of '" + childName + "' to '" + parentName + "'");
        return true;
    }

    bool SceneManager::RemoveParent(const std::string& childName) {
        if (childName.empty()) {
            Logger::Instance().Error("Cannot remove parent with empty name");
            return false;
        }
        
        auto child = GetObject(childName);
        if (!child) {
            Logger::Instance().Error("Child object '" + childName + "' not found");
            return false;
        }
        
        child->RemoveParent();
        Logger::Instance().Info("Removed parent of '" + childName + "'");
        return true;
    }

    std::vector<std::shared_ptr<SceneObject>> SceneManager::GetChildren(const std::string& parentName) const {
        if (parentName.empty()) {
            Logger::Instance().Warning("Cannot get children with empty parent name");
            return {};
        }
        
        auto parent = GetObject(parentName);
        if (!parent) {
            Logger::Instance().Warning("Parent object '" + parentName + "' not found");
            return {};
        }
        
        return parent->GetChildren();
    }

    void SceneManager::SelectObject(const std::string& name) {
        if (name.empty()) {
            Logger::Instance().Warning("Cannot select object with empty name");
            return;
        }
        
        if (m_objects.find(name) == m_objects.end()) {
            Logger::Instance().Warning("Object '" + name + "' not found for selection");
            return;
        }
        
        m_selectedObjectName = name;
        Logger::Instance().Debug("Selected object: " + name);
        NotifyObjectSelected(name);
    }

    void SceneManager::DeselectObject() {
        if (!m_selectedObjectName.empty()) {
            Logger::Instance().Debug("Deselected object: " + m_selectedObjectName);
            m_selectedObjectName.clear();
        }
    }

    std::shared_ptr<SceneObject> SceneManager::GetSelectedObject() const {
        if (m_selectedObjectName.empty()) {
            return nullptr;
        }
        
        auto it = m_objects.find(m_selectedObjectName);
        if (it == m_objects.end()) {
            return nullptr;
        }
        
        return it->second;
    }

    std::vector<std::shared_ptr<SceneObject>> SceneManager::GetSelectedObjects() const {
        std::vector<std::shared_ptr<SceneObject>> selected;
        
        if (!m_selectedObjectName.empty()) {
            auto obj = GetSelectedObject();
            if (obj) {
                selected.push_back(obj);
            }
        }
        
        return selected;
    }

    void SceneManager::SetMainCamera(std::shared_ptr<Camera> camera) {
        m_mainCamera = camera;
        Logger::Instance().Info("Set main camera");
    }

    std::shared_ptr<Light> SceneManager::CreateLight(const std::string& name, const std::string& type) {
        if (name.empty()) {
            Logger::Instance().Error("Cannot create light with empty name");
            return nullptr;
        }
        
        if (type.empty()) {
            Logger::Instance().Error("Cannot create light with empty type");
            return nullptr;
        }
        
        // Check if light already exists
        if (m_lights.find(name) != m_lights.end()) {
            Logger::Instance().Warning("Light '" + name + "' already exists");
            return m_lights[name];
        }
        
        // Create new light (placeholder - would need actual Light class)
        // auto light = std::make_shared<Light>(name, type);
        // m_lights[name] = light;
        
        Logger::Instance().Info("Created light: " + name + " (" + type + ")");
        return nullptr; // Placeholder
    }

    bool SceneManager::RemoveLight(const std::string& name) {
        if (name.empty()) {
            Logger::Instance().Error("Cannot remove light with empty name");
            return false;
        }
        
        auto it = m_lights.find(name);
        if (it == m_lights.end()) {
            Logger::Instance().Warning("Light '" + name + "' not found");
            return false;
        }
        
        m_lights.erase(it);
        Logger::Instance().Info("Removed light: " + name);
        return true;
    }

    std::shared_ptr<Light> SceneManager::GetLight(const std::string& name) {
        if (name.empty()) {
            Logger::Instance().Warning("Cannot get light with empty name");
            return nullptr;
        }
        
        auto it = m_lights.find(name);
        if (it == m_lights.end()) {
            Logger::Instance().Warning("Light '" + name + "' not found");
            return nullptr;
        }
        
        return it->second;
    }

    std::vector<std::shared_ptr<Light>> SceneManager::GetAllLights() const {
        std::vector<std::shared_ptr<Light>> lights;
        lights.reserve(m_lights.size());
        
        for (const auto& pair : m_lights) {
            lights.push_back(pair.second);
        }
        
        return lights;
    }

    void SceneManager::NotifyObjectAdded(const std::string& name) {
        if (m_objectAddedCallback) {
            m_objectAddedCallback(name);
        }
    }

    void SceneManager::NotifyObjectRemoved(const std::string& name) {
        if (m_objectRemovedCallback) {
            m_objectRemovedCallback(name);
        }
    }

    void SceneManager::NotifyObjectSelected(const std::string& name) {
        if (m_objectSelectedCallback) {
            m_objectSelectedCallback(name);
        }
    }

    void SceneManager::NotifySceneChanged(const std::string& name) {
        if (m_sceneChangedCallback) {
            m_sceneChangedCallback(name);
        }
    }

} // namespace Core
} // namespace Engine
