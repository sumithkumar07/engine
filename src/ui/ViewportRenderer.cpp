#include "ui/ViewportRenderer.h"
#include "core/Renderer.h"
#include "core/Logger.h"
#include "core/Engine.h"
#include "core/SceneManager.h"
#include "core/SceneObject.h"
#include <windows.h>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM macros
#include <algorithm>
#include <iostream>
#include <DirectXMath.h>

namespace Engine {
namespace UI {

    // Forward all input messages to parent window - don't let viewport intercept clicks
    static LRESULT CALLBACK ViewportWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        // Forward ALL mouse and keyboard messages to parent window
        // The parent window (main window) will handle them through the UI system
        if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) ||
            (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)) {
            HWND parent = GetParent(hwnd);
            if (parent) {
                // Convert client coordinates to screen coordinates, then back to parent's client
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ClientToScreen(hwnd, &pt);
                ScreenToClient(parent, &pt);
                LPARAM parentLParam = MAKELPARAM(pt.x, pt.y);
                
                // Forward to parent window
                return SendMessage(parent, uMsg, wParam, parentLParam);
            }
        }
        
        // For all other messages, use default handling (rendering, painting, etc.)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    ViewportRenderer::ViewportRenderer()
        : UIComponent("ViewportRenderer", 0, 0, 800, 600),
          m_parentPanel(nullptr), m_width(0), m_height(0), m_initialized(false),
          m_viewportWindow(nullptr), m_parentWindow(nullptr),
          m_cameraPosition(0.0f, 0.0f, -5.0f),
          m_cameraTarget(0.0f, 0.0f, 0.0f),
          m_cameraUp(0.0f, 1.0f, 0.0f),
          m_fov(DirectX::XM_PIDIV4), m_nearPlane(0.1f), m_farPlane(1000.0f),
          m_cameraDistance(5.0f), m_cameraRotationX(0.0f), m_cameraRotationY(0.0f),
          m_mouseLeftDown(false), m_mouseRightDown(false), m_mouseMiddleDown(false),
          m_lastMouseX(0), m_lastMouseY(0),
          m_lightPosition(2.0f, 2.0f, 2.0f),
          m_lightColor(1.0f, 1.0f, 1.0f, 1.0f),
          m_ambientColor(0.2f, 0.2f, 0.2f, 1.0f),
          m_gizmoMode(GizmoMode::None),
          m_selectedObjectName(""),
          m_renderer(nullptr),
          m_fenceEvent(nullptr), m_fenceValue(0), m_frameIndex(0) {
        
        Engine::Core::Logger::Instance().Info("ViewportRenderer created");
    }

    ViewportRenderer::~ViewportRenderer() {
        Shutdown();
    }

    bool ViewportRenderer::Initialize(UIPanel* parentPanel) {
        std::cout << "DEBUG: ViewportRenderer::Initialize called" << std::endl;
        Engine::Core::Logger::Instance().Info("Initializing ViewportRenderer...");
        
        if (!parentPanel) {
            std::cout << "DEBUG: Parent panel is null!" << std::endl;
            Engine::Core::Logger::Instance().Error("Parent panel is null");
            return false;
        }
        
        std::cout << "DEBUG: Setting parent panel and dimensions" << std::endl;
        m_parentPanel = parentPanel;
        m_width = parentPanel->GetWidth();
        m_height = parentPanel->GetHeight();
        std::cout << "DEBUG: ViewportRenderer dimensions: " << m_width << "x" << m_height << std::endl;
        
        try {
            // Get the parent window handle from the engine
            extern Engine::Core::Engine* g_engine;
            if (!g_engine) {
                Engine::Core::Logger::Instance().Error("Global engine not available");
                return false;
            }
            
            m_parentWindow = g_engine->GetWindowHandle();
            if (!m_parentWindow) {
                Engine::Core::Logger::Instance().Error("Parent window handle is null");
                return false;
            }
            
            // Create a child window for DirectX rendering
            // Register window class for viewport with custom WndProc that forwards input to parent
            WNDCLASSEX wc = {};
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = ViewportWindowProc; // Custom proc that forwards input messages to parent
            wc.hInstance = GetModuleHandle(NULL);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = L"ViewportWindowClass";
            
            static bool classRegistered = false;
            if (!classRegistered) {
                if (!RegisterClassEx(&wc)) {
                    Engine::Core::Logger::Instance().Error("Failed to register viewport window class");
                    return false;
                }
                classRegistered = true;
                Engine::Core::Logger::Instance().Info("Viewport window class registered");
            }
            
            // Calculate viewport position (center panel coordinates)
            int viewportX = 320; // Left panel width
            int viewportY = 60;  // Top panel height
            
            // Create the child window
            // The custom ViewportWindowProc will forward all input messages to parent
            // So we don't need WS_EX_TRANSPARENT or EnableWindow(FALSE)
            m_viewportWindow = CreateWindowEx(
                0,  // No extended styles needed - WndProc handles forwarding
                L"ViewportWindowClass",
                L"Viewport",
                WS_CHILD | WS_VISIBLE,
                viewportX, viewportY,
                m_width, m_height,
                m_parentWindow,
                NULL,
                GetModuleHandle(NULL),
                NULL
            );
            
            if (!m_viewportWindow) {
                Engine::Core::Logger::Instance().Error("Failed to create viewport child window");
                return false;
            }
            
            Engine::Core::Logger::Instance().Info("Viewport child window created successfully");
            std::cout << "DEBUG: Viewport window created with custom WndProc that forwards input to parent" << std::endl;
            
            ShowWindow(m_viewportWindow, SW_SHOW);
            UpdateWindow(m_viewportWindow);
            
            // Try to initialize DirectX 12 for this child window
            Engine::Core::Logger::Instance().Info("Attempting to initialize DirectX 12 rendering...");
            
            if (g_engine && g_engine->GetRenderer()) {
                m_renderer = g_engine->GetRenderer();
                Engine::Core::Logger::Instance().Info("Using engine's DirectX 12 renderer");
                
                // Initialize DirectX 12 for the viewport child window
                if (!InitializeDirectX12()) {
                    Engine::Core::Logger::Instance().Error("Failed to initialize DirectX 12 for viewport");
                    return false;
                }
                
                Engine::Core::Logger::Instance().Info("DirectX 12 initialized for viewport child window");
            } else {
                Engine::Core::Logger::Instance().Warning("Engine renderer not available, using GDI fallback");
            }
            
            m_initialized = true;
            Engine::Core::Logger::Instance().Info("ViewportRenderer initialized successfully");
            return true;
            
        } catch (const std::exception& e) {
            Engine::Core::Logger::Instance().Error("Exception during ViewportRenderer initialization: " + std::string(e.what()));
            return false;
        } catch (...) {
            Engine::Core::Logger::Instance().Error("Unknown exception during ViewportRenderer initialization");
            return false;
        }
    }

    void ViewportRenderer::Shutdown() {
        Engine::Core::Logger::Instance().Info("Shutting down ViewportRenderer...");
        
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
        
        // Reset rendering resources
        m_vertexBuffer.Reset();
        m_indexBuffer.Reset();
        m_constantBuffer.Reset();
        m_rootSignature.Reset();
        m_pipelineState.Reset();
        
        // Destroy child window
        if (m_viewportWindow) {
            DestroyWindow(m_viewportWindow);
            m_viewportWindow = nullptr;
            Engine::Core::Logger::Instance().Info("Viewport child window destroyed");
        }
        
        m_parentPanel = nullptr;
        
        Engine::Core::Logger::Instance().Info("ViewportRenderer shutdown complete");
    }

    void ViewportRenderer::Update(float deltaTime) {
        (void)deltaTime; // Suppress unused parameter warning
        if (!m_initialized) {
            return;
        }
        
        // Render DirectX frame if swap chain is available
        if (m_swapChain) {
            BeginFrame();
            RenderScene(); // Draw the 3D cube!
            EndFrame();
        }
        
        // Update camera
        UpdateCamera();
        
        // Update scene objects
        // This could include animation, physics, etc.
    }


    void ViewportRenderer::Render(HDC hdc) {
        std::cout << "DEBUG: ViewportRenderer::Render(HDC) called" << std::endl;
        Engine::Core::Logger::Instance().Debug("ViewportRenderer::Render(HDC) called");
        
        if (!m_visible) {
            return;
        }
        
        if (!m_initialized) {
            std::cout << "DEBUG: ViewportRenderer not initialized in HDC render" << std::endl;
            Engine::Core::Logger::Instance().Error("ViewportRenderer not initialized in HDC render");
            return;
        }
        
        // If we have a DirectX renderer available, DON'T paint over it with GDI
        bool hasDirectX = (m_renderer && m_renderer->IsInitialized());
        
        if (hasDirectX) {
            // DirectX is handling rendering - skip all GDI drawing to avoid covering it
            // Just draw a thin border so the user knows where the viewport is
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 120, 215)); // Blue border
            HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)::SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, m_x, m_y, m_x + m_width, m_y + m_height);
            ::SelectObject(hdc, hOldPen);
            ::SelectObject(hdc, hOldBrush);
            DeleteObject(hPen);
            return; // Don't draw anything else - let DirectX handle it
        }
        
        // GDI fallback (if DirectX not available)
        // Draw background
        HBRUSH hBrush = CreateSolidBrush(RGB(20, 20, 30));
        RECT rect = { m_x, m_y, m_x + m_width, m_y + m_height };
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        // Draw border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(60, 60, 80));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, m_x, m_y, NULL);
        LineTo(hdc, m_x + m_width, m_y);
        LineTo(hdc, m_x + m_width, m_y + m_height);
        LineTo(hdc, m_x, m_y + m_height);
        LineTo(hdc, m_x, m_y);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw title
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT titleRect = { m_x + 10, m_y + 10, m_x + m_width - 10, m_y + 30 };
        std::wstring title = L"3D Viewport (GDI Fallback)";
        DrawText(hdc, title.c_str(), -1, &titleRect, DT_LEFT | DT_VCENTER);
        
        // Get scene objects from SceneManager
        extern Engine::Core::Engine* g_engine;
        if (g_engine && g_engine->GetSceneManager()) {
            Engine::Core::SceneManager* sceneManager = g_engine->GetSceneManager();
            auto objects = sceneManager->GetAllObjects();
            
            // Draw scene objects as simple colored squares
            int objIndex = 0;
            for (const auto& objPtr : objects) {
                if (!objPtr) continue;
                
                int objX = m_x + 50 + (objIndex * 90);
                int objY = m_y + 100;
                
                // Draw object representation (simple colored square)
                COLORREF objColor = RGB(100 + (objIndex * 50) % 155, 150, 100 + (objIndex * 30) % 100);
                HBRUSH objBrush = CreateSolidBrush(objColor);
                RECT objRect = { objX - 25, objY - 25, objX + 25, objY + 25 };
                FillRect(hdc, &objRect, objBrush);
                DeleteObject(objBrush);
                
                // Draw object border
                HPEN objPen = CreatePen(PS_SOLID, 2, RGB(200, 200, 200));
                HGDIOBJ oldObjPen = ::SelectObject(hdc, objPen);
                Rectangle(hdc, objX - 25, objY - 25, objX + 25, objY + 25);
                ::SelectObject(hdc, oldObjPen);
                DeleteObject(objPen);
                
                // Draw object name
                std::wstring wideName(objPtr->GetName().begin(), objPtr->GetName().end());
                RECT nameRect = { objX - 45, objY + 30, objX + 45, objY + 50 };
                DrawText(hdc, wideName.c_str(), -1, &nameRect, DT_CENTER | DT_VCENTER);
                
                objIndex++;
                if (objIndex >= 6) break; // Limit to 6 objects for display
            }
            
            // Draw camera info
            RECT cameraInfoRect = { m_x + 10, m_y + m_height - 60, m_x + m_width - 10, m_y + m_height - 40 };
            std::wstring cameraInfo = L"Camera: Pos(" + 
                std::to_wstring(static_cast<int>(m_cameraPosition.x)) + L", " +
                std::to_wstring(static_cast<int>(m_cameraPosition.y)) + L", " +
                std::to_wstring(static_cast<int>(m_cameraPosition.z)) + L")";
            DrawText(hdc, cameraInfo.c_str(), -1, &cameraInfoRect, DT_LEFT | DT_VCENTER);
            
            // Draw controls info
            RECT controlsRect = { m_x + 10, m_y + m_height - 35, m_x + m_width - 10, m_y + m_height - 10 };
            DrawText(hdc, L"Controls: WASD=Move | Right Mouse=Orbit | Middle Mouse=Pan | Scroll=Zoom", -1, &controlsRect, DT_LEFT | DT_VCENTER);
            
            // Render gizmo if in gizmo mode and object selected
            if (m_gizmoMode != GizmoMode::None && !m_selectedObjectName.empty()) {
                auto selectedObj = sceneManager->FindObjectByName(m_selectedObjectName);
                if (selectedObj) {
                    RenderGizmo(hdc, selectedObj->GetPosition());
                    
                    // Draw gizmo mode indicator
                    RECT gizmoModeRect = { m_x + m_width - 200, m_y + 10, m_x + m_width - 10, m_y + 30 };
                    std::wstring modeText;
                    switch (m_gizmoMode) {
                        case GizmoMode::Move: modeText = L"MODE: Move (G)"; break;
                        case GizmoMode::Rotate: modeText = L"MODE: Rotate (R)"; break;
                        case GizmoMode::Scale: modeText = L"MODE: Scale (T)"; break;
                        default: break;
                    }
                    ::SetTextColor(hdc, RGB(255, 255, 0)); // Yellow
                    DrawText(hdc, modeText.c_str(), -1, &gizmoModeRect, DT_RIGHT | DT_VCENTER);
                    ::SetTextColor(hdc, RGB(255, 255, 255)); // Reset to white
                }
            }
        } else {
            // Fallback to local scene objects if SceneManager not available
            for (size_t i = 0; i < m_sceneObjects.size(); ++i) {
                const auto& obj = m_sceneObjects[i];
                int objX = m_x + 50 + static_cast<int>(i * 90);
                int objY = m_y + 100;
                
                // Draw object representation
                HBRUSH objBrush = CreateSolidBrush(RGB(
                    static_cast<int>(obj.color.x * 255),
                    static_cast<int>(obj.color.y * 255),
                    static_cast<int>(obj.color.z * 255)
                ));
                RECT objRect = { objX - 25, objY - 25, objX + 25, objY + 25 };
                FillRect(hdc, &objRect, objBrush);
                DeleteObject(objBrush);
                
                // Draw object name
                std::wstring wideName(obj.name.begin(), obj.name.end());
                RECT nameRect = { objX - 40, objY + 30, objX + 40, objY + 50 };
                DrawText(hdc, wideName.c_str(), -1, &nameRect, DT_CENTER | DT_VCENTER);
            }
        }
        
        std::cout << "DEBUG: ViewportRenderer::Render(HDC) completed" << std::endl;
        Engine::Core::Logger::Instance().Debug("ViewportRenderer::Render(HDC) completed");
    }

    void ViewportRenderer::Resize(int width, int height) {
        if (!m_initialized || (width == m_width && height == m_height)) {
            return;
        }
        
        Engine::Core::Logger::Instance().Info("Resizing ViewportRenderer to: " + std::to_string(width) + "x" + std::to_string(height));
        
        m_width = width;
        m_height = height;
        
        // Update UIComponent bounds
        SetSize(width, height);
        
        // Update the child window position and size to match the new dimensions
        if (m_viewportWindow) {
            // Calculate viewport position (center panel coordinates)
            int viewportX = 320; // Left panel width
            int viewportY = 60;  // Top panel height
            
            // Move and resize the child window
            ::MoveWindow(m_viewportWindow, viewportX, viewportY, width, height, TRUE);
            Engine::Core::Logger::Instance().Info("Child window resized to: " + std::to_string(width) + "x" + std::to_string(height));
        }
        
        // TODO: Recreate DirectX swap chain and render targets with new size
        // For now, DirectX will continue rendering at the old size
        
        Engine::Core::Logger::Instance().Info("ViewportRenderer resized successfully");
    }

    void ViewportRenderer::SetCameraPosition(const DirectX::XMFLOAT3& position) {
        m_cameraPosition = position;
        Engine::Core::Logger::Instance().Debug("Set camera position to: (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ")");
    }

    void ViewportRenderer::SetCameraTarget(const DirectX::XMFLOAT3& target) {
        m_cameraTarget = target;
        Engine::Core::Logger::Instance().Debug("Set camera target to: (" + std::to_string(target.x) + ", " + std::to_string(target.y) + ", " + std::to_string(target.z) + ")");
    }

    void ViewportRenderer::SetCameraUp(const DirectX::XMFLOAT3& up) {
        m_cameraUp = up;
        Engine::Core::Logger::Instance().Debug("Set camera up to: (" + std::to_string(up.x) + ", " + std::to_string(up.y) + ", " + std::to_string(up.z) + ")");
    }

    void ViewportRenderer::SetCameraDistance(float distance) {
        m_cameraDistance = distance;
        Engine::Core::Logger::Instance().Debug("Set camera distance to: " + std::to_string(distance));
    }

    void ViewportRenderer::SetCameraRotation(float rotationX, float rotationY) {
        m_cameraRotationX = rotationX;
        m_cameraRotationY = rotationY;
        UpdateCameraFromRotation();
        Engine::Core::Logger::Instance().Debug("Set camera rotation to: (" + std::to_string(rotationX) + ", " + std::to_string(rotationY) + ")");
    }
    
    void ViewportRenderer::UpdateCameraFromRotation() {
        // Calculate camera position based on rotation and distance
        float cosX = cosf(m_cameraRotationX);
        float sinX = sinf(m_cameraRotationX);
        float cosY = cosf(m_cameraRotationY);
        float sinY = sinf(m_cameraRotationY);
        
        // Calculate position in spherical coordinates
        m_cameraPosition.x = m_cameraTarget.x + m_cameraDistance * cosX * sinY;
        m_cameraPosition.y = m_cameraTarget.y + m_cameraDistance * sinX;
        m_cameraPosition.z = m_cameraTarget.z + m_cameraDistance * cosX * cosY;
    }

    void ViewportRenderer::AddSceneObject(const std::string& name, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color) {
        SceneObject obj;
        obj.name = name;
        obj.position = position;
        obj.rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        obj.scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
        obj.color = color;
        obj.visible = true;
        
        m_sceneObjects.push_back(obj);
        Engine::Core::Logger::Instance().Info("Added scene object '" + name + "' at position (" + 
                std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ")");
    }

    void ViewportRenderer::RemoveSceneObject(const std::string& name) {
        auto it = std::find_if(m_sceneObjects.begin(), m_sceneObjects.end(),
            [&name](const SceneObject& obj) { return obj.name == name; });
        
        if (it != m_sceneObjects.end()) {
            m_sceneObjects.erase(it);
            Engine::Core::Logger::Instance().Info("Removed scene object '" + name + "'");
        }
    }

    void ViewportRenderer::SetObjectVisibility(const std::string& name, bool visible) {
        auto it = std::find_if(m_sceneObjects.begin(), m_sceneObjects.end(),
            [&name](const SceneObject& obj) { return obj.name == name; });
        
        if (it != m_sceneObjects.end()) {
            it->visible = visible;
            Engine::Core::Logger::Instance().Debug("Set object '" + name + "' visibility to " + (visible ? "true" : "false"));
        }
    }

    void ViewportRenderer::SetObjectPosition(const std::string& name, const DirectX::XMFLOAT3& position) {
        auto it = std::find_if(m_sceneObjects.begin(), m_sceneObjects.end(),
            [&name](const SceneObject& obj) { return obj.name == name; });
        
        if (it != m_sceneObjects.end()) {
            it->position = position;
            Engine::Core::Logger::Instance().Debug("Set object '" + name + "' position to (" + 
                    std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ")");
        }
    }

    void ViewportRenderer::SetObjectColor(const std::string& name, const DirectX::XMFLOAT4& color) {
        auto it = std::find_if(m_sceneObjects.begin(), m_sceneObjects.end(),
            [&name](const SceneObject& obj) { return obj.name == name; });
        
        if (it != m_sceneObjects.end()) {
            it->color = color;
            Engine::Core::Logger::Instance().Debug("Set object '" + name + "' color to (" + 
                    std::to_string(color.x) + ", " + std::to_string(color.y) + ", " + std::to_string(color.z) + ", " + std::to_string(color.w) + ")");
        }
    }

    void ViewportRenderer::SetLightPosition(const DirectX::XMFLOAT3& position) {
        m_lightPosition = position;
        Engine::Core::Logger::Instance().Debug("Set light position to (" + 
                std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ")");
    }

    void ViewportRenderer::SetLightColor(const DirectX::XMFLOAT4& color) {
        m_lightColor = color;
        Engine::Core::Logger::Instance().Debug("Set light color to (" + 
                std::to_string(color.x) + ", " + std::to_string(color.y) + ", " + std::to_string(color.z) + ", " + std::to_string(color.w) + ")");
    }

    void ViewportRenderer::SetAmbientColor(const DirectX::XMFLOAT4& color) {
        m_ambientColor = color;
        Engine::Core::Logger::Instance().Debug("Set ambient color to (" + 
                std::to_string(color.x) + ", " + std::to_string(color.y) + ", " + std::to_string(color.z) + ", " + std::to_string(color.w) + ")");
    }

    void ViewportRenderer::HandleMouseEvent(int x, int y, int button, bool isDown) {
        if (!m_initialized) {
            return;
        }
        
        // Convert to local coordinates
        int localX = x - m_parentPanel->GetX();
        int localY = y - m_parentPanel->GetY();
        
        if (button == 0) { // Left mouse button
            m_mouseLeftDown = isDown;
        } else if (button == 1) { // Right mouse button
            m_mouseRightDown = isDown;
        } else if (button == 2) { // Middle mouse button
            m_mouseMiddleDown = isDown;
        }
        
        if (isDown) {
            m_lastMouseX = localX;
            m_lastMouseY = localY;
        }

        // For left click: object selection
        if (button == 0 && isDown) {
            // Object selection - pick object at mouse position
            Engine::Core::Logger::Instance().Debug("Viewport clicked for object selection");
            // TODO: Implement raycast/pick when we have proper 3D rendering
        }
    }

    void ViewportRenderer::HandleMouseMove(int x, int y) {
        if (!m_initialized) {
            return;
        }
        
        // Convert to local coordinates
        int localX = x - m_parentPanel->GetX();
        int localY = y - m_parentPanel->GetY();
        
        int deltaX = localX - m_lastMouseX;
        int deltaY = localY - m_lastMouseY;
        
        // Handle camera controls based on UI.md specification
        if (m_mouseRightDown) {
            // Right Click + Drag: Orbit camera
            float sensitivity = 0.005f;
            m_cameraRotationY += deltaX * sensitivity;
            m_cameraRotationX += deltaY * sensitivity;
            
            // Clamp vertical rotation to prevent flipping
            m_cameraRotationX = std::max(-DirectX::XM_PIDIV2 + 0.1f, std::min(DirectX::XM_PIDIV2 - 0.1f, m_cameraRotationX));
            
            // Update camera position based on rotation
            UpdateCameraFromRotation();
            
            Engine::Core::Logger::Instance().Debug("Camera orbit: X=" + std::to_string(m_cameraRotationX) + " Y=" + std::to_string(m_cameraRotationY));
        } 
        else if (m_mouseMiddleDown) {
            // Middle Click + Drag: Pan camera
            float panSpeed = 0.01f;
            float rightPan = -deltaX * panSpeed;
            float upPan = deltaY * panSpeed;
            
            // Calculate camera right and up vectors
            DirectX::XMFLOAT3 forward = {
                m_cameraTarget.x - m_cameraPosition.x,
                m_cameraTarget.y - m_cameraPosition.y,
                m_cameraTarget.z - m_cameraPosition.z
            };
            
            // Right vector (perpendicular to forward and up)
            DirectX::XMFLOAT3 right = {
                -forward.z, 0.0f, forward.x
            };
            
            // Normalize right vector
            float rightLen = sqrtf(right.x * right.x + right.z * right.z);
            if (rightLen > 0.0001f) {
                right.x /= rightLen;
                right.z /= rightLen;
            }
            
            // Pan camera and target
            m_cameraPosition.x += right.x * rightPan;
            m_cameraPosition.z += right.z * rightPan;
            m_cameraPosition.y += upPan;
            
            m_cameraTarget.x += right.x * rightPan;
            m_cameraTarget.z += right.z * rightPan;
            m_cameraTarget.y += upPan;
            
            Engine::Core::Logger::Instance().Debug("Camera pan: Right=" + std::to_string(rightPan) + " Up=" + std::to_string(upPan));
        }
        
        m_lastMouseX = localX;
        m_lastMouseY = localY;
    }
    
    void ViewportRenderer::HandleMouseWheel(short wheelDelta) {
        if (!m_initialized) {
            return;
        }
        
        // Scroll: Zoom in/out (according to UI.md)
        float zoomSpeed = 0.1f;
        float zoomFactor = wheelDelta > 0 ? (1.0f - zoomSpeed) : (1.0f + zoomSpeed);
        
        m_cameraDistance *= zoomFactor;
        
        // Clamp distance to reasonable values
        m_cameraDistance = std::max(1.0f, std::min(50.0f, m_cameraDistance));
        
        // Update camera position based on new distance
        UpdateCameraFromRotation();
        
        Engine::Core::Logger::Instance().Debug("Camera zoom: Distance=" + std::to_string(m_cameraDistance));
    }

    // Private methods (placeholders for now)
    bool ViewportRenderer::InitializeDirectX12() {
        Engine::Core::Logger::Instance().Info("Initializing DirectX 12 for viewport...");
        
        if (!m_renderer) {
            Engine::Core::Logger::Instance().Error("Renderer is null");
            return false;
        }
        
        // Get the device from the main renderer
        m_device = m_renderer->GetDevice();
        if (!m_device) {
            Engine::Core::Logger::Instance().Error("Failed to get device from renderer");
            return false;
        }
        
        // Get the command queue from the main renderer
        m_commandQueue = m_renderer->GetCommandQueue();
        if (!m_commandQueue) {
            Engine::Core::Logger::Instance().Error("Failed to get command queue from renderer");
            return false;
        }
        
        // Create command allocator
        HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create command allocator");
            return false;
        }
        
        // Create command list
        hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create command list");
            return false;
        }
        
        // Close the command list (it starts in recording state)
        m_commandList->Close();
        
        // Create descriptor heaps
        if (!CreateDescriptorHeaps()) {
            return false;
        }
        
        // Create swap chain for the viewport child window
        if (!CreateSwapChain()) {
            return false;
        }
        
        // Create render targets
        if (!CreateRenderTargets()) {
            return false;
        }
        
        // Create depth stencil buffer
        if (!CreateDepthStencilBuffer()) {
            return false;
        }
        
        // Create shaders and pipeline state
        if (!CreateShaders()) {
            return false;
        }
        
        // Create vertex and index buffers
        if (!CreateBuffers()) {
            return false;
        }
        
        // Create synchronization objects
        hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create fence");
            return false;
        }
        
        m_fenceValue = 1;
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr) {
            Engine::Core::Logger::Instance().Error("Failed to create fence event");
            return false;
        }
        
        Engine::Core::Logger::Instance().Info("DirectX 12 initialized for viewport successfully");
        return true;
    }

    bool ViewportRenderer::CreateDescriptorHeaps() {
        Engine::Core::Logger::Instance().Info("Creating descriptor heaps for viewport...");
        
        // Create RTV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 2; // Double buffered
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        
        HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create RTV descriptor heap");
            return false;
        }
        
        // Create DSV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        
        hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvDescriptorHeap));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create DSV descriptor heap");
            return false;
        }
        
        // Create CBV/SRV/UAV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
        cbvSrvUavHeapDesc.NumDescriptors = 10; // Enough for constant buffers and textures
        cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        
        hr = m_device->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&m_cbvSrvUavDescriptorHeap));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create CBV/SRV/UAV descriptor heap");
            return false;
        }
        
        Engine::Core::Logger::Instance().Info("Descriptor heaps created successfully");
        return true;
    }

    bool ViewportRenderer::CreateSwapChain() {
        Engine::Core::Logger::Instance().Info("Creating swap chain for viewport child window...");
        
        if (!m_viewportWindow) {
            Engine::Core::Logger::Instance().Error("Viewport window handle is null");
            return false;
        }
        
        // Get DXGI factory
        ComPtr<IDXGIFactory4> factory;
        HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to get DXGI factory");
            return false;
        }
        
        // Describe the swap chain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = 2; // Double buffering
        swapChainDesc.Width = m_width;
        swapChainDesc.Height = m_height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        
        // Create swap chain for the viewport child window
        ComPtr<IDXGISwapChain1> swapChain1;
        hr = factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            m_viewportWindow,  // Use the child window!
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        );
        
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create swap chain for viewport");
            return false;
        }
        
        // Disable Alt+Enter fullscreen toggle
        factory->MakeWindowAssociation(m_viewportWindow, DXGI_MWA_NO_ALT_ENTER);
        
        // Get the IDXGISwapChain3 interface
        hr = swapChain1.As(&m_swapChain);
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to get SwapChain3 interface");
            return false;
        }
        
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        
        Engine::Core::Logger::Instance().Info("Swap chain created successfully for viewport");
        return true;
    }

    bool ViewportRenderer::CreateRenderTargets() {
        Engine::Core::Logger::Instance().Info("Creating render targets for viewport...");
        
        // Get the descriptor size
        UINT rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        // Get the handle to the first descriptor
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        
        // Create a RTV for each back buffer
        for (UINT i = 0; i < 2; i++) {
            HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
            if (FAILED(hr)) {
                Engine::Core::Logger::Instance().Error("Failed to get swap chain buffer " + std::to_string(i));
                return false;
            }
            
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }
        
        Engine::Core::Logger::Instance().Info("Render targets created successfully");
        return true;
    }

    bool ViewportRenderer::CreateDepthStencilBuffer() {
        Engine::Core::Logger::Instance().Info("Creating depth stencil buffer for viewport...");
        
        // Describe the depth stencil buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Width = m_width;
        resourceDesc.Height = m_height;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
        resourceDesc.SampleDesc.Count = 1;
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
            Engine::Core::Logger::Instance().Error("Failed to create depth stencil buffer");
            return false;
        }
        
        // Create the depth stencil view
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, dsvHandle);
        
        Engine::Core::Logger::Instance().Info("Depth stencil buffer created successfully");
        return true;
    }

    bool ViewportRenderer::CreateShaders() {
        Engine::Core::Logger::Instance().Info("Creating shaders for viewport...");
        
        // Define shader code inline (HLSL)
        const char* vertexShaderCode = R"(
            cbuffer SceneConstantBuffer : register(b0)
            {
                float4x4 worldViewProj;
                float4x4 world;
                float3 lightDir;
                float padding1;
                float4 lightColor;
                float4 ambientColor;
            };
            
            struct VSInput
            {
                float3 position : POSITION;
                float3 normal : NORMAL;
                float4 color : COLOR;
            };
            
            struct PSInput
            {
                float4 position : SV_POSITION;
                float3 worldPos : POSITION1;
                float3 normal : NORMAL;
                float4 color : COLOR;
            };
            
            PSInput main(VSInput input)
            {
                PSInput output;
                output.position = mul(float4(input.position, 1.0), worldViewProj);
                output.worldPos = mul(float4(input.position, 1.0), world).xyz;
                output.normal = mul(input.normal, (float3x3)world);
                output.color = input.color;
                return output;
            }
        )";
        
        const char* pixelShaderCode = R"(
            cbuffer SceneConstantBuffer : register(b0)
            {
                float4x4 worldViewProj;
                float4x4 world;
                float3 lightDir;
                float padding1;
                float4 lightColor;
                float4 ambientColor;
            };
            
            struct PSInput
            {
                float4 position : SV_POSITION;
                float3 worldPos : POSITION1;
                float3 normal : NORMAL;
                float4 color : COLOR;
            };
            
            float4 main(PSInput input) : SV_TARGET
            {
                // Normalize the normal
                float3 normal = normalize(input.normal);
                
                // Calculate lighting
                float3 lightDirection = normalize(lightDir);
                float diffuse = max(dot(normal, lightDirection), 0.0);
                
                // Combine ambient and diffuse lighting
                float4 finalColor = input.color * ambientColor;
                finalColor += input.color * lightColor * diffuse;
                finalColor.a = 1.0;
                
                return finalColor;
            }
        )";
        
        // Compile vertex shader
        ComPtr<ID3DBlob> vertexShaderBlob;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompile(
            vertexShaderCode,
            strlen(vertexShaderCode),
            nullptr,
            nullptr,
            nullptr,
            "main",
            "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
            0,
            &vertexShaderBlob,
            &errorBlob
        );
        
        if (FAILED(hr)) {
            if (errorBlob) {
                Engine::Core::Logger::Instance().Error("Vertex shader compilation failed: " + std::string((char*)errorBlob->GetBufferPointer()));
            }
            return false;
        }
        
        // Compile pixel shader
        ComPtr<ID3DBlob> pixelShaderBlob;
        hr = D3DCompile(
            pixelShaderCode,
            strlen(pixelShaderCode),
            nullptr,
            nullptr,
            nullptr,
            "main",
            "ps_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
            0,
            &pixelShaderBlob,
            &errorBlob
        );
        
        if (FAILED(hr)) {
            if (errorBlob) {
                Engine::Core::Logger::Instance().Error("Pixel shader compilation failed: " + std::string((char*)errorBlob->GetBufferPointer()));
            }
            return false;
        }
        
        // Create root signature
        D3D12_ROOT_PARAMETER rootParameter = {};
        rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameter.Descriptor.ShaderRegister = 0;
        rootParameter.Descriptor.RegisterSpace = 0;
        rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = 1;
        rootSignatureDesc.pParameters = &rootParameter;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        
        ComPtr<ID3DBlob> signatureBlob;
        hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to serialize root signature");
            return false;
        }
        
        hr = m_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create root signature");
            return false;
        }
        
        // Define input layout
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        
        // Create graphics pipeline state
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        
        hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create graphics pipeline state");
            return false;
        }
        
        Engine::Core::Logger::Instance().Info("Shaders created successfully");
        return true;
    }

    bool ViewportRenderer::CreateBuffers() {
        Engine::Core::Logger::Instance().Info("Creating vertex and index buffers...");
        
        // Define cube vertices (position, normal, color)
        struct Vertex {
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT3 normal;
            DirectX::XMFLOAT4 color;
        };
        
        // Cube vertices with normals and colors
        Vertex cubeVertices[] = {
            // Front face (Z+) - Red
            { DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
            { DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
            
            // Back face (Z-) - Green
            { DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
            { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
            { DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
            
            // Top face (Y+) - Blue
            { DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
            { DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
            
            // Bottom face (Y-) - Yellow
            { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
            { DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
            
            // Right face (X+) - Magenta
            { DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
            { DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
            
            // Left face (X-) - Cyan
            { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
            { DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
            { DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
            { DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) }
        };
        
        // Cube indices (2 triangles per face)
        UINT16 cubeIndices[] = {
            0,  1,  2,  0,  2,  3,   // Front
            4,  5,  6,  4,  6,  7,   // Back
            8,  9,  10, 8,  10, 11,  // Top
            12, 13, 14, 12, 14, 15,  // Bottom
            16, 17, 18, 16, 18, 19,  // Right
            20, 21, 22, 20, 22, 23   // Left
        };
        
        const UINT vertexBufferSize = sizeof(cubeVertices);
        const UINT indexBufferSize = sizeof(cubeIndices);
        
        // Create vertex buffer
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        
        D3D12_RESOURCE_DESC vertexBufferDesc = {};
        vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vertexBufferDesc.Width = vertexBufferSize;
        vertexBufferDesc.Height = 1;
        vertexBufferDesc.DepthOrArraySize = 1;
        vertexBufferDesc.MipLevels = 1;
        vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        vertexBufferDesc.SampleDesc.Count = 1;
        vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        HRESULT hr = m_device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &vertexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)
        );
        
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create vertex buffer");
            return false;
        }
        
        // Copy vertex data to buffer
        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange = { 0, 0 };
        hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to map vertex buffer");
            return false;
        }
        memcpy(pVertexDataBegin, cubeVertices, vertexBufferSize);
        m_vertexBuffer->Unmap(0, nullptr);
        
        // Initialize vertex buffer view
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
        
        // Create index buffer
        D3D12_RESOURCE_DESC indexBufferDesc = {};
        indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        indexBufferDesc.Width = indexBufferSize;
        indexBufferDesc.Height = 1;
        indexBufferDesc.DepthOrArraySize = 1;
        indexBufferDesc.MipLevels = 1;
        indexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        indexBufferDesc.SampleDesc.Count = 1;
        indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        hr = m_device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &indexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_indexBuffer)
        );
        
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create index buffer");
            return false;
        }
        
        // Copy index data to buffer
        UINT8* pIndexDataBegin;
        hr = m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to map index buffer");
            return false;
        }
        memcpy(pIndexDataBegin, cubeIndices, indexBufferSize);
        m_indexBuffer->Unmap(0, nullptr);
        
        // Initialize index buffer view
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = indexBufferSize;
        
        // Create constant buffer
        const UINT constantBufferSize = (sizeof(DirectX::XMFLOAT4X4) * 2 + sizeof(DirectX::XMFLOAT4) * 3 + 255) & ~255; // 256-byte aligned
        
        D3D12_RESOURCE_DESC constantBufferDesc = {};
        constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        constantBufferDesc.Width = constantBufferSize;
        constantBufferDesc.Height = 1;
        constantBufferDesc.DepthOrArraySize = 1;
        constantBufferDesc.MipLevels = 1;
        constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        constantBufferDesc.SampleDesc.Count = 1;
        constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        hr = m_device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer)
        );
        
        if (FAILED(hr)) {
            Engine::Core::Logger::Instance().Error("Failed to create constant buffer");
            return false;
        }
        
        Engine::Core::Logger::Instance().Info("Vertex and index buffers created successfully");
        return true;
    }

    void ViewportRenderer::UpdateCamera() {
        // TODO: Implement camera update
        // This would calculate the view matrix based on camera position, target, and rotation
    }

    void ViewportRenderer::RenderScene() {
        if (!m_commandList || !m_pipelineState || !m_vertexBuffer || !m_indexBuffer) {
            return;
        }
        
        // DEBUG: Verify if SceneManager connection exists
        extern Engine::Core::Engine* g_engine;
        if (g_engine && g_engine->GetSceneManager()) {
            auto objects = g_engine->GetSceneManager()->GetAllObjects();
            static int lastObjectCount = -1;
            if (static_cast<int>(objects.size()) != lastObjectCount) {
                lastObjectCount = static_cast<int>(objects.size());
                Engine::Core::Logger::Instance().Info("[ViewportRenderer] RenderScene: SceneManager has " + std::to_string(objects.size()) + " objects, but RenderScene() only renders 1 hardcoded cube!");
            }
        } else {
            static bool logged = false;
            if (!logged) {
                Engine::Core::Logger::Instance().Warning("[ViewportRenderer] RenderScene: SceneManager not available");
                logged = true;
            }
        }
        
        // Set pipeline state
        m_commandList->SetPipelineState(m_pipelineState.Get());
        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        
        // Set primitive topology
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        // Set vertex and index buffers
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->IASetIndexBuffer(&m_indexBufferView);
        
        // Update constant buffer with transformation matrices
        struct SceneConstantBuffer {
            DirectX::XMFLOAT4X4 worldViewProj;
            DirectX::XMFLOAT4X4 world;
            DirectX::XMFLOAT3 lightDir;
            float padding1;
            DirectX::XMFLOAT4 lightColor;
            DirectX::XMFLOAT4 ambientColor;
        };
        
        SceneConstantBuffer constantBufferData;
        
        // Build world matrix (rotate the cube for visual interest)
        static float rotation = 0.0f;
        rotation += 0.01f; // Slowly rotate
        
        DirectX::XMMATRIX world = DirectX::XMMatrixRotationY(rotation) * DirectX::XMMatrixRotationX(rotation * 0.5f);
        DirectX::XMStoreFloat4x4(&constantBufferData.world, DirectX::XMMatrixTranspose(world));
                
        // Build view matrix
        DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&m_cameraPosition);
        DirectX::XMVECTOR at = DirectX::XMLoadFloat3(&m_cameraTarget);
        DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&m_cameraUp);
        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, at, up);
        
        // Build projection matrix
        float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
        DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(m_fov, aspectRatio, m_nearPlane, m_farPlane);
        
        // Combine matrices
        DirectX::XMMATRIX worldViewProj = world * view * projection;
        DirectX::XMStoreFloat4x4(&constantBufferData.worldViewProj, DirectX::XMMatrixTranspose(worldViewProj));
        
        // Set lighting
        constantBufferData.lightDir = m_lightPosition;
        constantBufferData.padding1 = 0.0f;
        constantBufferData.lightColor = m_lightColor;
        constantBufferData.ambientColor = m_ambientColor;
        
        // Copy data to constant buffer
        UINT8* pConstantDataBegin;
        D3D12_RANGE readRange = { 0, 0 };
        m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pConstantDataBegin));
        memcpy(pConstantDataBegin, &constantBufferData, sizeof(constantBufferData));
        m_constantBuffer->Unmap(0, nullptr);
        
        // Set constant buffer
        m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
        
        // Draw the cube (36 indices = 12 triangles = 6 faces * 2 triangles per face)
        // ISSUE: This only draws ONE hardcoded cube, doesn't iterate SceneManager objects!
        m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
        
        // TODO: Should iterate SceneManager objects here:
        // extern Engine::Core::Engine* g_engine;
        // if (g_engine && g_engine->GetSceneManager()) {
        //     auto objects = g_engine->GetSceneManager()->GetAllObjects();
        //     for (each object) { render with object's transform }
        // }
    }

    void ViewportRenderer::BeginFrame() {
        if (!m_initialized || !m_swapChain) {
            return;
        }
        
        // Reset command allocator and command list
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);
        
        // Transition the render target from present to render target state
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrier);
        
        // Set viewport and scissor rect
        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_width);
        viewport.Height = static_cast<float>(m_height);
        viewport.MaxDepth = 1.0f;
        m_commandList->RSSetViewports(1, &viewport);
        
        D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
        m_commandList->RSSetScissorRects(1, &scissorRect);
        
        // Get RTV and DSV handles
        UINT rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += m_frameIndex * rtvDescriptorSize;
        
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        
        // Bind render targets
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
        
        // Clear render target to cornflower blue
        const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.0f };
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    void ViewportRenderer::EndFrame() {
        if (!m_initialized || !m_swapChain) {
            return;
        }
        
        // Transition the render target from render target to present state
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrier);
        
        // Close and execute command list
        m_commandList->Close();
        
        ID3D12CommandList* commandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, commandLists);
        
        // Present the frame
        m_swapChain->Present(1, 0);
        
        // Update frame index
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        
        // Wait for previous frame
        WaitForPreviousFrame();
    }

    void ViewportRenderer::WaitForPreviousFrame() {
        if (!m_fence || !m_fenceEvent) {
            return;
        }
        
        // Signal and increment the fence value
        const UINT64 fenceValue = m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), fenceValue);
        m_fenceValue++;
        
        // Wait until the fence is signaled
        if (m_fence->GetCompletedValue() < fenceValue) {
            m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    void ViewportRenderer::Present() {
        // TODO: Implement present
        // This would present the swap chain
    }

    void ViewportRenderer::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) {
        (void)lParam; // Suppress unused parameter warning
        
        // Handle keyboard events for viewport controls (according to UI.md)
        if (message == WM_KEYDOWN) {
            float moveSpeed = 0.5f;
            
            switch (wParam) {
                // WASD Camera Movement
                case 'W': {
                    // Move camera forward
                    DirectX::XMFLOAT3 forward = {
                        m_cameraTarget.x - m_cameraPosition.x,
                        0.0f, // Don't move up/down with W
                        m_cameraTarget.z - m_cameraPosition.z
                    };
                    float len = sqrtf(forward.x * forward.x + forward.z * forward.z);
                    if (len > 0.0001f) {
                        forward.x /= len;
                        forward.z /= len;
                    }
                    m_cameraPosition.x += forward.x * moveSpeed;
                    m_cameraPosition.z += forward.z * moveSpeed;
                    m_cameraTarget.x += forward.x * moveSpeed;
                    m_cameraTarget.z += forward.z * moveSpeed;
                    Engine::Core::Logger::Instance().Debug("Camera move forward (W)");
                    break;
                }
                case 'S': {
                    // Move camera backward
                    DirectX::XMFLOAT3 forward = {
                        m_cameraTarget.x - m_cameraPosition.x,
                        0.0f,
                        m_cameraTarget.z - m_cameraPosition.z
                    };
                    float len = sqrtf(forward.x * forward.x + forward.z * forward.z);
                    if (len > 0.0001f) {
                        forward.x /= len;
                        forward.z /= len;
                    }
                    m_cameraPosition.x -= forward.x * moveSpeed;
                    m_cameraPosition.z -= forward.z * moveSpeed;
                    m_cameraTarget.x -= forward.x * moveSpeed;
                    m_cameraTarget.z -= forward.z * moveSpeed;
                    Engine::Core::Logger::Instance().Debug("Camera move backward (S)");
                    break;
                }
                case 'A': {
                    // Move camera left
                    DirectX::XMFLOAT3 forward = {
                        m_cameraTarget.x - m_cameraPosition.x,
                        0.0f,
                        m_cameraTarget.z - m_cameraPosition.z
                    };
                    DirectX::XMFLOAT3 right = { -forward.z, 0.0f, forward.x };
                    float len = sqrtf(right.x * right.x + right.z * right.z);
                    if (len > 0.0001f) {
                        right.x /= len;
                        right.z /= len;
                    }
                    m_cameraPosition.x -= right.x * moveSpeed;
                    m_cameraPosition.z -= right.z * moveSpeed;
                    m_cameraTarget.x -= right.x * moveSpeed;
                    m_cameraTarget.z -= right.z * moveSpeed;
                    Engine::Core::Logger::Instance().Debug("Camera move left (A)");
                    break;
                }
                case 'D': {
                    // Move camera right
                    DirectX::XMFLOAT3 forward = {
                        m_cameraTarget.x - m_cameraPosition.x,
                        0.0f,
                        m_cameraTarget.z - m_cameraPosition.z
                    };
                    DirectX::XMFLOAT3 right = { -forward.z, 0.0f, forward.x };
                    float len = sqrtf(right.x * right.x + right.z * right.z);
                    if (len > 0.0001f) {
                        right.x /= len;
                        right.z /= len;
                    }
                    m_cameraPosition.x += right.x * moveSpeed;
                    m_cameraPosition.z += right.z * moveSpeed;
                    m_cameraTarget.x += right.x * moveSpeed;
                    m_cameraTarget.z += right.z * moveSpeed;
                    Engine::Core::Logger::Instance().Debug("Camera move right (D)");
                    break;
                }
                
                // UI.md Keyboard Shortcuts
                case 'F':
                    // F: Focus on selected object
                    Engine::Core::Logger::Instance().Info("Focus on selected object (F)");
                    // Get selected object from SceneManager and center camera on it
                    {
                        extern Engine::Core::Engine* g_engine;
                        if (g_engine && g_engine->GetSceneManager()) {
                            auto selected = g_engine->GetSceneManager()->GetSelectedObject();
                            if (selected) {
                                m_cameraTarget = selected->GetPosition();
                                UpdateCameraFromRotation();
                                Engine::Core::Logger::Instance().Info("Focused on: " + selected->GetName());
                            }
                        }
                    }
                    break;
                    
                case 'G':
                    // G: Grab/move object (transform gizmo mode)
                    m_gizmoMode = GizmoMode::Move;
                    Engine::Core::Logger::Instance().Info("Activated Move Gizmo mode (G)");
                    {
                        extern Engine::Core::Engine* g_engine;
                        if (g_engine && g_engine->GetSceneManager()) {
                            auto selected = g_engine->GetSceneManager()->GetSelectedObject();
                            if (selected) {
                                m_selectedObjectName = selected->GetName();
                            }
                        }
                    }
                    break;
                    
                case 'R':
                    // R: Rotate object (transform gizmo mode)
                    m_gizmoMode = GizmoMode::Rotate;
                    Engine::Core::Logger::Instance().Info("Activated Rotate Gizmo mode (R)");
                    {
                        extern Engine::Core::Engine* g_engine;
                        if (g_engine && g_engine->GetSceneManager()) {
                            auto selected = g_engine->GetSceneManager()->GetSelectedObject();
                            if (selected) {
                                m_selectedObjectName = selected->GetName();
                            }
                        }
                    }
                    break;
                    
                case 'T':
                    // T: Scale object (S is used for backward movement in WASD)
                    m_gizmoMode = GizmoMode::Scale;
                    Engine::Core::Logger::Instance().Info("Activated Scale Gizmo mode (T)");
                    {
                        extern Engine::Core::Engine* g_engine;
                        if (g_engine && g_engine->GetSceneManager()) {
                            auto selected = g_engine->GetSceneManager()->GetSelectedObject();
                            if (selected) {
                                m_selectedObjectName = selected->GetName();
                            }
                        }
                    }
                    break;
                    
                case VK_ESCAPE:
                    // ESC: Cancel gizmo mode
                    m_gizmoMode = GizmoMode::None;
                    m_selectedObjectName = "";
                    Engine::Core::Logger::Instance().Info("Gizmo mode cancelled");
                    break;
                    
                // Arrow keys for legacy support
                case VK_UP:
                    m_cameraPosition.y += 0.1f;
                    m_cameraTarget.y += 0.1f;
                    break;
                case VK_DOWN:
                    m_cameraPosition.y -= 0.1f;
                    m_cameraTarget.y -= 0.1f;
                    break;
                    
                // Zoom keys
                case VK_ADD:
                case VK_OEM_PLUS:
                    m_cameraDistance -= 0.5f;
                    m_cameraDistance = std::max(1.0f, m_cameraDistance);
                    UpdateCameraFromRotation();
                    break;
                case VK_SUBTRACT:
                case VK_OEM_MINUS:
                    m_cameraDistance += 0.5f;
                    m_cameraDistance = std::min(50.0f, m_cameraDistance);
                    UpdateCameraFromRotation();
                    break;
            }
        }
    }
    
    // Transform Gizmo Rendering Functions
    void ViewportRenderer::WorldToScreen(const DirectX::XMFLOAT3& worldPos, int& screenX, int& screenY) {
        // Simple orthographic-style projection for gizmo positioning
        // In a real implementation, this would use proper view-projection matrices
        
        // Calculate vector from camera to world position
        float dx = worldPos.x - m_cameraPosition.x;
        float dy = worldPos.y - m_cameraPosition.y;
        float dz = worldPos.z - m_cameraPosition.z;
        
        // Simple perspective projection
        float distance = sqrtf(dx*dx + dy*dy + dz*dz);
        if (distance < 0.0001f) distance = 0.0001f;
        
        float scale = 200.0f / distance; // Scale factor based on distance
        
        // Project to screen space (center of viewport)
        screenX = m_x + m_width / 2 + static_cast<int>(dx * scale);
        screenY = m_y + m_height / 2 - static_cast<int>(dy * scale); // Flip Y for screen coords
    }
    
    void ViewportRenderer::RenderGizmo(HDC hdc, const DirectX::XMFLOAT3& position) {
        int screenX, screenY;
        WorldToScreen(position, screenX, screenY);
        
        switch (m_gizmoMode) {
            case GizmoMode::Move:
                RenderMoveGizmo(hdc, screenX, screenY);
                break;
            case GizmoMode::Rotate:
                RenderRotateGizmo(hdc, screenX, screenY);
                break;
            case GizmoMode::Scale:
                RenderScaleGizmo(hdc, screenX, screenY);
                break;
            default:
                break;
        }
    }
    
    void ViewportRenderer::RenderMoveGizmo(HDC hdc, int screenX, int screenY) {
        // Draw 3 colored arrows for X, Y, Z axes
        int arrowLength = 60;
        int arrowHeadSize = 10;
        
        // X Axis (Red)
        HPEN redPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
        HGDIOBJ oldPen = SelectObject(hdc, redPen);
        MoveToEx(hdc, screenX, screenY, NULL);
        LineTo(hdc, screenX + arrowLength, screenY);
        // Arrow head
        LineTo(hdc, screenX + arrowLength - arrowHeadSize, screenY - arrowHeadSize/2);
        MoveToEx(hdc, screenX + arrowLength, screenY, NULL);
        LineTo(hdc, screenX + arrowLength - arrowHeadSize, screenY + arrowHeadSize/2);
        SelectObject(hdc, oldPen);
        DeleteObject(redPen);
        
        // Y Axis (Green)
        HPEN greenPen = CreatePen(PS_SOLID, 3, RGB(0, 255, 0));
        SelectObject(hdc, greenPen);
        MoveToEx(hdc, screenX, screenY, NULL);
        LineTo(hdc, screenX, screenY - arrowLength);
        // Arrow head
        LineTo(hdc, screenX - arrowHeadSize/2, screenY - arrowLength + arrowHeadSize);
        MoveToEx(hdc, screenX, screenY - arrowLength, NULL);
        LineTo(hdc, screenX + arrowHeadSize/2, screenY - arrowLength + arrowHeadSize);
        SelectObject(hdc, oldPen);
        DeleteObject(greenPen);
        
        // Z Axis (Blue)
        HPEN bluePen = CreatePen(PS_SOLID, 3, RGB(0, 0, 255));
        SelectObject(hdc, bluePen);
        MoveToEx(hdc, screenX, screenY, NULL);
        LineTo(hdc, screenX + arrowLength/2, screenY + arrowLength/2);
        // Arrow head
        LineTo(hdc, screenX + arrowLength/2 - arrowHeadSize, screenY + arrowLength/2 - arrowHeadSize/2);
        MoveToEx(hdc, screenX + arrowLength/2, screenY + arrowLength/2, NULL);
        LineTo(hdc, screenX + arrowLength/2 - arrowHeadSize/2, screenY + arrowLength/2 - arrowHeadSize);
        SelectObject(hdc, oldPen);
        DeleteObject(bluePen);
        
        // Draw center circle
        HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        HGDIOBJ oldBrush = SelectObject(hdc, whiteBrush);
        Ellipse(hdc, screenX - 5, screenY - 5, screenX + 5, screenY + 5);
        SelectObject(hdc, oldBrush);
        DeleteObject(whiteBrush);
    }
    
    void ViewportRenderer::RenderRotateGizmo(HDC hdc, int screenX, int screenY) {
        // Draw 3 colored circles for X, Y, Z rotation
        int radius = 50;
        
        // X Axis rotation (Red circle - YZ plane)
        HPEN redPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
        HGDIOBJ oldPen = SelectObject(hdc, redPen);
        HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Ellipse(hdc, screenX - radius, screenY - radius/2, screenX + radius, screenY + radius/2);
        SelectObject(hdc, oldPen);
        DeleteObject(redPen);
        
        // Y Axis rotation (Green circle - XZ plane)
        HPEN greenPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
        SelectObject(hdc, greenPen);
        Ellipse(hdc, screenX - radius, screenY - radius, screenX + radius, screenY + radius);
        SelectObject(hdc, oldPen);
        DeleteObject(greenPen);
        
        // Z Axis rotation (Blue circle - XY plane)
        HPEN bluePen = CreatePen(PS_SOLID, 2, RGB(0, 0, 255));
        SelectObject(hdc, bluePen);
        Ellipse(hdc, screenX - radius/2, screenY - radius, screenX + radius/2, screenY + radius);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(bluePen);
        
        // Draw center circle
        HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        SelectObject(hdc, whiteBrush);
        Ellipse(hdc, screenX - 5, screenY - 5, screenX + 5, screenY + 5);
        SelectObject(hdc, oldBrush);
        DeleteObject(whiteBrush);
    }
    
    void ViewportRenderer::RenderScaleGizmo(HDC hdc, int screenX, int screenY) {
        // Draw 3 colored lines with boxes for X, Y, Z scaling
        int lineLength = 60;
        int boxSize = 8;
        
        // X Axis (Red)
        HPEN redPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
        HGDIOBJ oldPen = SelectObject(hdc, redPen);
        MoveToEx(hdc, screenX, screenY, NULL);
        LineTo(hdc, screenX + lineLength, screenY);
        // Box at end
        Rectangle(hdc, screenX + lineLength - boxSize, screenY - boxSize/2, 
                  screenX + lineLength + boxSize, screenY + boxSize/2);
        SelectObject(hdc, oldPen);
        DeleteObject(redPen);
        
        // Y Axis (Green)
        HPEN greenPen = CreatePen(PS_SOLID, 3, RGB(0, 255, 0));
        SelectObject(hdc, greenPen);
        MoveToEx(hdc, screenX, screenY, NULL);
        LineTo(hdc, screenX, screenY - lineLength);
        // Box at end
        Rectangle(hdc, screenX - boxSize/2, screenY - lineLength - boxSize, 
                  screenX + boxSize/2, screenY - lineLength + boxSize);
        SelectObject(hdc, oldPen);
        DeleteObject(greenPen);
        
        // Z Axis (Blue)
        HPEN bluePen = CreatePen(PS_SOLID, 3, RGB(0, 0, 255));
        SelectObject(hdc, bluePen);
        MoveToEx(hdc, screenX, screenY, NULL);
        LineTo(hdc, screenX + lineLength/2, screenY + lineLength/2);
        // Box at end
        Rectangle(hdc, screenX + lineLength/2 - boxSize, screenY + lineLength/2 - boxSize, 
                  screenX + lineLength/2 + boxSize, screenY + lineLength/2 + boxSize);
        SelectObject(hdc, oldPen);
        DeleteObject(bluePen);
        
        // Draw center cube
        HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        HGDIOBJ oldBrush = SelectObject(hdc, whiteBrush);
        Rectangle(hdc, screenX - 6, screenY - 6, screenX + 6, screenY + 6);
        SelectObject(hdc, oldBrush);
        DeleteObject(whiteBrush);
    }

} // namespace UI
} // namespace Engine
