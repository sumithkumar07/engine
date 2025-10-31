#pragma once

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <algorithm>

namespace Engine {
namespace Core {

/**
 * @brief Vertex structure for meshes
 */
struct Vertex {
    DirectX::XMFLOAT3 position;   // Position (x, y, z)
    DirectX::XMFLOAT3 normal;     // Normal vector (nx, ny, nz)
    DirectX::XMFLOAT2 texCoord;  // Texture coordinates (u, v)
    DirectX::XMFLOAT3 tangent;   // Tangent vector for normal mapping
    DirectX::XMFLOAT3 bitangent; // Bitangent vector for normal mapping
    
    Vertex() : position(0, 0, 0), normal(0, 1, 0), texCoord(0, 0), tangent(1, 0, 0), bitangent(0, 0, 1) {}
    Vertex(float x, float y, float z) : position(x, y, z), normal(0, 1, 0), texCoord(0, 0), tangent(1, 0, 0), bitangent(0, 0, 1) {}
};

/**
 * @brief Mesh primitive types
 */
enum class PrimitiveType {
    TriangleList,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList
};

/**
 * @brief Mesh class for AI Movie Studio
 * 
 * Represents 3D geometry with vertices, indices, and material properties.
 * Supports all standard mesh operations needed for movie production.
 */
class Mesh {
public:
    Mesh(const std::string& name);
    ~Mesh();

    // Basic properties
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    // Geometry data
    const std::vector<Vertex>& GetVertices() const { return m_vertices; }
    void SetVertices(const std::vector<Vertex>& vertices);
    void AddVertex(const Vertex& vertex);

    const std::vector<uint32_t>& GetIndices() const { return m_indices; }
    void SetIndices(const std::vector<uint32_t>& indices);
    void AddIndex(uint32_t index);

    // Primitive type
    PrimitiveType GetPrimitiveType() const { return m_primitiveType; }
    void SetPrimitiveType(PrimitiveType type) { m_primitiveType = type; }

    // Bounding box
    const DirectX::XMFLOAT3& GetMinBounds() const { return m_minBounds; }
    const DirectX::XMFLOAT3& GetMaxBounds() const { return m_maxBounds; }
    void CalculateBounds();

    // Material properties
    const std::string& GetMaterialName() const { return m_materialName; }
    void SetMaterialName(const std::string& materialName) { m_materialName = materialName; }

    // Rendering properties
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }

    bool IsWireframe() const { return m_wireframe; }
    void SetWireframe(bool wireframe) { m_wireframe = wireframe; }

    // Update and render
    void Update(float deltaTime);
    void Render(class Renderer* renderer);

    // Mesh operations
    void Clear();
    void Optimize();
    void GenerateNormals();
    void GenerateTangents();
    void Subdivide(int levels);
    void Simplify(float ratio);

    // File operations
    bool LoadFromFile(const std::string& filePath);
    bool SaveToFile(const std::string& filePath) const;

    // Primitive generation
    static std::shared_ptr<Mesh> CreateCube(const std::string& name, float size = 1.0f);
    static std::shared_ptr<Mesh> CreateSphere(const std::string& name, float radius = 1.0f, int segments = 32);
    static std::shared_ptr<Mesh> CreatePlane(const std::string& name, float width = 1.0f, float height = 1.0f);
    static std::shared_ptr<Mesh> CreateCylinder(const std::string& name, float radius = 1.0f, float height = 2.0f, int segments = 32);
    static std::shared_ptr<Mesh> CreateCone(const std::string& name, float radius = 1.0f, float height = 2.0f, int segments = 32);

private:
    std::string m_name;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    PrimitiveType m_primitiveType;
    
    DirectX::XMFLOAT3 m_minBounds;
    DirectX::XMFLOAT3 m_maxBounds;
    
    std::string m_materialName;
    bool m_visible;
    bool m_wireframe;

    // Helper methods
    void CalculateVertexNormals();
    void CalculateVertexTangents();
};

} // namespace Core
} // namespace Engine
