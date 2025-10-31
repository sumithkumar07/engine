#pragma once

#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

namespace Engine {
namespace Core {

class SceneManager;
class SceneObject;
class Camera;
class Light;

/**
 * @brief Scene serialization/deserialization for save/load functionality
 * 
 * Handles converting scene data to/from JSON format for persistent storage.
 * Supports all scene elements: objects, transforms, hierarchy, materials, cameras, lights.
 */
class SceneSerializer {
public:
    SceneSerializer();
    ~SceneSerializer();

    /**
     * @brief Serialize scene to JSON file
     * @param sceneManager Scene manager containing scene data
     * @param filePath Path to save file
     * @return true if successful
     */
    bool SerializeToFile(const SceneManager* sceneManager, const std::string& filePath);

    /**
     * @brief Deserialize scene from JSON file
     * @param sceneManager Scene manager to populate
     * @param filePath Path to load file
     * @return true if successful
     */
    bool DeserializeFromFile(SceneManager* sceneManager, const std::string& filePath);

    /**
     * @brief Serialize scene to JSON string
     * @param sceneManager Scene manager containing scene data
     * @return JSON string
     */
    std::string SerializeToString(const SceneManager* sceneManager);

    /**
     * @brief Deserialize scene from JSON string
     * @param sceneManager Scene manager to populate
     * @param jsonString JSON string to parse
     * @return true if successful
     */
    bool DeserializeFromString(SceneManager* sceneManager, const std::string& jsonString);

private:
    // Helper methods for serialization
    std::string SerializeSceneObject(const std::shared_ptr<SceneObject>& object);
    std::string SerializeCamera(const std::shared_ptr<Camera>& camera);
    std::string SerializeLight(const std::shared_ptr<Light>& light);
    std::string SerializeVector3(const DirectX::XMFLOAT3& vec);

    // Helper methods for deserialization
    bool DeserializeSceneObject(SceneManager* sceneManager, const std::string& json);
    bool DeserializeCamera(SceneManager* sceneManager, const std::string& json);
    bool DeserializeLight(SceneManager* sceneManager, const std::string& json);
    DirectX::XMFLOAT3 DeserializeVector3(const std::string& json);

    // JSON helper utilities
    std::string EscapeString(const std::string& str);
    std::string UnescapeString(const std::string& str);
    std::string GetJsonValue(const std::string& json, const std::string& key);
    std::vector<std::string> ParseJsonArray(const std::string& json, const std::string& arrayKey);
};

} // namespace Core
} // namespace Engine

