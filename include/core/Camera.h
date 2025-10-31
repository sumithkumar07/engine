#pragma once

#include <memory>
#include <string>
#include <DirectXMath.h>

namespace Engine {
namespace Core {

/**
 * @brief Camera class for 3D scene viewing
 * 
 * This class represents a camera in the 3D scene with position,
 * orientation, projection settings, and cinematic controls.
 */
class Camera {
public:
    Camera();
    ~Camera();

    /**
     * @brief Initialize the camera
     * @return true if initialization successful
     */
    bool Initialize();

    /**
     * @brief Shutdown the camera
     */
    void Shutdown();

    /**
     * @brief Update the camera
     * @param deltaTime Time elapsed since last update
     */
    void Update(float deltaTime);

    /**
     * @brief Get the camera ID
     * @return Camera ID
     */
    uint32_t GetId() const { return m_id; }

    /**
     * @brief Set the camera ID
     * @param id Camera ID
     */
    void SetId(uint32_t id) { m_id = id; }

    /**
     * @brief Get the camera name
     * @return Camera name
     */
    const std::string& GetName() const { return m_name; }

    /**
     * @brief Set the camera name
     * @param name Camera name
     */
    void SetName(const std::string& name) { m_name = name; }

    /**
     * @brief Get the position
     * @return Camera position
     */
    const DirectX::XMFLOAT3& GetPosition() const { return m_position; }

    /**
     * @brief Set the position
     * @param position Camera position
     */
    void SetPosition(const DirectX::XMFLOAT3& position);

    /**
     * @brief Get the target
     * @return Camera target
     */
    const DirectX::XMFLOAT3& GetTarget() const { return m_target; }

    /**
     * @brief Set the target
     * @param target Camera target
     */
    void SetTarget(const DirectX::XMFLOAT3& target);

    /**
     * @brief Get the up vector
     * @return Camera up vector
     */
    const DirectX::XMFLOAT3& GetUp() const { return m_up; }

    /**
     * @brief Set the up vector
     * @param up Camera up vector
     */
    void SetUp(const DirectX::XMFLOAT3& up);

    /**
     * @brief Get the field of view
     * @return Field of view in degrees
     */
    float GetFieldOfView() const { return m_fieldOfView; }

    /**
     * @brief Set the field of view
     * @param fov Field of view in degrees
     */
    void SetFieldOfView(float fov);

    /**
     * @brief Get the aspect ratio
     * @return Aspect ratio
     */
    float GetAspectRatio() const { return m_aspectRatio; }

    /**
     * @brief Set the aspect ratio
     * @param aspectRatio Aspect ratio
     */
    void SetAspectRatio(float aspectRatio);

    /**
     * @brief Get the near plane distance
     * @return Near plane distance
     */
    float GetNearPlane() const { return m_nearPlane; }

    /**
     * @brief Set the near plane distance
     * @param nearPlane Near plane distance
     */
    void SetNearPlane(float nearPlane);

    /**
     * @brief Get the far plane distance
     * @return Far plane distance
     */
    float GetFarPlane() const { return m_farPlane; }

    /**
     * @brief Set the far plane distance
     * @param farPlane Far plane distance
     */
    void SetFarPlane(float farPlane);

    /**
     * @brief Get the view matrix
     * @return View transformation matrix
     */
    const DirectX::XMMATRIX& GetViewMatrix() const { return m_viewMatrix; }

    /**
     * @brief Get the projection matrix
     * @return Projection matrix
     */
    const DirectX::XMMATRIX& GetProjectionMatrix() const { return m_projectionMatrix; }

    /**
     * @brief Get the view-projection matrix
     * @return Combined view-projection matrix
     */
    DirectX::XMMATRIX GetViewProjectionMatrix() const;

    /**
     * @brief Get the camera type
     * @return Camera type (perspective, orthographic)
     */
    const std::string& GetCameraType() const { return m_cameraType; }

    /**
     * @brief Set the camera type
     * @param type Camera type
     */
    void SetCameraType(const std::string& type);

    /**
     * @brief Check if the camera is active
     * @return true if active
     */
    bool IsActive() const { return m_active; }

    /**
     * @brief Set the camera as active
     * @param active Active flag
     */
    void SetActive(bool active) { m_active = active; }

    /**
     * @brief Look at a target position
     * @param target Target position
     * @param up Up vector
     */
    void LookAt(const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));

    /**
     * @brief Move the camera forward
     * @param distance Distance to move
     */
    void MoveForward(float distance);

    /**
     * @brief Move the camera backward
     * @param distance Distance to move
     */
    void MoveBackward(float distance);

    /**
     * @brief Move the camera right
     * @param distance Distance to move
     */
    void MoveRight(float distance);

    /**
     * @brief Move the camera left
     * @param distance Distance to move
     */
    void MoveLeft(float distance);

    /**
     * @brief Move the camera up
     * @param distance Distance to move
     */
    void MoveUp(float distance);

    /**
     * @brief Move the camera down
     * @param distance Distance to move
     */
    void MoveDown(float distance);

    /**
     * @brief Rotate the camera around Y axis
     * @param angle Angle in radians
     */
    void RotateY(float angle);

    /**
     * @brief Rotate the camera around X axis
     * @param angle Angle in radians
     */
    void RotateX(float angle);

    /**
     * @brief Rotate the camera around Z axis
     * @param angle Angle in radians
     */
    void RotateZ(float angle);

    /**
     * @brief Orbit around the target
     * @param horizontalAngle Horizontal angle in radians
     * @param verticalAngle Vertical angle in radians
     * @param distance Distance from target
     */
    void Orbit(float horizontalAngle, float verticalAngle, float distance);

    /**
     * @brief Zoom the camera
     * @param zoomFactor Zoom factor
     */
    void Zoom(float zoomFactor);

    /**
     * @brief Reset the camera to default position
     */
    void Reset();

    /**
     * @brief Check if the camera is dirty (needs matrix update)
     * @return true if dirty
     */
    bool IsDirty() const { return m_dirty; }

    /**
     * @brief Mark the camera as dirty
     */
    void MarkDirty() { m_dirty = true; }

private:
    // Camera properties
    uint32_t m_id;
    std::string m_name;
    std::string m_cameraType;

    // Transform properties
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT3 m_target;
    DirectX::XMFLOAT3 m_up;

    // Projection properties
    float m_fieldOfView;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;

    // Matrices
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;

    // State properties
    bool m_active;
    bool m_dirty;
    bool m_isInitialized;

    // Helper functions
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
    void UpdateMatrices();
};

} // namespace Core
} // namespace Engine