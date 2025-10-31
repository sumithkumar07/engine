#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Engine {
namespace Core {

    class ResourceManager {
    public:
        ResourceManager();
        ~ResourceManager();

        // Resource loading
        bool LoadTexture(const std::string& path, const std::string& name);
        bool LoadModel(const std::string& path, const std::string& name);
        bool LoadShader(const std::string& path, const std::string& name);
        bool LoadAudio(const std::string& path, const std::string& name);

        // Resource retrieval
        void* GetTexture(const std::string& name) const;
        void* GetModel(const std::string& name) const;
        void* GetShader(const std::string& name) const;
        void* GetAudio(const std::string& name) const;

        // Resource management
        bool UnloadResource(const std::string& name);
        void UnloadAllResources();
        bool IsResourceLoaded(const std::string& name) const;
        std::vector<std::string> GetLoadedResourceNames() const;

        // Resource caching
        void SetCacheSize(size_t maxSize);
        size_t GetCacheSize() const;
        void ClearCache();

        // Resource monitoring
        size_t GetResourceCount() const;
        size_t GetMemoryUsage() const;

    private:
        struct ResourceInfo {
            std::string name;
            std::string path;
            void* data;
            size_t size;
            std::string type;
        };

        std::unordered_map<std::string, ResourceInfo> m_resources;
        size_t m_maxCacheSize;
        size_t m_currentMemoryUsage;

        bool LoadResourceFromFile(const std::string& path, const std::string& name, const std::string& type);
        void UpdateMemoryUsage();
        void EvictOldestResource();
    };

} // namespace Core
} // namespace Engine
