#pragma once

#include <memory>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Engine {
namespace Core {

    // Forward declarations
    class SceneManager;
    class ResourceManager;

    /**
     * @brief Renderer class - Same as before, but with proper DirectX 12 management
     * 
     * DirectX 12 based renderer for 3D scene rendering.
     * Same design as the previous implementation, but with proper resource management.
     */
    class Renderer {
    public:
        Renderer();
        ~Renderer();

        // Initialization
        bool Initialize(HWND hwnd, int width, int height);
        void Shutdown();

        // Rendering
        void BeginFrame();
        void EndFrame();

        // Resize handling
        void Resize(int width, int height);

        // Getters
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        bool IsInitialized() const { return m_initialized; }

        // DirectX 12 access
        ID3D12Device* GetDevice() const { return m_device.Get(); }
        ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
        ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

    private:
        // DirectX 12 objects
        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
    UINT m_rtvDescriptorSize;
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

        // Window and rendering state
        HWND m_hwnd;
        int m_width;
        int m_height;
        bool m_initialized;

        // Private methods
        bool InitializeDirectX12();
        bool CreateDescriptorHeaps();
        bool CreateSwapChain();
        bool CreateRenderTargets();
        bool CreateDepthStencilBuffer();
        void WaitForPreviousFrame();
        void Present();
    };

} // namespace Core
} // namespace Engine
