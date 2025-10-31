#include "ui/UIRenderer.h"
#include "ui/UI.h"
#include "core/Logger.h"

namespace Engine {
namespace UI {

    UIRenderer::UIRenderer()
        : m_hwnd(nullptr)
        , m_width(0)
        , m_height(0)
        , m_initialized(false)
        , m_hdc(nullptr)
        , m_memoryDC(nullptr)
        , m_bitmap(nullptr)
        , m_oldBitmap(nullptr) {
        Engine::Core::Logger::Instance().Info("UIRenderer created");
    }

    UIRenderer::~UIRenderer() {
        Shutdown();
        Engine::Core::Logger::Instance().Info("UIRenderer destroyed");
    }

    bool UIRenderer::Initialize(HWND hwnd, int width, int height) {
        if (m_initialized) {
            Engine::Core::Logger::Instance().Warning("UIRenderer already initialized");
            return true;
        }

        m_hwnd = hwnd;
        m_width = width;
        m_height = height;

        // Get device context
        m_hdc = GetDC(hwnd);
        if (!m_hdc) {
            Engine::Core::Logger::Instance().Error("Failed to get device context");
            return false;
        }

        // Create memory DC for double buffering
        m_memoryDC = CreateCompatibleDC(m_hdc);
        if (!m_memoryDC) {
            Engine::Core::Logger::Instance().Error("Failed to create memory DC");
            CleanupGDIResources();
            return false;
        }

        // Create bitmap for double buffering
        m_bitmap = CreateCompatibleBitmap(m_hdc, width, height);
        if (!m_bitmap) {
            Engine::Core::Logger::Instance().Error("Failed to create bitmap");
            CleanupGDIResources();
            return false;
        }

        // Select bitmap into memory DC
        m_oldBitmap = (HBITMAP)SelectObject(m_memoryDC, m_bitmap);

        m_initialized = true;
        Engine::Core::Logger::Instance().Info("UIRenderer initialized successfully");
        return true;
    }

    void UIRenderer::Shutdown() {
        if (!m_initialized) {
            return;
        }

        CleanupGDIResources();
        m_initialized = false;
        Engine::Core::Logger::Instance().Info("UIRenderer shutdown complete");
    }

    void UIRenderer::BeginFrame() {
        if (!m_initialized) {
            return;
        }

        // Clear the back buffer
        RECT rect = { 0, 0, m_width, m_height };
        HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30)); // Dark background
        FillRect(m_memoryDC, &rect, hBrush);
        DeleteObject(hBrush);
    }

    void UIRenderer::EndFrame() {
        if (!m_initialized) {
            return;
        }

        // Copy back buffer to screen
        BitBlt(m_hdc, 0, 0, m_width, m_height, m_memoryDC, 0, 0, SRCCOPY);
    }

    void UIRenderer::Render(UIComponent* component) {
        if (!m_initialized || !component) {
            return;
        }

        // Render the component
        component->Render(m_memoryDC);
    }

    void UIRenderer::SetViewport(int x, int y, int width, int height) {
        if (!m_initialized) {
            return;
        }

        // Set viewport (for now, just store the values)
        // In a more advanced implementation, this would set up clipping regions
        (void)x; (void)y; (void)width; (void)height;
    }

    void UIRenderer::Resize(int width, int height) {
        if (!m_initialized) {
            return;
        }

        m_width = width;
        m_height = height;

        // Recreate bitmap for new size
        if (m_bitmap) {
            SelectObject(m_memoryDC, m_oldBitmap);
            DeleteObject(m_bitmap);
        }

        m_bitmap = CreateCompatibleBitmap(m_hdc, width, height);
        if (m_bitmap) {
            m_oldBitmap = (HBITMAP)SelectObject(m_memoryDC, m_bitmap);
        }
    }

    void UIRenderer::CleanupGDIResources() {
        if (m_oldBitmap && m_memoryDC) {
            SelectObject(m_memoryDC, m_oldBitmap);
            m_oldBitmap = nullptr;
        }

        if (m_bitmap) {
            DeleteObject(m_bitmap);
            m_bitmap = nullptr;
        }

        if (m_memoryDC) {
            DeleteDC(m_memoryDC);
            m_memoryDC = nullptr;
        }

        if (m_hdc) {
            ReleaseDC(m_hwnd, m_hdc);
            m_hdc = nullptr;
        }
    }

} // namespace UI
} // namespace Engine
