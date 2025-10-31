#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <DirectXMath.h>

namespace Engine {
namespace Core {

    // Forward declarations
    class SceneObject;
    class Camera;
    class Light;

    /**
     * @brief SceneManager class - Same as before, but with proper memory management
     * 
     * Manages all scenes, objects, and their relationships.
     * Same design as the previous implementation, but with proper smart pointer usage.
     */
    class SceneManager {
    public:
        SceneManager();
        ~SceneManager();
        void Shutdown();

        // Update
        void Update(float deltaTime);

        // Scene management
        bool CreateScene(const std::string& name);
        bool LoadScene(const std::string& name);
        bool SaveScene(const std::string& name);
        void UnloadScene();
        const std::string& GetCurrentSceneName() const { return m_currentSceneName; }

        // Object management
        std::shared_ptr<SceneObject> CreateObject(const std::string& name, const std::string& type);
        bool RemoveObject(const std::string& name);
        std::shared_ptr<SceneObject> GetObject(const std::string& name) const;
        std::shared_ptr<SceneObject> FindObjectByName(const std::string& name) const; // Alias to avoid Windows macro
        std::vector<std::shared_ptr<SceneObject>> GetAllObjects() const;
        std::vector<std::shared_ptr<SceneObject>> GetObjectsByType(const std::string& type) const;

        // Hierarchy management
        bool SetParent(const std::string& childName, const std::string& parentName);
        bool RemoveParent(const std::string& childName);
        std::vector<std::shared_ptr<SceneObject>> GetChildren(const std::string& parentName) const;

        // Selection management
        void SelectObject(const std::string& name);
        void DeselectObject();
        std::shared_ptr<SceneObject> GetSelectedObject() const;
        std::vector<std::shared_ptr<SceneObject>> GetSelectedObjects() const;

        // Camera management
        Camera* GetMainCamera() const { return m_mainCamera.get(); }
        void SetMainCamera(std::shared_ptr<Camera> camera);

        // Light management
        std::shared_ptr<Light> CreateLight(const std::string& name, const std::string& type);
        bool RemoveLight(const std::string& name);
        std::shared_ptr<Light> GetLight(const std::string& name);
        std::vector<std::shared_ptr<Light>> GetAllLights() const;

        // Event callbacks
        using ObjectAddedCallback = std::function<void(const std::string&)>;
        using ObjectRemovedCallback = std::function<void(const std::string&)>;
        using ObjectSelectedCallback = std::function<void(const std::string&)>;
        using SceneChangedCallback = std::function<void(const std::string&)>;

        void SetObjectAddedCallback(ObjectAddedCallback callback) { m_objectAddedCallback = callback; }
        void SetObjectRemovedCallback(ObjectRemovedCallback callback) { m_objectRemovedCallback = callback; }
        void SetObjectSelectedCallback(ObjectSelectedCallback callback) { m_objectSelectedCallback = callback; }
        void SetSceneChangedCallback(SceneChangedCallback callback) { m_sceneChangedCallback = callback; }

    private:
        // Scene data
        std::string m_currentSceneName;
        std::unordered_map<std::string, std::shared_ptr<SceneObject>> m_objects;
        std::unordered_map<std::string, std::shared_ptr<Light>> m_lights;
        std::shared_ptr<Camera> m_mainCamera;
        std::string m_selectedObjectName;
        std::vector<std::shared_ptr<SceneObject>> m_selectedObjects;

        // Event callbacks
        ObjectAddedCallback m_objectAddedCallback;
        ObjectRemovedCallback m_objectRemovedCallback;
        ObjectSelectedCallback m_objectSelectedCallback;
        SceneChangedCallback m_sceneChangedCallback;

        // Private methods
        void NotifyObjectAdded(const std::string& name);
        void NotifyObjectRemoved(const std::string& name);
        void NotifyObjectSelected(const std::string& name);
        void NotifySceneChanged(const std::string& name);
    };

} // namespace Core
} // namespace Engine
