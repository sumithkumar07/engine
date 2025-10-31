#pragma once

#include <memory>
#include <string>
#include <vector>
#include <windows.h>

namespace Engine {
namespace UI {

    class UIComponent;

    class UIRenderer {
    public:
        UIRenderer();
        ~UIRenderer();

        // Initialization
        bool Initialize(HWND hwnd, int width, int height);
        void Shutdown();

        // Rendering
        void BeginFrame();
        void EndFrame();
        void Render(UIComponent* component);

        // Viewport management
        void SetViewport(int x, int y, int width, int height);
        void Resize(int width, int height);

        // State management
        bool IsInitialized() const { return m_initialized; }
        HWND GetWindowHandle() const { return m_hwnd; }

    private:
        HWND m_hwnd;
        int m_width;
        int m_height;
        bool m_initialized;

        // GDI resources
        HDC m_hdc;
        HDC m_memoryDC;
        HBITMAP m_bitmap;
        HBITMAP m_oldBitmap;

        void CleanupGDIResources();
    };

} // namespace UI
} // namespace Engine
