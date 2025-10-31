#pragma once

#include <memory>
#include <string>
#include <vector>
#include "core/Mesh.h"

namespace Engine {
namespace Loaders {

/**
 * @brief Mesh loader for various 3D file formats
 * 
 * Supports:
 * - OBJ (Wavefront)
 * - FBX (Autodesk Filmbox)
 * - GLTF/GLB (Khronos GL Transmission Format)
 * - COLLADA (DAE)
 * - And more via Assimp
 */
class MeshLoader {
public:
    MeshLoader();
    ~MeshLoader();

    /**
     * @brief Load mesh from file (auto-detects format)
     * @param filePath Path to mesh file
     * @param mesh Target mesh to populate
     * @return true if successful
     */
    bool LoadFromFile(const std::string& filePath, Core::Mesh* mesh);

    /**
     * @brief Load OBJ file (manual parser, no dependencies)
     * @param filePath Path to .obj file
     * @param mesh Target mesh to populate
     * @return true if successful
     */
    bool LoadOBJ(const std::string& filePath, Core::Mesh* mesh);

    /**
     * @brief Load using Assimp (supports FBX, GLTF, COLLADA, etc.)
     * @param filePath Path to mesh file
     * @param mesh Target mesh to populate
     * @return true if successful
     */
    bool LoadWithAssimp(const std::string& filePath, Core::Mesh* mesh);

    /**
     * @brief Get supported file extensions
     * @return Vector of supported extensions (e.g., ".obj", ".fbx", ".gltf")
     */
    static std::vector<std::string> GetSupportedExtensions();

    /**
     * @brief Check if file format is supported
     * @param filePath File path to check
     * @return true if supported
     */
    static bool IsFormatSupported(const std::string& filePath);

private:
    // Helper methods for OBJ parsing
    void ParseOBJLine(const std::string& line, 
                      std::vector<DirectX::XMFLOAT3>& positions,
                      std::vector<DirectX::XMFLOAT3>& normals,
                      std::vector<DirectX::XMFLOAT2>& texCoords,
                      std::vector<Core::Vertex>& vertices,
                      std::vector<uint32_t>& indices);

    void ParseOBJVertex(const std::string& vertexStr,
                        const std::vector<DirectX::XMFLOAT3>& positions,
                        const std::vector<DirectX::XMFLOAT3>& normals,
                        const std::vector<DirectX::XMFLOAT2>& texCoords,
                        std::vector<Core::Vertex>& vertices,
                        std::vector<uint32_t>& indices,
                        std::unordered_map<std::string, uint32_t>& vertexCache);

    std::string GetFileExtension(const std::string& filePath);
    std::string ToLower(const std::string& str);
};

} // namespace Loaders
} // namespace Engine

