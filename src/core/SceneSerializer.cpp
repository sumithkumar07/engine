#include "core/SceneSerializer.h"
#include "core/SceneManager.h"
#include "core/SceneObject.h"
#include "core/Camera.h"
#include "core/Light.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <DirectXMath.h>

namespace Engine {
namespace Core {

SceneSerializer::SceneSerializer() {
    Logger::Instance().Debug("SceneSerializer created");
}

SceneSerializer::~SceneSerializer() {
    Logger::Instance().Debug("SceneSerializer destroyed");
}

bool SceneSerializer::SerializeToFile(const SceneManager* sceneManager, const std::string& filePath) {
    if (!sceneManager) {
        Logger::Instance().Error("SceneSerializer: Cannot serialize null scene manager");
        return false;
    }

    try {
        std::string jsonData = SerializeToString(sceneManager);
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            Logger::Instance().Error("SceneSerializer: Failed to open file for writing: " + filePath);
            return false;
        }

        file << jsonData;
        file.close();

        Logger::Instance().Info("Scene serialized successfully to: " + filePath);
        return true;
    }
    catch (const std::exception& e) {
        Logger::Instance().Error("SceneSerializer: Exception during serialization: " + std::string(e.what()));
        return false;
    }
}

std::string SceneSerializer::SerializeToString(const SceneManager* sceneManager) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"version\": \"1.0\",\n";
    json << "  \"sceneName\": \"" << EscapeString(sceneManager->GetCurrentSceneName()) << "\",\n";
    
    // Serialize objects
    json << "  \"objects\": [\n";
    auto objects = sceneManager->GetAllObjects();
    for (size_t i = 0; i < objects.size(); ++i) {
        json << SerializeSceneObject(objects[i]);
        if (i < objects.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";

    // Serialize lights
    json << "  \"lights\": [\n";
    auto lights = sceneManager->GetAllLights();
    for (size_t i = 0; i < lights.size(); ++i) {
        json << SerializeLight(lights[i]);
        if (i < lights.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ]\n";

    json << "}\n";
    return json.str();
}

std::string SceneSerializer::SerializeSceneObject(const std::shared_ptr<SceneObject>& object) {
    std::ostringstream json;
    json << "    {\n";
    json << "      \"name\": \"" << EscapeString(object->GetName()) << "\",\n";
    json << "      \"type\": \"" << EscapeString(object->GetType()) << "\",\n";
    json << "      \"visible\": " << (object->IsVisible() ? "true" : "false") << ",\n";
    json << "      \"position\": " << SerializeVector3(object->GetPosition()) << ",\n";
    json << "      \"rotation\": " << SerializeVector3(object->GetRotation()) << ",\n";
    json << "      \"scale\": " << SerializeVector3(object->GetScale()) << ",\n";
    
    // Serialize parent name (if any)
    auto parent = object->GetParent();
    if (parent) {
        json << "      \"parent\": \"" << EscapeString(parent->GetName()) << "\",\n";
    } else {
        json << "      \"parent\": null,\n";
    }

    // Serialize tags
    json << "      \"tags\": [";
    const auto& tags = object->GetTags();
    for (size_t i = 0; i < tags.size(); ++i) {
        json << "\"" << EscapeString(tags[i]) << "\"";
        if (i < tags.size() - 1) json << ", ";
    }
    json << "]\n";
    
    json << "    }";
    return json.str();
}

std::string SceneSerializer::SerializeCamera(const std::shared_ptr<Camera>& camera) {
    std::ostringstream json;
    json << "    {\n";
    json << "      \"name\": \"" << EscapeString(camera->GetName()) << "\",\n";
    json << "      \"position\": " << SerializeVector3(camera->GetPosition()) << ",\n";
    json << "      \"target\": " << SerializeVector3(camera->GetTarget()) << ",\n";
    json << "      \"up\": " << SerializeVector3(camera->GetUp()) << ",\n";
    json << "      \"fov\": " << camera->GetFieldOfView() << ",\n";
    json << "      \"nearPlane\": " << camera->GetNearPlane() << ",\n";
    json << "      \"farPlane\": " << camera->GetFarPlane() << "\n";
    json << "    }";
    return json.str();
}

std::string SceneSerializer::SerializeLight(const std::shared_ptr<Light>& light) {
    std::ostringstream json;
    json << "    {\n";
    json << "      \"name\": \"" << EscapeString(light->GetName()) << "\",\n";
    json << "      \"type\": " << static_cast<int>(light->GetType()) << ",\n";
    json << "      \"enabled\": " << (light->IsEnabled() ? "true" : "false") << ",\n";
    json << "      \"position\": " << SerializeVector3(light->GetPosition()) << ",\n";
    json << "      \"direction\": " << SerializeVector3(light->GetDirection()) << ",\n";
    json << "      \"color\": " << SerializeVector3(light->GetColor()) << ",\n";
    json << "      \"intensity\": " << light->GetIntensity() << ",\n";
    json << "      \"range\": " << light->GetRange() << "\n";
    json << "    }";
    return json.str();
}

std::string SceneSerializer::SerializeVector3(const DirectX::XMFLOAT3& vec) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    json << "[" << vec.x << ", " << vec.y << ", " << vec.z << "]";
    return json.str();
}

bool SceneSerializer::DeserializeFromFile(SceneManager* sceneManager, const std::string& filePath) {
    if (!sceneManager) {
        Logger::Instance().Error("SceneSerializer: Cannot deserialize to null scene manager");
        return false;
    }

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            Logger::Instance().Error("SceneSerializer: Failed to open file for reading: " + filePath);
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        bool success = DeserializeFromString(sceneManager, buffer.str());
        if (success) {
            Logger::Instance().Info("Scene deserialized successfully from: " + filePath);
        }
        return success;
    }
    catch (const std::exception& e) {
        Logger::Instance().Error("SceneSerializer: Exception during deserialization: " + std::string(e.what()));
        return false;
    }
}

bool SceneSerializer::DeserializeFromString(SceneManager* sceneManager, const std::string& jsonString) {
    try {
        // Clear current scene
        auto objects = sceneManager->GetAllObjects();
        for (auto& obj : objects) {
            sceneManager->RemoveObject(obj->GetName());
        }

        // Parse scene name
        std::string sceneName = GetJsonValue(jsonString, "sceneName");
        if (!sceneName.empty()) {
            sceneManager->CreateScene(sceneName);
        }

        // Parse objects array
        auto objectsJson = ParseJsonArray(jsonString, "objects");
        for (const auto& objJson : objectsJson) {
            DeserializeSceneObject(sceneManager, objJson);
        }

        // Parse lights array
        auto lightsJson = ParseJsonArray(jsonString, "lights");
        for (const auto& lightJson : lightsJson) {
            DeserializeLight(sceneManager, lightJson);
        }

        // Reconstruct hierarchy (second pass after all objects exist)
        for (const auto& objJson : objectsJson) {
            std::string objName = GetJsonValue(objJson, "name");
            std::string parentName = GetJsonValue(objJson, "parent");
            if (!parentName.empty() && parentName != "null") {
                sceneManager->SetParent(objName, parentName);
            }
        }

        Logger::Instance().Info("Scene deserialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        Logger::Instance().Error("SceneSerializer: Exception parsing JSON: " + std::string(e.what()));
        return false;
    }
}

bool SceneSerializer::DeserializeSceneObject(SceneManager* sceneManager, const std::string& json) {
    std::string name = GetJsonValue(json, "name");
    std::string type = GetJsonValue(json, "type");

    auto obj = sceneManager->CreateObject(name, type);
    if (!obj) {
        Logger::Instance().Warning("Failed to create object: " + name);
        return false;
    }

    // Set transform
    obj->SetPosition(DeserializeVector3(GetJsonValue(json, "position")));
    obj->SetRotation(DeserializeVector3(GetJsonValue(json, "rotation")));
    obj->SetScale(DeserializeVector3(GetJsonValue(json, "scale")));

    // Set visibility
    std::string visibleStr = GetJsonValue(json, "visible");
    obj->SetVisible(visibleStr == "true");

    // Tags are set in a separate pass if needed
    return true;
}

bool SceneSerializer::DeserializeLight(SceneManager* sceneManager, const std::string& json) {
    std::string name = GetJsonValue(json, "name");
    std::string typeStr = GetJsonValue(json, "type");
    
    auto light = sceneManager->CreateLight(name, "Point"); // Default type
    if (!light) {
        Logger::Instance().Warning("Failed to create light: " + name);
        return false;
    }

    light->SetPosition(DeserializeVector3(GetJsonValue(json, "position")));
    light->SetDirection(DeserializeVector3(GetJsonValue(json, "direction")));
    light->SetColor(DeserializeVector3(GetJsonValue(json, "color")));
    
    std::string intensityStr = GetJsonValue(json, "intensity");
    if (!intensityStr.empty()) {
        light->SetIntensity(std::stof(intensityStr));
    }

    std::string rangeStr = GetJsonValue(json, "range");
    if (!rangeStr.empty()) {
        light->SetRange(std::stof(rangeStr));
    }

    return true;
}

DirectX::XMFLOAT3 SceneSerializer::DeserializeVector3(const std::string& json) {
    DirectX::XMFLOAT3 result(0, 0, 0);
    if (json.empty() || json == "null") return result;

    // Parse [x, y, z] format
    size_t start = json.find('[');
    size_t end = json.find(']');
    if (start == std::string::npos || end == std::string::npos) return result;

    std::string values = json.substr(start + 1, end - start - 1);
    std::istringstream ss(values);
    std::string value;
    
    if (std::getline(ss, value, ',')) result.x = std::stof(value);
    if (std::getline(ss, value, ',')) result.y = std::stof(value);
    if (std::getline(ss, value, ',')) result.z = std::stof(value);

    return result;
}

std::string SceneSerializer::EscapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\t') result += "\\t";
        else result += c;
    }
    return result;
}

std::string SceneSerializer::UnescapeString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            char next = str[i + 1];
            if (next == '"') result += '"';
            else if (next == '\\') result += '\\';
            else if (next == 'n') result += '\n';
            else if (next == 'r') result += '\r';
            else if (next == 't') result += '\t';
            else result += next;
            ++i;
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string SceneSerializer::GetJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;

    if (pos >= json.length()) return "";

    if (json[pos] == '"') {
        // String value
        size_t start = pos + 1;
        size_t end = start;
        while (end < json.length() && json[end] != '"') {
            if (json[end] == '\\') ++end; // Skip escaped characters
            ++end;
        }
        return UnescapeString(json.substr(start, end - start));
    } else if (json[pos] == '[') {
        // Array value
        int depth = 0;
        size_t start = pos;
        size_t end = pos;
        while (end < json.length()) {
            if (json[end] == '[') ++depth;
            else if (json[end] == ']') {
                --depth;
                if (depth == 0) break;
            }
            ++end;
        }
        return json.substr(start, end - start + 1);
    } else {
        // Number, boolean, or null
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != '\n') ++end;
        std::string value = json.substr(pos, end - pos);
        // Trim whitespace
        size_t trimEnd = value.find_last_not_of(" \t\r\n");
        return (trimEnd != std::string::npos) ? value.substr(0, trimEnd + 1) : value;
    }
}

std::vector<std::string> SceneSerializer::ParseJsonArray(const std::string& json, const std::string& arrayKey) {
    std::vector<std::string> result;
    std::string arrayJson = GetJsonValue(json, arrayKey);
    if (arrayJson.empty() || arrayJson[0] != '[') return result;

    int depth = 0;
    size_t start = 1; // Skip opening [
    for (size_t i = 1; i < arrayJson.length(); ++i) {
        char c = arrayJson[i];
        if (c == '{') {
            if (depth == 0) start = i;
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                result.push_back(arrayJson.substr(start, i - start + 1));
            }
        }
    }

    return result;
}

} // namespace Core
} // namespace Engine

