#pragma once

#include <memory>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "UI.h"
#include "core/Renderer.h"

using Microsoft::WRL::ComPtr;

namespace Engine {
namespace UI {

    // Forward declarations
    class UIPanel;

    /**
     * @brief ViewportRenderer class - Same as before, but with proper DirectX 12 integration
     * 
     * Renders the 3D viewport using DirectX 12.
     * Same design as the previous implementation, but with proper resource management.
     */
    class ViewportRenderer : public UIComponent {
    public:
        // Scene object structure
        struct SceneObject {
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT3 rotation;
            DirectX::XMFLOAT3 scale;
            DirectX::XMFLOAT4 color;
            bool visible;
            std::string name;
        };

        ViewportRenderer();
        ~ViewportRenderer();

        // UIComponent overrides
        Type GetType() const override { return Type::Panel; }

        // Initialization
        bool Initialize(UIPanel* parentPanel);
        void Shutdown();

        // Rendering
        void Update(float deltaTime) override;
        void Render(HDC hdc) override; // For UI integration
        void BeginFrame();
        void EndFrame();

        // Resize handling
        void Resize(int width, int height);

        // Camera controls
        void SetCameraPosition(const DirectX::XMFLOAT3& position);
        void SetCameraTarget(const DirectX::XMFLOAT3& target);
        void SetCameraUp(const DirectX::XMFLOAT3& up);
        void SetCameraDistance(float distance);
        void SetCameraRotation(float rotationX, float rotationY);

        // Scene management
        void AddSceneObject(const std::string& name, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color);
        void RemoveSceneObject(const std::string& name);
        void SetObjectVisibility(const std::string& name, bool visible);
        void SetObjectPosition(const std::string& name, const DirectX::XMFLOAT3& position);
        void SetObjectColor(const std::string& name, const DirectX::XMFLOAT4& color);
        const std::vector<SceneObject>& GetSceneObjects() const { return m_sceneObjects; }

        // Lighting
        void SetLightPosition(const DirectX::XMFLOAT3& position);
        void SetLightColor(const DirectX::XMFLOAT4& color);
        void SetAmbientColor(const DirectX::XMFLOAT4& color);

        // Mouse controls
        void HandleMouseEvent(int x, int y, int button, bool isDown) override;
        void HandleMouseMove(int x, int y);
        void HandleMouseWheel(short wheelDelta);
        void HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) override;

        // Getters
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        bool IsInitialized() const { return m_initialized; }

    private:
        // DirectX 12 objects
        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
        ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
        ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
        ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavDescriptorHeap;

        // Swap chain
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12Resource> m_renderTargets[2];
        ComPtr<ID3D12Resource> m_depthStencilBuffer;

        // Synchronization
        ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fenceEvent;
        UINT64 m_fenceValue;
        UINT m_frameIndex;

        // Rendering resources
        ComPtr<ID3D12Resource> m_vertexBuffer;
        ComPtr<ID3D12Resource> m_indexBuffer;
        ComPtr<ID3D12Resource> m_constantBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

        // Shaders
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12PipelineState> m_pipelineState;

        // Camera
        DirectX::XMFLOAT3 m_cameraPosition;
        DirectX::XMFLOAT3 m_cameraTarget;
        DirectX::XMFLOAT3 m_cameraUp;
        float m_fov;
        float m_nearPlane;
        float m_farPlane;
        float m_cameraDistance;
        float m_cameraRotationX;
        float m_cameraRotationY;

        // Mouse state
        bool m_mouseLeftDown;
        bool m_mouseRightDown;
        bool m_mouseMiddleDown;
        int m_lastMouseX;
        int m_lastMouseY;

        // Scene objects
        std::vector<SceneObject> m_sceneObjects;
        DirectX::XMFLOAT3 m_lightPosition;
        DirectX::XMFLOAT4 m_lightColor;
        DirectX::XMFLOAT4 m_ambientColor;

        // Transform gizmo state
        enum class GizmoMode {
            None,
            Move,
            Rotate,
            Scale
        };
        GizmoMode m_gizmoMode;
        std::string m_selectedObjectName;

        // UI panel
        UIPanel* m_parentPanel;
        
        // Child window for DirectX rendering
        HWND m_viewportWindow;
        HWND m_parentWindow;
        
        // Renderer reference
        class Engine::Core::Renderer* m_renderer;
        int m_width;
        int m_height;
        bool m_initialized;

        // Private methods
        bool InitializeDirectX12();
        bool CreateDescriptorHeaps();
        bool CreateSwapChain();
        bool CreateRenderTargets();
        bool CreateDepthStencilBuffer();
        bool CreateShaders();
        bool CreateBuffers();
        void UpdateCamera();
        void UpdateCameraFromRotation();
        void RenderScene();
        void RenderGizmo(HDC hdc, const DirectX::XMFLOAT3& position);
        void RenderMoveGizmo(HDC hdc, int screenX, int screenY);
        void RenderRotateGizmo(HDC hdc, int screenX, int screenY);
        void RenderScaleGizmo(HDC hdc, int screenX, int screenY);
        void WorldToScreen(const DirectX::XMFLOAT3& worldPos, int& screenX, int& screenY);
        void WaitForPreviousFrame();
        void Present();
    };

} // namespace UI
} // namespace Engine
