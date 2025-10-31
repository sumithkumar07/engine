#include "loaders/MeshLoader.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

// NOTE: Assimp integration is optional for now
// If Assimp is not available, we'll use OBJ-only fallback
#ifdef USE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

namespace Engine {
namespace Loaders {

MeshLoader::MeshLoader() {
    Core::Logger::Instance().Debug("MeshLoader created");
}

MeshLoader::~MeshLoader() {
    Core::Logger::Instance().Debug("MeshLoader destroyed");
}

bool MeshLoader::LoadFromFile(const std::string& filePath, Core::Mesh* mesh) {
    if (!mesh) {
        Core::Logger::Instance().Error("MeshLoader: Cannot load into null mesh");
        return false;
    }

    if (!IsFormatSupported(filePath)) {
        Core::Logger::Instance().Error("MeshLoader: Unsupported file format: " + filePath);
        return false;
    }

    std::string ext = GetFileExtension(filePath);
    Core::Logger::Instance().Info("MeshLoader: Loading mesh from " + filePath + " (format: " + ext + ")");

    // For OBJ files, use our custom parser (no dependencies)
    if (ext == ".obj") {
        return LoadOBJ(filePath, mesh);
    }

#ifdef USE_ASSIMP
    // For other formats (FBX, GLTF, etc.), use Assimp
    return LoadWithAssimp(filePath, mesh);
#else
    // If Assimp not available, only OBJ is supported
    Core::Logger::Instance().Warning("MeshLoader: Assimp not available. Only OBJ format is currently supported.");
    Core::Logger::Instance().Warning("MeshLoader: To enable FBX/GLTF/etc., build with USE_ASSIMP flag and link Assimp library.");
    return false;
#endif
}

bool MeshLoader::LoadOBJ(const std::string& filePath, Core::Mesh* mesh) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Core::Logger::Instance().Error("MeshLoader: Failed to open OBJ file: " + filePath);
        return false;
    }

    // Temporary storage for OBJ data
    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> texCoords;
    std::vector<Core::Vertex> vertices;
    std::vector<uint32_t> indices;

    std::string line;
    while (std::getline(file, line)) {
        ParseOBJLine(line, positions, normals, texCoords, vertices, indices);
    }

    file.close();

    if (vertices.empty()) {
        Core::Logger::Instance().Warning("MeshLoader: No vertices found in OBJ file: " + filePath);
        return false;
    }

    // Set mesh data
    mesh->SetVertices(vertices);
    mesh->SetIndices(indices);
    mesh->GenerateNormals(); // Generate normals if missing
    mesh->GenerateTangents();

    Core::Logger::Instance().Info("MeshLoader: Loaded OBJ with " + std::to_string(vertices.size()) + " vertices, " + 
                                  std::to_string(indices.size()) + " indices");
    return true;
}

void MeshLoader::ParseOBJLine(const std::string& line,
                               std::vector<DirectX::XMFLOAT3>& positions,
                               std::vector<DirectX::XMFLOAT3>& normals,
                               std::vector<DirectX::XMFLOAT2>& texCoords,
                               std::vector<Core::Vertex>& vertices,
                               std::vector<uint32_t>& indices) {
    if (line.empty() || line[0] == '#') return;

    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if (prefix == "v") {
        // Vertex position
        DirectX::XMFLOAT3 pos;
        iss >> pos.x >> pos.y >> pos.z;
        positions.push_back(pos);
    }
    else if (prefix == "vt") {
        // Texture coordinate
        DirectX::XMFLOAT2 tc;
        iss >> tc.x >> tc.y;
        texCoords.push_back(tc);
    }
    else if (prefix == "vn") {
        // Vertex normal
        DirectX::XMFLOAT3 n;
        iss >> n.x >> n.y >> n.z;
        normals.push_back(n);
    }
    else if (prefix == "f") {
        // Face (triangle or quad)
        std::vector<std::string> faceVertices;
        std::string vertexStr;
        while (iss >> vertexStr) {
            faceVertices.push_back(vertexStr);
        }

        // Triangulate if quad
        if (faceVertices.size() >= 3) {
            static std::unordered_map<std::string, uint32_t> vertexCache;
            
            // First triangle
            for (int i = 0; i < 3; ++i) {
                ParseOBJVertex(faceVertices[i], positions, normals, texCoords, vertices, indices, vertexCache);
            }

            // If quad, create second triangle
            if (faceVertices.size() == 4) {
                ParseOBJVertex(faceVertices[0], positions, normals, texCoords, vertices, indices, vertexCache);
                ParseOBJVertex(faceVertices[2], positions, normals, texCoords, vertices, indices, vertexCache);
                ParseOBJVertex(faceVertices[3], positions, normals, texCoords, vertices, indices, vertexCache);
            }
        }
    }
}

void MeshLoader::ParseOBJVertex(const std::string& vertexStr,
                                 const std::vector<DirectX::XMFLOAT3>& positions,
                                 const std::vector<DirectX::XMFLOAT3>& normals,
                                 const std::vector<DirectX::XMFLOAT2>& texCoords,
                                 std::vector<Core::Vertex>& vertices,
                                 std::vector<uint32_t>& indices,
                                 std::unordered_map<std::string, uint32_t>& vertexCache) {
    // Check cache first
    auto it = vertexCache.find(vertexStr);
    if (it != vertexCache.end()) {
        indices.push_back(it->second);
        return;
    }

    // Parse vertex indices (format: v/vt/vn or v//vn or v/vt or v)
    Core::Vertex vertex;
    int posIdx = -1, tcIdx = -1, normIdx = -1;

    std::istringstream iss(vertexStr);
    std::string indexStr;
    int component = 0;

    while (std::getline(iss, indexStr, '/')) {
        if (!indexStr.empty()) {
            int idx = std::stoi(indexStr) - 1; // OBJ indices are 1-based
            if (component == 0) posIdx = idx;
            else if (component == 1) tcIdx = idx;
            else if (component == 2) normIdx = idx;
        }
        ++component;
    }

    // Set vertex data
    if (posIdx >= 0 && posIdx < static_cast<int>(positions.size())) {
        vertex.position = positions[posIdx];
    }
    if (tcIdx >= 0 && tcIdx < static_cast<int>(texCoords.size())) {
        vertex.texCoord = texCoords[tcIdx];
    }
    if (normIdx >= 0 && normIdx < static_cast<int>(normals.size())) {
        vertex.normal = normals[normIdx];
    }

    // Add to vertices and cache
    uint32_t newIndex = static_cast<uint32_t>(vertices.size());
    vertices.push_back(vertex);
    indices.push_back(newIndex);
    vertexCache[vertexStr] = newIndex;
}

#ifdef USE_ASSIMP
bool MeshLoader::LoadWithAssimp(const std::string& filePath, Core::Mesh* mesh) {
    Assimp::Importer importer;
    
    // Import with post-processing
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Core::Logger::Instance().Error("MeshLoader: Assimp error: " + std::string(importer.GetErrorString()));
        return false;
    }

    // Process first mesh (for simplicity; can be extended to load all meshes)
    if (scene->mNumMeshes == 0) {
        Core::Logger::Instance().Warning("MeshLoader: No meshes found in file: " + filePath);
        return false;
    }

    aiMesh* aiMesh = scene->mMeshes[0];
    std::vector<Core::Vertex> vertices;
    std::vector<uint32_t> indices;

    // Extract vertices
    for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i) {
        Core::Vertex vertex;

        // Position
        vertex.position.x = aiMesh->mVertices[i].x;
        vertex.position.y = aiMesh->mVertices[i].y;
        vertex.position.z = aiMesh->mVertices[i].z;

        // Normal
        if (aiMesh->HasNormals()) {
            vertex.normal.x = aiMesh->mNormals[i].x;
            vertex.normal.y = aiMesh->mNormals[i].y;
            vertex.normal.z = aiMesh->mNormals[i].z;
        }

        // Texture coordinates
        if (aiMesh->mTextureCoords[0]) {
            vertex.texCoord.x = aiMesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = aiMesh->mTextureCoords[0][i].y;
        }

        // Tangent
        if (aiMesh->HasTangentsAndBitangents()) {
            vertex.tangent.x = aiMesh->mTangents[i].x;
            vertex.tangent.y = aiMesh->mTangents[i].y;
            vertex.tangent.z = aiMesh->mTangents[i].z;

            vertex.bitangent.x = aiMesh->mBitangents[i].x;
            vertex.bitangent.y = aiMesh->mBitangents[i].y;
            vertex.bitangent.z = aiMesh->mBitangents[i].z;
        }

        vertices.push_back(vertex);
    }

    // Extract indices
    for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i) {
        aiFace face = aiMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Set mesh data
    mesh->SetVertices(vertices);
    mesh->SetIndices(indices);

    Core::Logger::Instance().Info("MeshLoader: Loaded with Assimp - " + std::to_string(vertices.size()) + " vertices, " + 
                                  std::to_string(indices.size()) + " indices");
    return true;
}
#else
bool MeshLoader::LoadWithAssimp(const std::string& filePath, Core::Mesh* mesh) {
    (void)filePath;
    (void)mesh;
    Core::Logger::Instance().Error("MeshLoader: Assimp support not compiled in. Rebuild with USE_ASSIMP.");
    return false;
}
#endif

std::vector<std::string> MeshLoader::GetSupportedExtensions() {
    std::vector<std::string> extensions = { ".obj" };
    
#ifdef USE_ASSIMP
    // Assimp supports many formats
    extensions.push_back(".fbx");
    extensions.push_back(".gltf");
    extensions.push_back(".glb");
    extensions.push_back(".dae");
    extensions.push_back(".blend");
    extensions.push_back(".3ds");
    extensions.push_back(".ase");
    extensions.push_back(".ply");
    extensions.push_back(".stl");
#endif

    return extensions;
}

bool MeshLoader::IsFormatSupported(const std::string& filePath) {
    std::string ext = MeshLoader().GetFileExtension(filePath);
    auto supported = GetSupportedExtensions();
    return std::find(supported.begin(), supported.end(), ext) != supported.end();
}

std::string MeshLoader::GetFileExtension(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) return "";
    return ToLower(filePath.substr(dotPos));
}

std::string MeshLoader::ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

} // namespace Loaders
} // namespace Engine

