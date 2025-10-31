#include "core/Camera.h"
#include "core/Logger.h"
#include <DirectXMath.h>

using namespace DirectX;

namespace Engine {
namespace Core {

Camera::Camera()
    : m_id(0)
    , m_name("Unnamed Camera")
    , m_cameraType("perspective")
    , m_position(0.0f, 0.0f, -5.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_fieldOfView(45.0f)
    , m_aspectRatio(16.0f / 9.0f)
    , m_nearPlane(0.1f)
    , m_farPlane(1000.0f)
    , m_viewMatrix(XMMatrixIdentity())
    , m_projectionMatrix(XMMatrixIdentity())
    , m_active(false)
    , m_dirty(true)
    , m_isInitialized(false)
{
    Logger::Instance().Debug("Camera constructor called");
}

Camera::~Camera()
{
    Shutdown();
    Logger::Instance().Debug("Camera destructor called");
}

bool Camera::Initialize()
{
    if (m_isInitialized) {
        return true;
    }

    Logger::Instance().Debug("Initializing camera: " + m_name);

    // Update matrices
    UpdateMatrices();

    m_isInitialized = true;
    Logger::Instance().Debug("Camera initialized successfully: " + m_name);
    return true;
}

void Camera::Shutdown()
{
    if (!m_isInitialized) {
        return;
    }

    Logger::Instance().Debug("Shutting down camera: " + m_name);
    m_isInitialized = false;
}

void Camera::Update(float deltaTime)
{
    (void)deltaTime; // Suppress unused parameter warning
    if (!m_isInitialized) {
        return;
    }

    // Update matrices if dirty
    if (m_dirty) {
        UpdateMatrices();
        m_dirty = false;
    }
}

void Camera::SetPosition(const DirectX::XMFLOAT3& position)
{
    m_position = position;
    MarkDirty();
}

void Camera::SetTarget(const DirectX::XMFLOAT3& target)
{
    m_target = target;
    MarkDirty();
}

void Camera::SetUp(const DirectX::XMFLOAT3& up)
{
    m_up = up;
    MarkDirty();
}

void Camera::SetFieldOfView(float fov)
{
    if (fov > 0.0f && fov < 180.0f) {
        m_fieldOfView = fov;
        MarkDirty();
    }
}

void Camera::SetAspectRatio(float aspectRatio)
{
    if (aspectRatio > 0.0f) {
        m_aspectRatio = aspectRatio;
        MarkDirty();
    }
}

void Camera::SetNearPlane(float nearPlane)
{
    if (nearPlane > 0.0f && nearPlane < m_farPlane) {
        m_nearPlane = nearPlane;
        MarkDirty();
    }
}

void Camera::SetFarPlane(float farPlane)
{
    if (farPlane > m_nearPlane) {
        m_farPlane = farPlane;
        MarkDirty();
    }
}

DirectX::XMMATRIX Camera::GetViewProjectionMatrix() const
{
    return XMMatrixMultiply(m_viewMatrix, m_projectionMatrix);
}

void Camera::SetCameraType(const std::string& type)
{
    if (type == "perspective" || type == "orthographic") {
        m_cameraType = type;
        MarkDirty();
    }
}

void Camera::LookAt(const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up)
{
    m_target = target;
    m_up = up;
    MarkDirty();
}

void Camera::MoveForward(float distance)
{
    XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&m_target), XMLoadFloat3(&m_position)));
    XMVECTOR newPosition = XMVectorAdd(XMLoadFloat3(&m_position), XMVectorScale(forward, distance));
    XMStoreFloat3(&m_position, newPosition);
    MarkDirty();
}

void Camera::MoveBackward(float distance)
{
    MoveForward(-distance);
}

void Camera::MoveRight(float distance)
{
    XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&m_target), XMLoadFloat3(&m_position)));
    XMVECTOR right = XMVector3Cross(forward, XMLoadFloat3(&m_up));
    XMVECTOR newPosition = XMVectorAdd(XMLoadFloat3(&m_position), XMVectorScale(right, distance));
    XMStoreFloat3(&m_position, newPosition);
    MarkDirty();
}

void Camera::MoveLeft(float distance)
{
    MoveRight(-distance);
}

void Camera::MoveUp(float distance)
{
    XMVECTOR newPosition = XMVectorAdd(XMLoadFloat3(&m_position), XMVectorScale(XMLoadFloat3(&m_up), distance));
    XMStoreFloat3(&m_position, newPosition);
    MarkDirty();
}

void Camera::MoveDown(float distance)
{
    MoveUp(-distance);
}

void Camera::RotateY(float angle)
{
    XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&m_target), XMLoadFloat3(&m_position)));
    XMVECTOR right = XMVector3Cross(forward, XMLoadFloat3(&m_up));
    
    XMMATRIX rotation = XMMatrixRotationAxis(XMLoadFloat3(&m_up), angle);
    XMVECTOR newForward = XMVector3TransformNormal(forward, rotation);
    
    XMVECTOR newTarget = XMVectorAdd(XMLoadFloat3(&m_position), newForward);
    XMStoreFloat3(&m_target, newTarget);
    MarkDirty();
}

void Camera::RotateX(float angle)
{
    XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&m_target), XMLoadFloat3(&m_position)));
    XMVECTOR right = XMVector3Cross(forward, XMLoadFloat3(&m_up));
    
    XMMATRIX rotation = XMMatrixRotationAxis(right, angle);
    XMVECTOR newForward = XMVector3TransformNormal(forward, rotation);
    
    XMVECTOR newTarget = XMVectorAdd(XMLoadFloat3(&m_position), newForward);
    XMStoreFloat3(&m_target, newTarget);
    MarkDirty();
}

void Camera::RotateZ(float angle)
{
    XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&m_target), XMLoadFloat3(&m_position)));
    XMVECTOR right = XMVector3Cross(forward, XMLoadFloat3(&m_up));
    
    XMMATRIX rotation = XMMatrixRotationAxis(forward, angle);
    XMVECTOR newUp = XMVector3TransformNormal(XMLoadFloat3(&m_up), rotation);
    XMStoreFloat3(&m_up, newUp);
    MarkDirty();
}

void Camera::Orbit(float horizontalAngle, float verticalAngle, float distance)
{
    XMVECTOR target = XMLoadFloat3(&m_target);
    
    // Calculate new position based on spherical coordinates
    float x = distance * cos(verticalAngle) * sin(horizontalAngle);
    float y = distance * sin(verticalAngle);
    float z = distance * cos(verticalAngle) * cos(horizontalAngle);
    
    XMVECTOR newPosition = XMVectorAdd(target, XMVectorSet(x, y, z, 0.0f));
    XMStoreFloat3(&m_position, newPosition);
    MarkDirty();
}

void Camera::Zoom(float zoomFactor)
{
    if (zoomFactor > 0.0f) {
        m_fieldOfView = m_fieldOfView / zoomFactor;
        if (m_fieldOfView < 1.0f) m_fieldOfView = 1.0f;
        if (m_fieldOfView > 179.0f) m_fieldOfView = 179.0f;
        MarkDirty();
    }
}

void Camera::Reset()
{
    m_position = XMFLOAT3(0.0f, 0.0f, -5.0f);
    m_target = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_up = XMFLOAT3(0.0f, 1.0f, 0.0f);
    m_fieldOfView = 45.0f;
    m_aspectRatio = 16.0f / 9.0f;
    m_nearPlane = 0.1f;
    m_farPlane = 1000.0f;
    MarkDirty();
}

void Camera::UpdateViewMatrix()
{
    XMVECTOR position = XMLoadFloat3(&m_position);
    XMVECTOR target = XMLoadFloat3(&m_target);
    XMVECTOR up = XMLoadFloat3(&m_up);
    
    m_viewMatrix = XMMatrixLookAtLH(position, target, up);
}

void Camera::UpdateProjectionMatrix()
{
    if (m_cameraType == "perspective") {
        m_projectionMatrix = XMMatrixPerspectiveFovLH(
            XMConvertToRadians(m_fieldOfView),
            m_aspectRatio,
            m_nearPlane,
            m_farPlane
        );
    }
    else if (m_cameraType == "orthographic") {
        float width = 10.0f * m_aspectRatio;
        float height = 10.0f;
        m_projectionMatrix = XMMatrixOrthographicLH(width, height, m_nearPlane, m_farPlane);
    }
}

void Camera::UpdateMatrices()
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

} // namespace Core
} // namespace Engine
