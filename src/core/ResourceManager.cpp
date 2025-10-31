#include "core/ResourceManager.h"
#include "core/Logger.h"
#include "loaders/TextureLoader.h"
#include <fstream>
#include <algorithm>

namespace Engine {
namespace Core {

    ResourceManager::ResourceManager()
        : m_maxCacheSize(1024 * 1024 * 100) // 100MB default cache
        , m_currentMemoryUsage(0) {
        Logger::Instance().Info("ResourceManager created");
    }

    ResourceManager::~ResourceManager() {
        UnloadAllResources();
        Logger::Instance().Info("ResourceManager destroyed");
    }

    bool ResourceManager::LoadTexture(const std::string& path, const std::string& name) {
        // Check if resource already exists
        if (IsResourceLoaded(name)) {
            Logger::Instance().Warning("Texture already loaded: " + name);
            return true;
        }

        // Load texture using TextureLoader
        Loaders::TextureLoader loader;
        Loaders::TextureData textureData;
        
        if (!loader.LoadFromFile(path, textureData)) {
            Logger::Instance().Error("Failed to load texture: " + path);
            return false;
        }

        // Create resource info
        ResourceInfo info;
        info.name = name;
        info.path = path;
        info.type = "texture";
        info.size = textureData.dataSize;
        info.data = textureData.pixels; // Transfer ownership
        textureData.pixels = nullptr;   // Prevent double delete

        // Store in cache
        m_resources[name] = info;
        m_currentMemoryUsage += info.size;

        Logger::Instance().Info("Loaded texture: " + name + " (" + std::to_string(info.size) + " bytes)");
        return true;
    }

    bool ResourceManager::LoadModel(const std::string& path, const std::string& name) {
        return LoadResourceFromFile(path, name, "model");
    }

    bool ResourceManager::LoadShader(const std::string& path, const std::string& name) {
        return LoadResourceFromFile(path, name, "shader");
    }

    bool ResourceManager::LoadAudio(const std::string& path, const std::string& name) {
        return LoadResourceFromFile(path, name, "audio");
    }

    void* ResourceManager::GetTexture(const std::string& name) const {
        auto it = m_resources.find(name);
        if (it != m_resources.end() && it->second.type == "texture") {
            return it->second.data;
        }
        return nullptr;
    }

    void* ResourceManager::GetModel(const std::string& name) const {
        auto it = m_resources.find(name);
        if (it != m_resources.end() && it->second.type == "model") {
            return it->second.data;
        }
        return nullptr;
    }

    void* ResourceManager::GetShader(const std::string& name) const {
        auto it = m_resources.find(name);
        if (it != m_resources.end() && it->second.type == "shader") {
            return it->second.data;
        }
        return nullptr;
    }

    void* ResourceManager::GetAudio(const std::string& name) const {
        auto it = m_resources.find(name);
        if (it != m_resources.end() && it->second.type == "audio") {
            return it->second.data;
        }
        return nullptr;
    }

    bool ResourceManager::UnloadResource(const std::string& name) {
        auto it = m_resources.find(name);
        if (it != m_resources.end()) {
            if (it->second.data) {
                delete[] static_cast<char*>(it->second.data);
            }
            m_currentMemoryUsage -= it->second.size;
            m_resources.erase(it);
            Logger::Instance().Info("Unloaded resource: " + name);
            return true;
        }
        return false;
    }

    void ResourceManager::UnloadAllResources() {
        for (auto& resource : m_resources) {
            if (resource.second.data) {
                delete[] static_cast<char*>(resource.second.data);
            }
        }
        m_resources.clear();
        m_currentMemoryUsage = 0;
        Logger::Instance().Info("Unloaded all resources");
    }

    bool ResourceManager::IsResourceLoaded(const std::string& name) const {
        return m_resources.find(name) != m_resources.end();
    }

    std::vector<std::string> ResourceManager::GetLoadedResourceNames() const {
        std::vector<std::string> names;
        for (const auto& resource : m_resources) {
            names.push_back(resource.first);
        }
        return names;
    }

    void ResourceManager::SetCacheSize(size_t maxSize) {
        m_maxCacheSize = maxSize;
        while (m_currentMemoryUsage > m_maxCacheSize && !m_resources.empty()) {
            EvictOldestResource();
        }
    }

    size_t ResourceManager::GetCacheSize() const {
        return m_maxCacheSize;
    }

    void ResourceManager::ClearCache() {
        UnloadAllResources();
    }

    size_t ResourceManager::GetResourceCount() const {
        return m_resources.size();
    }

    size_t ResourceManager::GetMemoryUsage() const {
        return m_currentMemoryUsage;
    }

    bool ResourceManager::LoadResourceFromFile(const std::string& path, const std::string& name, const std::string& type) {
        // Check if resource already exists
        if (IsResourceLoaded(name)) {
            Logger::Instance().Warning("Resource already loaded: " + name);
            return true;
        }

        // Open file
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            Logger::Instance().Error("Failed to open resource file: " + path);
            return false;
        }

        // Get file size
        file.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        // Check cache size
        while (m_currentMemoryUsage + fileSize > m_maxCacheSize && !m_resources.empty()) {
            EvictOldestResource();
        }

        // Allocate memory
        char* data = new char[fileSize];
        if (!data) {
            Logger::Instance().Error("Failed to allocate memory for resource: " + name);
            return false;
        }

        // Read file
        file.read(data, fileSize);
        if (!file) {
            Logger::Instance().Error("Failed to read resource file: " + path);
            delete[] data;
            return false;
        }

        // Store resource
        ResourceInfo info;
        info.name = name;
        info.path = path;
        info.data = data;
        info.size = fileSize;
        info.type = type;

        m_resources[name] = info;
        m_currentMemoryUsage += fileSize;

        Logger::Instance().Info("Loaded resource: " + name + " (" + std::to_string(fileSize) + " bytes)");
        return true;
    }

    void ResourceManager::UpdateMemoryUsage() {
        m_currentMemoryUsage = 0;
        for (const auto& resource : m_resources) {
            m_currentMemoryUsage += resource.second.size;
        }
    }

    void ResourceManager::EvictOldestResource() {
        if (m_resources.empty()) {
            return;
        }

        // Simple eviction: remove first resource (FIFO)
        auto it = m_resources.begin();
        if (it->second.data) {
            delete[] static_cast<char*>(it->second.data);
        }
        m_currentMemoryUsage -= it->second.size;
        Logger::Instance().Info("Evicted resource: " + it->first);
        m_resources.erase(it);
    }

} // namespace Core
} // namespace Engine
