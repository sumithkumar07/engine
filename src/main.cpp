#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "core/Engine.h"
#include "core/Logger.h"

// Global engine pointer for window procedure
Engine::Core::Engine* g_engine = nullptr;

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // CRITICAL DEBUG (throttled): Log mouse/keyboard messages
    if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
        static int mouseMoveCounter = 0;
        if (uMsg == WM_MOUSEMOVE) {
            mouseMoveCounter++;
            if (mouseMoveCounter % 50 == 0) {
                std::cout << "========================================" << std::endl;
                std::cout << "MOUSE MOVE WindowProc: 0x" << std::hex << uMsg << std::dec << std::endl;
                std::cout << "========================================" << std::endl;
            }
        } else {
            std::cout << "========================================" << std::endl;
            std::cout << "MOUSE EVENT WindowProc: 0x" << std::hex << uMsg << std::dec << std::endl;
            std::cout << "========================================" << std::endl;
        }
    }
    if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) {
        std::cout << "========================================" << std::endl;
        std::cout << "!!! KEYBOARD MESSAGE IN WindowProc: 0x" << std::hex << uMsg << std::dec << std::endl;
        std::cout << "========================================" << std::endl;
    }
    
    // Debug: Log every 100th message
    static int messageCount = 0;
    messageCount++;
    if (messageCount % 100 == 0) {
        std::cout << "DEBUG: WindowProc called " << messageCount << " times, current message: 0x" << std::hex << uMsg << std::dec << std::endl;
    }
    
    // Let engine handle the message first
    if (g_engine) {
        LRESULT result = g_engine->ProcessMessage(hwnd, uMsg, wParam, lParam);
        if (result != 0) {
            return result;
        }
    } else {
        // Debug: Engine not available
        if (messageCount % 1000 == 0) {
            std::cout << "DEBUG: g_engine is null in WindowProc!" << std::endl;
        }
    }
    
    // Handle any remaining messages
    switch (uMsg) {
        case WM_PAINT:
            {
                // Let the engine handle WM_PAINT - don't duplicate rendering
                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                EndPaint(hwnd, &ps);
            }
            return 0;
        
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// Create main window
HWND CreateMainWindow(HINSTANCE hInstance, [[maybe_unused]] int nCmdShow)
{
    std::cout << "DEBUG: CreateMainWindow function called" << std::endl;
    Engine::Core::Logger::Instance().Info("CreateMainWindow function called");
    const wchar_t CLASS_NAME[] = L"AI Movie Studio Window";
    
    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; // Enable redraw and double-clicks
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);  // Black background
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    
    if (!RegisterClass(&wc)) {
        std::cout << "ERROR: Failed to register window class" << std::endl;
        Engine::Core::Logger::Instance().Error("Failed to register window class");
        return nullptr;
    }
    std::cout << "DEBUG: Window class registered successfully" << std::endl;
    Engine::Core::Logger::Instance().Info("Window class registered successfully");
    
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"AI Movie Studio V2.0",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Make sure window is visible
        CW_USEDEFAULT, CW_USEDEFAULT,
        1920, 1080,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    
    if (!hwnd) {
        std::cout << "ERROR: Failed to create window!" << std::endl;
        Engine::Core::Logger::Instance().Error("Failed to create window");
        return nullptr;
    }
    
    std::cout << "DEBUG: Window created successfully, handle: " << hwnd << std::endl;
    Engine::Core::Logger::Instance().Info("Window created successfully, handle: " + std::to_string(reinterpret_cast<uintptr_t>(hwnd)));
    
    std::cout << "DEBUG: About to call ShowWindow and UpdateWindow" << std::endl;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    std::cout << "DEBUG: ShowWindow and UpdateWindow completed" << std::endl;
    
            // Debug: Check if window is visible and enabled
            if (IsWindowVisible(hwnd)) {
                std::cout << "DEBUG: Window is visible" << std::endl;
                Engine::Core::Logger::Instance().Info("Window is visible");
            } else {
                std::cout << "DEBUG: Window is NOT visible!" << std::endl;
                Engine::Core::Logger::Instance().Error("Window is NOT visible!");
            }
            
            if (IsWindowEnabled(hwnd)) {
                std::cout << "DEBUG: Window is enabled" << std::endl;
                Engine::Core::Logger::Instance().Info("Window is enabled");
            } else {
                std::cout << "DEBUG: Window is NOT enabled!" << std::endl;
                Engine::Core::Logger::Instance().Error("Window is NOT enabled!");
            }
            
    // Try to bring window to front (but don't force focus yet - that comes later)
    std::cout << "DEBUG: Attempting to bring window to front" << std::endl;
    Engine::Core::Logger::Instance().Info("Attempting to bring window to front");
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    
    // Force an immediate WM_PAINT
    InvalidateRect(hwnd, nullptr, TRUE);
    UpdateWindow(hwnd);
    
    return hwnd;
}

// Main entry point
    int WINAPI WinMain(HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, int nCmdShow)
{
    
    // Enable console for debugging
    // Free any existing console first
    FreeConsole();
    
    // Allocate a new console
    BOOL consoleCreated = AllocConsole();
    if (!consoleCreated) {
        // If AllocConsole fails, try to attach to parent process console
        DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) {
            // Console already exists, attach to it
            AttachConsole(ATTACH_PARENT_PROCESS);
        }
    }
    
    // Redirect stdio to console
    FILE* pCout;
    FILE* pCerr;
    FILE* pCin;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    freopen_s(&pCerr, "CONOUT$", "w", stderr);
    freopen_s(&pCin, "CONIN$", "r", stdin);
    
    // Set console title
    SetConsoleTitle(L"AI Movie Studio V2.0 - Debug Console");
    
    // Make sure console is visible but DON'T make it the active window
    // (We want the main app window to be active so it receives input!)
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow) {
        ShowWindow(consoleWindow, SW_SHOW);
        // DON'T call SetForegroundWindow or BringWindowToTop on console!
        // This was stealing focus from the main window!
    }
    
    // Flush output to make sure it appears
    std::cout.flush();
    std::cerr.flush();
    
    std::cout << "=== AI MOVIE STUDIO V2.0 DEBUG CONSOLE ===" << std::endl;
    std::cout << "Starting AI Movie Studio V2.0..." << std::endl;
    
    // Initialize logger
    if (!Engine::Core::Logger::Instance().Initialize("ai_movie_studio_v2.log")) {
        std::cout << "ERROR: Failed to initialize logger" << std::endl;
        return -1;
    }
    
    std::cout << "AI Movie Studio V2.0 starting..." << std::endl;
    Engine::Core::Logger::Instance().Info("AI Movie Studio V2.0 starting...");
    
    // Create engine FIRST, before any window operations
    Engine::Core::Engine engine;
    g_engine = &engine; // Set global pointer for window procedure
    std::cout << "DEBUG: Engine created and g_engine set" << std::endl;
    Engine::Core::Logger::Instance().Info("Engine created and g_engine set");
    
    // Create main window
    std::cout << "Creating main window..." << std::endl;
    Engine::Core::Logger::Instance().Info("Creating main window...");
    std::cout << "DEBUG: About to call CreateMainWindow" << std::endl;
    Engine::Core::Logger::Instance().Info("About to call CreateMainWindow");
    HWND hwnd = CreateMainWindow(hInstance, nCmdShow);
    std::cout << "DEBUG: CreateMainWindow returned, hwnd = " << hwnd << std::endl;
    Engine::Core::Logger::Instance().Info("CreateMainWindow returned, hwnd = " + std::to_string(reinterpret_cast<uintptr_t>(hwnd)));
    if (!hwnd) {
        std::cout << "ERROR: Failed to create main window" << std::endl;
        Engine::Core::Logger::Instance().Error("Failed to create main window");
        return -1;
    }
    std::cout << "SUCCESS: Main window created successfully" << std::endl;
    Engine::Core::Logger::Instance().Info("Main window created successfully");
    
    // Initialize engine
    std::cout << "=== ENGINE INITIALIZATION ===" << std::endl;
    std::cout << "About to initialize engine..." << std::endl;
    Engine::Core::Logger::Instance().Info("About to initialize engine...");
    
    if (!engine.Initialize(hwnd, 1920, 1080)) {
        std::cout << "ERROR: Failed to initialize engine" << std::endl;
        Engine::Core::Logger::Instance().Error("Failed to initialize engine");
        return -1;
    }
    std::cout << "SUCCESS: Engine initialized successfully" << std::endl;
    Engine::Core::Logger::Instance().Info("Engine initialized successfully");
    
    // Force initial window repaint to trigger UI rendering
    std::cout << "Forcing initial window repaint..." << std::endl;
    Engine::Core::Logger::Instance().Info("Forcing initial window repaint...");
    
    InvalidateRect(hwnd, nullptr, TRUE);
    UpdateWindow(hwnd);
    
    std::cout << "Initial window repaint completed" << std::endl;
    Engine::Core::Logger::Instance().Info("Initial window repaint completed");
    
    // Main message loop
    MSG msg = {};
    bool running = true;
    
    std::cout << "=== MESSAGE LOOP DEBUGGER ===" << std::endl;
    std::cout << "Starting main message loop..." << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << ">>> CLICK ON THE MAIN WINDOW (not this console!) <<<" << std::endl;
    std::cout << ">>> The main app window is ready for interaction! <<<" << std::endl;
    std::cout << "======================================================" << std::endl;
    Engine::Core::Logger::Instance().Info("Starting main message loop...");
    
    int messageCount = 0;
    while (running) {
        messageCount++;
        if (messageCount % 100 == 0) {
            std::cout << "Message loop running... (iteration " << messageCount << ")" << std::endl;
        }
        // Process Windows messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                std::cout << "WM_QUIT received - exiting main loop" << std::endl;
                Engine::Core::Logger::Instance().Info("WM_QUIT received - exiting main loop");
                running = false;
                break;
            }
            
                    // Log WM_PAINT messages
                    if (msg.message == WM_PAINT) {
                        std::cout << "WM_PAINT message received in PeekMessage loop" << std::endl;
                        Engine::Core::Logger::Instance().Info("WM_PAINT message received in main loop");
                    }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // Only force initial repaint once
        if (messageCount == 1) {
            std::cout << "DEBUG: Forcing initial window repaint" << std::endl;
            Engine::Core::Logger::Instance().Info("Forcing initial window repaint");
            InvalidateRect(hwnd, nullptr, TRUE);
            UpdateWindow(hwnd);
        }
        
        if (!running) {
            break;
        }
        
        // Update engine
        static auto lastTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        engine.Update(deltaTime);
        // Render 3D each frame (DirectX path); UI is drawn on WM_PAINT
        engine.Render();
        
        // Only sleep if no messages are pending
        if (!PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    
    std::cout << "=== EXIT DEBUGGER ===" << std::endl;
    std::cout << "AI Movie Studio V2.0 shutting down..." << std::endl;
    std::cout << "Press any key to close this console..." << std::endl;
    Engine::Core::Logger::Instance().Info("AI Movie Studio V2.0 shutting down...");
    
    // Engine will be automatically destroyed
    Engine::Core::Logger::Instance().Shutdown();
    
    // Wait for user input before closing console
    std::cin.get();
    
    return 0;
}