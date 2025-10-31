#include "core/Renderer.h"
#include "core/Logger.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace Engine {
namespace Core {

    Renderer::Renderer()
        : m_hwnd(nullptr), m_width(0), m_height(0), m_initialized(false),
          m_fenceValue(0), m_frameIndex(0), m_fenceEvent(nullptr) {
        
        Logger::Instance().Info("Renderer created");
    }

    Renderer::~Renderer() {
        Shutdown();
    }

    bool Renderer::Initialize(HWND hwnd, int width, int height) {
        Logger::Instance().Info("Initializing DirectX 12 renderer...");
        
        m_hwnd = hwnd;
        m_width = width;
        m_height = height;
        
        try {
            // Initialize DirectX 12
            if (!InitializeDirectX12()) {
                Logger::Instance().Error("Failed to initialize DirectX 12");
                return false;
            }
            
            // Create descriptor heaps
            if (!CreateDescriptorHeaps()) {
                Logger::Instance().Error("Failed to create descriptor heaps");
                return false;
            }
            
            // Create swap chain
            if (!CreateSwapChain()) {
                Logger::Instance().Error("Failed to create swap chain");
                return false;
            }
            
            // Create render targets
            if (!CreateRenderTargets()) {
                Logger::Instance().Error("Failed to create render targets");
                return false;
            }
            
            // Create depth stencil buffer
            if (!CreateDepthStencilBuffer()) {
                Logger::Instance().Error("Failed to create depth stencil buffer");
                return false;
            }
            
            m_initialized = true;
            Logger::Instance().Info("DirectX 12 renderer initialized successfully");
            return true;
            
        } catch (const std::exception& e) {
            Logger::Instance().Error("Exception during renderer initialization: " + std::string(e.what()));
            return false;
        } catch (...) {
            Logger::Instance().Error("Unknown exception during renderer initialization");
            return false;
        }
    }

    void Renderer::Shutdown() {
        Logger::Instance().Info("Shutting down DirectX 12 renderer...");
        
        m_initialized = false;
        
        // Wait for GPU to finish
        if (m_fence && m_fenceEvent) {
            WaitForPreviousFrame();
        }
        
        // Close fence event
        if (m_fenceEvent) {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }
        
        // Reset all COM objects
        m_renderTargets[0].Reset();
        m_renderTargets[1].Reset();
        m_depthStencilBuffer.Reset();
        m_swapChain.Reset();
        m_rtvDescriptorHeap.Reset();
        m_dsvDescriptorHeap.Reset();
        m_cbvSrvUavDescriptorHeap.Reset();
        m_commandList.Reset();
        m_commandAllocator.Reset();
        m_commandQueue.Reset();
        m_fence.Reset();
        m_device.Reset();
        
        Logger::Instance().Info("DirectX 12 renderer shutdown complete");
    }

    void Renderer::BeginFrame() {
        if (!m_initialized) {
            return;
        }

        // Reset allocator and command list for a new frame
        HRESULT hr = m_commandAllocator->Reset();
        if (FAILED(hr)) {
            char buf[128];
            sprintf_s(buf, "Command allocator reset failed: HRESULT=0x%08X", static_cast<unsigned int>(hr));
            Logger::Instance().Error(buf);
            return;
        }
        hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
        if (FAILED(hr)) {
            char buf[128];
            sprintf_s(buf, "Command list reset failed: HRESULT=0x%08X", static_cast<unsigned int>(hr));
            Logger::Instance().Error(buf);
            return;
        }
        
        Logger::Instance().Info("BeginFrame: Command list reset successfully");

        // Transition back buffer to render target
        D3D12_RESOURCE_BARRIER barrierToRT = {};
        barrierToRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierToRT.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierToRT.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrierToRT.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrierToRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrierToRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrierToRT);

        // Set viewport and scissor rect
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_width);
        viewport.Height = static_cast<float>(m_height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        
        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = m_width;
        scissorRect.bottom = m_height;
        
        m_commandList->RSSetViewports(1, &viewport);
        m_commandList->RSSetScissorRects(1, &scissorRect);
        
        // Set render target
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;
        
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
        
        // Clear render target to cornflower blue (classic DirectX color)
        const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.0f }; // RGB(100, 149, 237)
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    void Renderer::EndFrame() {
        if (!m_initialized) {
            return;
        }
        
        // Transition back buffer to present
        D3D12_RESOURCE_BARRIER barrierToPresent = {};
        barrierToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierToPresent.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrierToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrierToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrierToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrierToPresent);

        // Close command list
        HRESULT hr = m_commandList->Close();
        if (FAILED(hr)) {
            char buf[128];
            sprintf_s(buf, "Failed to close command list: HRESULT=0x%08X", static_cast<unsigned int>(hr));
            Logger::Instance().Error(buf);
            return;
        }
        
        // Execute command list
        ID3D12CommandList* commandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, commandLists);
        
        // Present frame
        hr = m_swapChain->Present(1, 0);
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to present frame: hr=" + std::to_string(hr));
            return;
        }
        
        // Wait for frame to complete
        WaitForPreviousFrame();
    }

    void Renderer::Resize(int width, int height) {
        if (!m_initialized || (width == m_width && height == m_height)) {
            return;
        }
        
        Logger::Instance().Info("Resizing renderer to: " + std::to_string(width) + "x" + std::to_string(height));
        
        m_width = width;
        m_height = height;
        
        // Wait for GPU to finish
        WaitForPreviousFrame();
        
        // Release render targets
        m_renderTargets[0].Reset();
        m_renderTargets[1].Reset();
        
        // Resize swap chain
        HRESULT hr = m_swapChain->ResizeBuffers(2, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to resize swap chain buffers");
            return;
        }
        
        // Recreate render targets
        CreateRenderTargets();
        
        // Recreate depth stencil buffer
        CreateDepthStencilBuffer();
        
        Logger::Instance().Info("Renderer resized successfully");
    }

    bool Renderer::InitializeDirectX12() {
        UINT dxgiFactoryFlags = 0;
        
#ifdef _DEBUG
        // Enable debug layer
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif
        
        // Create DXGI factory
        ComPtr<IDXGIFactory4> factory;
        HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create DXGI factory");
            return false;
        }
        
        // Create device
        hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create D3D12 device");
            return false;
        }
        
        // Create command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        
        hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create command queue");
            return false;
        }
        
        // Create command allocator
        hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create command allocator");
            return false;
        }
        
        // Create command list
        hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create command list");
            return false;
        }
        
        // Command list is created in recording state - close it so BeginFrame can reset it
        m_commandList->Close();
        
        // Create fence
        hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create fence");
            return false;
        }
        
        m_fenceValue = 1;
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr) {
            Logger::Instance().Error("Failed to create fence event");
            return false;
        }
        
        Logger::Instance().Info("DirectX 12 device created successfully");
        return true;
    }

    bool Renderer::CreateDescriptorHeaps() {
        // RTV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 2; // Double buffering
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        
        HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create RTV descriptor heap");
            return false;
        }
        
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        // DSV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        
        hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvDescriptorHeap));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create DSV descriptor heap");
            return false;
        }
        
        // CBV/SRV/UAV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
        cbvSrvUavHeapDesc.NumDescriptors = 1000; // Large enough for now
        cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        
        hr = m_device->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&m_cbvSrvUavDescriptorHeap));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create CBV/SRV/UAV descriptor heap");
            return false;
        }
        
        Logger::Instance().Info("Descriptor heaps created successfully");
        return true;
    }

    bool Renderer::CreateSwapChain() {
        // Get DXGI factory
        ComPtr<IDXGIFactory4> factory;
        HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to get DXGI factory");
            return false;
        }
        
        // Create swap chain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Width = m_width;
        swapChainDesc.Height = m_height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        
        ComPtr<IDXGISwapChain1> swapChain;
        hr = factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain);
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create swap chain");
            return false;
        }
        
        // Disable Alt+Enter fullscreen toggle
        factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
        
        // Get swap chain 3 interface
        hr = swapChain.As(&m_swapChain);
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to get swap chain 3 interface");
            return false;
        }
        
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        
        Logger::Instance().Info("Swap chain created successfully");
        return true;
    }

    bool Renderer::CreateRenderTargets() {
        // Get RTV descriptor size
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        // Create render target views
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        
        for (UINT n = 0; n < 2; n++) {
            HRESULT hr = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
            if (FAILED(hr)) {
                Logger::Instance().Error("Failed to get swap chain buffer " + std::to_string(n));
                return false;
            }
            
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += m_rtvDescriptorSize;
        }
        
        Logger::Instance().Info("Render targets created successfully");
        return true;
    }

    bool Renderer::CreateDepthStencilBuffer() {
        // Create depth stencil buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = m_width;
        resourceDesc.Height = m_height;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        
        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&m_depthStencilBuffer)
        );
        
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to create depth stencil buffer");
            return false;
        }
        
        // Create depth stencil view
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        
        m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        
        Logger::Instance().Info("Depth stencil buffer created successfully");
        return true;
    }

    void Renderer::WaitForPreviousFrame() {
        if (!m_fence || !m_fenceEvent) {
            return;
        }
        
        const UINT64 fence = m_fenceValue;
        HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fence);
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to signal fence");
            return;
        }
        
        m_fenceValue++;
        
        if (m_fence->GetCompletedValue() < fence) {
            hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            if (FAILED(hr)) {
                Logger::Instance().Error("Failed to set event on completion");
                return;
            }
            
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
        
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }

    void Renderer::Present() {
        if (!m_swapChain) {
            return;
        }
        
        HRESULT hr = m_swapChain->Present(1, 0);
        if (FAILED(hr)) {
            Logger::Instance().Error("Failed to present swap chain");
        }
    }

} // namespace Core
} // namespace Engine
