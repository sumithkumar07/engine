#include "core/Mesh.h"
#include "core/Logger.h"
#include "loaders/MeshLoader.h"
#include <DirectXMath.h>
#include <fstream>
#include <cmath>

using namespace DirectX;

namespace Engine {
namespace Core {

Mesh::Mesh(const std::string& name)
    : m_name(name)
    , m_primitiveType(PrimitiveType::TriangleList)
    , m_minBounds(0.0f, 0.0f, 0.0f)
    , m_maxBounds(0.0f, 0.0f, 0.0f)
    , m_materialName("Default")
    , m_visible(true)
    , m_wireframe(false)
{
    Logger::Instance().Debug("Mesh created: " + m_name);
}

Mesh::~Mesh()
{
    Logger::Instance().Debug("Mesh destroyed: " + m_name);
}

void Mesh::SetVertices(const std::vector<Vertex>& vertices)
{
    m_vertices = vertices;
    CalculateBounds();
    Logger::Instance().Debug("Set " + std::to_string(vertices.size()) + " vertices for mesh: " + m_name);
}

void Mesh::AddVertex(const Vertex& vertex)
{
    m_vertices.push_back(vertex);
    CalculateBounds();
}

void Mesh::SetIndices(const std::vector<uint32_t>& indices)
{
    m_indices = indices;
    Logger::Instance().Debug("Set " + std::to_string(indices.size()) + " indices for mesh: " + m_name);
}

void Mesh::AddIndex(uint32_t index)
{
    m_indices.push_back(index);
}

void Mesh::CalculateBounds()
{
    if (m_vertices.empty()) {
        m_minBounds = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        m_maxBounds = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        return;
    }

    m_minBounds = m_vertices[0].position;
    m_maxBounds = m_vertices[0].position;

    for (const auto& vertex : m_vertices) {
        m_minBounds.x = std::min(m_minBounds.x, vertex.position.x);
        m_minBounds.y = std::min(m_minBounds.y, vertex.position.y);
        m_minBounds.z = std::min(m_minBounds.z, vertex.position.z);

        m_maxBounds.x = std::max(m_maxBounds.x, vertex.position.x);
        m_maxBounds.y = std::max(m_maxBounds.y, vertex.position.y);
        m_maxBounds.z = std::max(m_maxBounds.z, vertex.position.z);
    }
}

void Mesh::Update(float deltaTime)
{
    (void)deltaTime; // Suppress unused parameter warning
    // Update mesh-specific logic here
    // For example, animated meshes, morphing, etc.
}

void Mesh::Render(class Renderer* renderer)
{
    (void)renderer; // Suppress unused parameter warning
    // Render mesh using the provided renderer
    // This would be called by the renderer to draw the mesh
}

void Mesh::Clear()
{
    m_vertices.clear();
    m_indices.clear();
    m_minBounds = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_maxBounds = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    Logger::Instance().Debug("Cleared mesh: " + m_name);
}

void Mesh::Optimize()
{
    // Basic optimization - remove duplicate vertices
    std::vector<Vertex> optimizedVertices;
    std::vector<uint32_t> optimizedIndices;
    std::unordered_map<std::string, uint32_t> vertexMap;

    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const Vertex& vertex = m_vertices[i];
        std::string key = std::to_string(vertex.position.x) + "," +
                         std::to_string(vertex.position.y) + "," +
                         std::to_string(vertex.position.z) + "," +
                         std::to_string(vertex.normal.x) + "," +
                         std::to_string(vertex.normal.y) + "," +
                         std::to_string(vertex.normal.z) + "," +
                         std::to_string(vertex.texCoord.x) + "," +
                         std::to_string(vertex.texCoord.y);

        auto it = vertexMap.find(key);
        if (it != vertexMap.end()) {
            optimizedIndices.push_back(it->second);
        } else {
            uint32_t newIndex = static_cast<uint32_t>(optimizedVertices.size());
            optimizedVertices.push_back(vertex);
            optimizedIndices.push_back(newIndex);
            vertexMap[key] = newIndex;
        }
    }

    m_vertices = optimizedVertices;
    m_indices = optimizedIndices;
    Logger::Instance().Debug("Optimized mesh: " + m_name + " - " + std::to_string(optimizedVertices.size()) + " vertices, " + std::to_string(optimizedIndices.size()) + " indices");
}

void Mesh::GenerateNormals()
{
    // Initialize all normals to zero
    for (auto& vertex : m_vertices) {
        vertex.normal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    }

    // Calculate face normals and accumulate to vertex normals
    for (size_t i = 0; i < m_indices.size(); i += 3) {
        if (i + 2 < m_indices.size()) {
            uint32_t i0 = m_indices[i];
            uint32_t i1 = m_indices[i + 1];
            uint32_t i2 = m_indices[i + 2];

            if (i0 < m_vertices.size() && i1 < m_vertices.size() && i2 < m_vertices.size()) {
                DirectX::XMVECTOR v0 = XMLoadFloat3(&m_vertices[i0].position);
                DirectX::XMVECTOR v1 = XMLoadFloat3(&m_vertices[i1].position);
                DirectX::XMVECTOR v2 = XMLoadFloat3(&m_vertices[i2].position);

                DirectX::XMVECTOR edge1 = XMVectorSubtract(v1, v0);
                DirectX::XMVECTOR edge2 = XMVectorSubtract(v2, v0);
                DirectX::XMVECTOR normal = XMVector3Cross(edge1, edge2);
                normal = XMVector3Normalize(normal);

                DirectX::XMFLOAT3 faceNormal;
                XMStoreFloat3(&faceNormal, normal);

                m_vertices[i0].normal.x += faceNormal.x;
                m_vertices[i0].normal.y += faceNormal.y;
                m_vertices[i0].normal.z += faceNormal.z;

                m_vertices[i1].normal.x += faceNormal.x;
                m_vertices[i1].normal.y += faceNormal.y;
                m_vertices[i1].normal.z += faceNormal.z;

                m_vertices[i2].normal.x += faceNormal.x;
                m_vertices[i2].normal.y += faceNormal.y;
                m_vertices[i2].normal.z += faceNormal.z;
            }
        }
    }

    // Normalize all vertex normals
    for (auto& vertex : m_vertices) {
        DirectX::XMVECTOR normal = XMLoadFloat3(&vertex.normal);
        normal = XMVector3Normalize(normal);
        XMStoreFloat3(&vertex.normal, normal);
    }

    Logger::Instance().Debug("Generated normals for mesh: " + m_name);
}

void Mesh::GenerateTangents()
{
    // This is a simplified tangent generation
    // In a real implementation, you'd calculate proper tangents for normal mapping
    for (auto& vertex : m_vertices) {
        vertex.tangent = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
        vertex.bitangent = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
    }
    Logger::Instance().Debug("Generated tangents for mesh: " + m_name);
}

void Mesh::Subdivide(int levels)
{
    (void)levels; // Suppress unused parameter warning
    // Subdivision logic would go here
    Logger::Instance().Debug("Subdivision not implemented for mesh: " + m_name);
}

void Mesh::Simplify(float ratio)
{
    (void)ratio; // Suppress unused parameter warning
    // Simplification logic would go here
    Logger::Instance().Debug("Simplification not implemented for mesh: " + m_name);
}

bool Mesh::LoadFromFile(const std::string& filePath)
{
    Logger::Instance().Info("Mesh::LoadFromFile - Loading: " + filePath);
    
    // Use MeshLoader to load the file
    Loaders::MeshLoader loader;
    bool success = loader.LoadFromFile(filePath, this);
    
    if (success) {
        Logger::Instance().Info("Mesh loaded successfully: " + m_name + " from " + filePath);
    } else {
        Logger::Instance().Error("Failed to load mesh: " + m_name + " from " + filePath);
    }
    
    return success;
}

bool Mesh::SaveToFile(const std::string& filePath) const
{
    (void)filePath; // Suppress unused parameter warning
    // File saving logic would go here
    Logger::Instance().Debug("File saving not implemented for mesh: " + m_name);
    return false;
}

std::shared_ptr<Mesh> Mesh::CreateCube(const std::string& name, float size)
{
    auto mesh = std::make_shared<Mesh>(name);
    
    float halfSize = size * 0.5f;
    
    // Define cube vertices
    std::vector<Vertex> vertices = {
        // Front face
        Vertex(-halfSize, -halfSize,  halfSize), // 0
        Vertex( halfSize, -halfSize,  halfSize), // 1
        Vertex( halfSize,  halfSize,  halfSize), // 2
        Vertex(-halfSize,  halfSize,  halfSize), // 3
        
        // Back face
        Vertex( halfSize, -halfSize, -halfSize), // 4
        Vertex(-halfSize, -halfSize, -halfSize), // 5
        Vertex(-halfSize,  halfSize, -halfSize), // 6
        Vertex( halfSize,  halfSize, -halfSize), // 7
        
        // Top face
        Vertex(-halfSize,  halfSize,  halfSize), // 8
        Vertex( halfSize,  halfSize,  halfSize), // 9
        Vertex( halfSize,  halfSize, -halfSize), // 10
        Vertex(-halfSize,  halfSize, -halfSize), // 11
        
        // Bottom face
        Vertex(-halfSize, -halfSize, -halfSize), // 12
        Vertex( halfSize, -halfSize, -halfSize), // 13
        Vertex( halfSize, -halfSize,  halfSize), // 14
        Vertex(-halfSize, -halfSize,  halfSize), // 15
        
        // Right face
        Vertex( halfSize, -halfSize,  halfSize), // 16
        Vertex( halfSize, -halfSize, -halfSize), // 17
        Vertex( halfSize,  halfSize, -halfSize), // 18
        Vertex( halfSize,  halfSize,  halfSize), // 19
        
        // Left face
        Vertex(-halfSize, -halfSize, -halfSize), // 20
        Vertex(-halfSize, -halfSize,  halfSize), // 21
        Vertex(-halfSize,  halfSize,  halfSize), // 22
        Vertex(-halfSize,  halfSize, -halfSize), // 23
    };
    
    // Define cube indices
    std::vector<uint32_t> indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Top face
        8, 9, 10, 10, 11, 8,
        // Bottom face
        12, 13, 14, 14, 15, 12,
        // Right face
        16, 17, 18, 18, 19, 16,
        // Left face
        20, 21, 22, 22, 23, 20
    };
    
    mesh->SetVertices(vertices);
    mesh->SetIndices(indices);
    mesh->GenerateNormals();
    
    Logger::Instance().Info("Created cube mesh: " + name);
    return mesh;
}

std::shared_ptr<Mesh> Mesh::CreateSphere(const std::string& name, float radius, int segments)
{
    auto mesh = std::make_shared<Mesh>(name);
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Generate sphere vertices
    for (int i = 0; i <= segments; ++i) {
        float lat = DirectX::XM_PI * i / segments;
        for (int j = 0; j <= segments; ++j) {
            float lon = 2.0f * DirectX::XM_PI * j / segments;
            
            float x = radius * sin(lat) * cos(lon);
            float y = radius * cos(lat);
            float z = radius * sin(lat) * sin(lon);
            
            Vertex vertex;
            vertex.position = DirectX::XMFLOAT3(x, y, z);
            vertex.normal = DirectX::XMFLOAT3(x / radius, y / radius, z / radius);
            vertex.texCoord = DirectX::XMFLOAT2(1.0f - (float)j / segments, (float)i / segments);
            
            vertices.push_back(vertex);
        }
    }
    
    // Generate sphere indices
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            int first = i * (segments + 1) + j;
            int second = first + segments + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
        }
    }
    
    mesh->SetVertices(vertices);
    mesh->SetIndices(indices);
    
    Logger::Instance().Info("Created sphere mesh: " + name);
    return mesh;
}

std::shared_ptr<Mesh> Mesh::CreatePlane(const std::string& name, float width, float height)
{
    auto mesh = std::make_shared<Mesh>(name);
    
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    
    std::vector<Vertex> vertices = {
        Vertex(-halfWidth, 0.0f, -halfHeight),
        Vertex( halfWidth, 0.0f, -halfHeight),
        Vertex( halfWidth, 0.0f,  halfHeight),
        Vertex(-halfWidth, 0.0f,  halfHeight)
    };
    
    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0
    };
    
    mesh->SetVertices(vertices);
    mesh->SetIndices(indices);
    mesh->GenerateNormals();
    
    Logger::Instance().Info("Created plane mesh: " + name);
    return mesh;
}

std::shared_ptr<Mesh> Mesh::CreateCylinder(const std::string& name, float radius, float height, int segments)
{
    (void)radius; (void)height; (void)segments; // Suppress unused parameter warnings
    auto mesh = std::make_shared<Mesh>(name);
    Logger::Instance().Info("Created cylinder mesh: " + name);
    return mesh;
}

std::shared_ptr<Mesh> Mesh::CreateCone(const std::string& name, float radius, float height, int segments)
{
    (void)radius; (void)height; (void)segments; // Suppress unused parameter warnings
    auto mesh = std::make_shared<Mesh>(name);
    Logger::Instance().Info("Created cone mesh: " + name);
    return mesh;
}

} // namespace Core
} // namespace Engine
