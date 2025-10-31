#include "ui/AIMovieStudioUI.h"
#include "ui/UIManager.h"
#include "ui/ViewportRenderer.h"
#include "core/Logger.h"
#include "core/Command.h"
#include "core/CommandHistory.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <windowsx.h> // For GET_X_LPARAM, GET_Y_LPARAM

// Include engine headers AFTER all UI/Windows headers to avoid macro issues
#include "core/Engine.h"
#include "core/SceneManager.h"
#include "core/SceneObject.h"
#include "core/Mesh.h"
#include "core/ResourceManager.h"
#include "core/AnimationManager.h"

// Prevent Windows.h macro collision with our SceneManager::GetObject
// Must be AFTER all Windows.h includes
#ifdef GetObject
#undef GetObject
#endif

namespace Engine {
namespace UI {

    AIMovieStudioUI::AIMovieStudioUI()
        : m_uiManager(nullptr), m_initialized(false) {
        
        Engine::Core::Logger::Instance().Info("AIMovieStudioUI created");
    }

    AIMovieStudioUI::~AIMovieStudioUI() {
        Shutdown();
        Engine::Core::Logger::Instance().Info("AIMovieStudioUI destroyed");
    }

    void AIMovieStudioUI::Update(float deltaTime) {
        // Update all UI components
        if (m_sceneHierarchy) {
            m_sceneHierarchy->Update(deltaTime);
        }
        if (m_aiCommandProcessor) {
            m_aiCommandProcessor->Update(deltaTime);
        }
        if (m_timelineSystem) {
            m_timelineSystem->Update(deltaTime);
        }
        if (m_assetBrowser) {
            m_assetBrowser->Update(deltaTime);
        }
        if (m_propertiesPanel) {
            m_propertiesPanel->Update(deltaTime);
        }
        if (m_menuSystem) {
            m_menuSystem->Update(deltaTime);
        }
        if (m_toolbarSystem) {
            m_toolbarSystem->Update(deltaTime);
        }
        if (m_viewportRenderer) {
            m_viewportRenderer->Update(deltaTime);
        }
    }

    bool AIMovieStudioUI::Initialize(UIManager* uiManager) {
        Engine::Core::Logger::Instance().Info("Initializing AIMovieStudioUI...");
        
        if (!uiManager) {
            Engine::Core::Logger::Instance().Error("UIManager is null");
            return false;
        }
        
        m_uiManager = uiManager;
        
        try {
            // Create panels
            CreatePanels();
            
            // Create components
            CreateComponents();
            
            // Update layout with window size from UIManager
            int windowWidth = uiManager->GetWidth();
            int windowHeight = uiManager->GetHeight();
            if (windowWidth > 0 && windowHeight > 0) {
                UpdateLayout(windowWidth, windowHeight);
            }
            
            m_initialized = true;
            Engine::Core::Logger::Instance().Info("AIMovieStudioUI initialized successfully");
            return true;
            
        } catch (const std::exception& e) {
            Engine::Core::Logger::Instance().Error("Exception during AIMovieStudioUI initialization: " + std::string(e.what()));
            return false;
        } catch (...) {
            Engine::Core::Logger::Instance().Error("Unknown exception during AIMovieStudioUI initialization");
            return false;
        }
    }

    void AIMovieStudioUI::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        // Convert screen coordinates to client coordinates for mouse events
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        
        // For mouse events, use the panel hierarchy to handle coordinate conversion automatically
        if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) {
            int button = 0;
            bool isDown = false;
            
            if (message == WM_LBUTTONDOWN) { button = 0; isDown = true; }
            else if (message == WM_LBUTTONUP) { button = 0; isDown = false; }
            else if (message == WM_RBUTTONDOWN) { button = 1; isDown = true; }
            else if (message == WM_RBUTTONUP) { button = 1; isDown = false; }
            else if (message == WM_MBUTTONDOWN) { button = 2; isDown = true; }
            else if (message == WM_MBUTTONUP) { button = 2; isDown = false; }
            
            // Forward to panels - they will handle coordinate conversion for their children
            HandleMouseEvent(x, y, button, isDown);
            
            // Special handling for viewport - ONLY if click is within center panel (viewport area)
            // This prevents the viewport from capturing clicks meant for other panels
            if (m_viewportRenderer && m_centerPanel) {
                // Check if click is actually in the center panel area
                if (m_centerPanel->Contains(x, y)) {
                    // Click is in viewport area - forward to viewport
                    if (message == WM_MOUSEMOVE) {
                        m_viewportRenderer->HandleMouseMove(x, y);
                    } else if (message == WM_MOUSEWHEEL) {
                        short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                        m_viewportRenderer->HandleMouseWheel(wheelDelta);
                    } else {
                        m_viewportRenderer->HandleMouseEvent(x, y, button, isDown);
                    }
                }
                // If click is NOT in center panel, don't forward to viewport
            }
        }
        
        // Forward keyboard events
        if (message >= WM_KEYFIRST && message <= WM_KEYLAST) {
            // Forward keyboard events to AICommandProcessor (for text input)
            if (m_aiCommandProcessor) {
                m_aiCommandProcessor->HandleKeyboardEvent(message, wParam, lParam);
            }
            
            // Forward to viewport for camera controls (WASD)
            if (m_viewportRenderer) {
                m_viewportRenderer->HandleKeyboardEvent(message, wParam, lParam);
            }
            
            // Forward through panel hierarchy
            HandleKeyboardEvent(message, wParam, lParam);
        }
    }

    void AIMovieStudioUI::Shutdown() {
        Engine::Core::Logger::Instance().Info("Shutting down AIMovieStudioUI...");
        
        // Shutdown components
        if (m_viewportRenderer) {
            m_viewportRenderer->Shutdown();
            m_viewportRenderer.reset();
        }
        
        if (m_sceneHierarchy) {
            m_sceneHierarchy->Shutdown();
            m_sceneHierarchy.reset();
        }
        
        if (m_aiCommandProcessor) {
            m_aiCommandProcessor->Shutdown();
            m_aiCommandProcessor.reset();
        }
        
        if (m_timelineSystem) {
            m_timelineSystem->Shutdown();
            m_timelineSystem.reset();
        }
        
        if (m_assetBrowser) {
            m_assetBrowser->Shutdown();
            m_assetBrowser.reset();
        }
        
        if (m_propertiesPanel) {
            m_propertiesPanel->Shutdown();
            m_propertiesPanel.reset();
        }
        
        if (m_menuSystem) {
            m_menuSystem->Shutdown();
            m_menuSystem.reset();
        }
        
        if (m_toolbarSystem) {
            m_toolbarSystem->Shutdown();
            m_toolbarSystem.reset();
        }
        
        // Clear sub-panels
        m_leftTopPanel.reset();
        m_leftMiddlePanel.reset();
        m_leftBottomPanel.reset();
        
        // Clear panels
        m_leftPanel.reset();
        m_centerPanel.reset();
        m_rightPanel.reset();
        m_bottomPanel.reset();
        m_topPanel.reset();
        
        m_uiManager = nullptr;
        
        Engine::Core::Logger::Instance().Info("AIMovieStudioUI shutdown complete");
    }

    void AIMovieStudioUI::UpdateLayout(int width, int height) {
        if (!m_uiManager) {
            return;
        }
        
        Engine::Core::Logger::Instance().Debug("Updating layout to: " + std::to_string(width) + "x" + std::to_string(height));
        
        // Update panel layouts
        UpdatePanelLayouts(width, height);
    }

    void AIMovieStudioUI::Render() {
        if (!m_uiManager) {
            return;
        }
        
        // Get device context
        HDC hdc = GetDC(m_uiManager->GetWindowHandle());
        if (!hdc) {
            return;
        }
        
        // Render panels
        if (m_leftPanel && m_leftPanel->IsVisible()) {
            m_leftPanel->Render(hdc);
        }
        
        if (m_centerPanel && m_centerPanel->IsVisible()) {
            m_centerPanel->Render(hdc);
        }
        
        if (m_rightPanel && m_rightPanel->IsVisible()) {
            m_rightPanel->Render(hdc);
        }
        
        if (m_bottomPanel && m_bottomPanel->IsVisible()) {
            m_bottomPanel->Render(hdc);
        }
        
        if (m_topPanel && m_topPanel->IsVisible()) {
            m_topPanel->Render(hdc);
        }
        
        // Release device context
        ReleaseDC(m_uiManager->GetWindowHandle(), hdc);
    }

    void AIMovieStudioUI::Render(HDC hdc) {
        std::cout << "DEBUG: AIMovieStudioUI::Render(HDC) called" << std::endl;
        Engine::Core::Logger::Instance().Info("AIMovieStudioUI::Render(HDC) called");
        
        if (!m_initialized) {
            std::cout << "DEBUG: AIMovieStudioUI not initialized" << std::endl;
            Engine::Core::Logger::Instance().Error("AIMovieStudioUI not initialized");
            return;
        }
        
        if (!hdc) {
            std::cout << "DEBUG: HDC is null!" << std::endl;
            Engine::Core::Logger::Instance().Error("HDC is null!");
            return;
        }
        
        // Render panels using the provided HDC
        std::cout << "=== RENDERING PANELS DEBUG ===" << std::endl;
        
        if (m_leftPanel && m_leftPanel->IsVisible()) {
            std::cout << "Rendering LeftPanel..." << std::endl;
            m_leftPanel->Render(hdc);
        }
        
        if (m_centerPanel && m_centerPanel->IsVisible()) {
            std::cout << "Rendering CenterPanel..." << std::endl;
            m_centerPanel->Render(hdc);
        }
        
        if (m_rightPanel && m_rightPanel->IsVisible()) {
            std::cout << "Rendering RightPanel..." << std::endl;
            m_rightPanel->Render(hdc);
        }
        
        std::cout << "=== END RENDERING PANELS DEBUG ===" << std::endl;
        
        if (m_bottomPanel && m_bottomPanel->IsVisible()) {
            m_bottomPanel->Render(hdc);
        }
        
        if (m_topPanel && m_topPanel->IsVisible()) {
            m_topPanel->Render(hdc);
        }
        
        std::cout << "DEBUG: AIMovieStudioUI::Render(HDC) completed" << std::endl;
        Engine::Core::Logger::Instance().Info("AIMovieStudioUI::Render(HDC) completed");
    }

    void AIMovieStudioUI::HandleMouseEvent(int x, int y, int button, bool isDown) {
        // DEBUG: Only log clicks (not moves) for non-working components
        static int clickCount = 0;
        if (isDown && button == 0) {
            clickCount++;
            if (clickCount <= 30) {
                std::cout << "[AIMovieStudioUI] ========================================" << std::endl;
                std::cout << "[AIMovieStudioUI] CLICK #" << clickCount << " at window coords (" << x << "," << y << ")" << std::endl;
                std::cout << "[AIMovieStudioUI] Checking panels: Left(" << (m_leftPanel ? "exists" : "NULL") << "), Center(" 
                          << (m_centerPanel ? "exists" : "NULL") << "), Right(" << (m_rightPanel ? "exists" : "NULL") 
                          << "), Bottom(" << (m_bottomPanel ? "exists" : "NULL") << "), Top(" 
                          << (m_topPanel ? "exists" : "NULL") << ")" << std::endl;
                
                // Check panel bounds and Contains() results
                if (m_leftPanel) {
                    std::cout << "[AIMovieStudioUI] LeftPanel: bounds=[" << m_leftPanel->GetX() << "," << m_leftPanel->GetY() 
                              << "] size=" << m_leftPanel->GetWidth() << "x" << m_leftPanel->GetHeight() 
                              << ", visible=" << m_leftPanel->IsVisible() << ", contains=" << m_leftPanel->Contains(x, y) << std::endl;
                }
                if (m_rightPanel) {
                    std::cout << "[AIMovieStudioUI] RightPanel: bounds=[" << m_rightPanel->GetX() << "," << m_rightPanel->GetY() 
                              << "] size=" << m_rightPanel->GetWidth() << "x" << m_rightPanel->GetHeight() 
                              << ", visible=" << m_rightPanel->IsVisible() << ", contains=" << m_rightPanel->Contains(x, y) << std::endl;
                }
                if (m_bottomPanel) {
                    std::cout << "[AIMovieStudioUI] BottomPanel: bounds=[" << m_bottomPanel->GetX() << "," << m_bottomPanel->GetY() 
                              << "] size=" << m_bottomPanel->GetWidth() << "x" << m_bottomPanel->GetHeight() 
                              << ", visible=" << m_bottomPanel->IsVisible() << ", contains=" << m_bottomPanel->Contains(x, y) << std::endl;
                }
                if (m_topPanel) {
                    std::cout << "[AIMovieStudioUI] TopPanel: bounds=[" << m_topPanel->GetX() << "," << m_topPanel->GetY() 
                              << "] size=" << m_topPanel->GetWidth() << "x" << m_topPanel->GetHeight() 
                              << ", visible=" << m_topPanel->IsVisible() << ", contains=" << m_topPanel->Contains(x, y) << std::endl;
                }
                std::cout << "[AIMovieStudioUI] ========================================" << std::endl;
            }
        }
        
        // Handle panels - check ALL panels, not just first match (panels don't overlap)
        bool handled = false;
        
        if (m_leftPanel && m_leftPanel->IsVisible() && m_leftPanel->Contains(x, y)) {
            if (isDown && button == 0 && clickCount <= 30) {
                std::cout << "[AIMovieStudioUI] ✓ HIT LeftPanel at (" << x << "," << y << ") - forwarding to panel" << std::endl;
            }
            m_leftPanel->HandleMouseEvent(x, y, button, isDown);
            handled = true;
        }
        
        if (m_centerPanel && m_centerPanel->IsVisible() && m_centerPanel->Contains(x, y)) {
            if (isDown && button == 0 && clickCount <= 30) {
                std::cout << "[AIMovieStudioUI] ✓ HIT CenterPanel at (" << x << "," << y << ") - forwarding to panel" << std::endl;
            }
            m_centerPanel->HandleMouseEvent(x, y, button, isDown);
            handled = true;
        }
        
        if (m_rightPanel && m_rightPanel->IsVisible() && m_rightPanel->Contains(x, y)) {
            if (isDown && button == 0 && clickCount <= 30) {
                std::cout << "[AIMovieStudioUI] ✓ HIT RightPanel at (" << x << "," << y << ") - forwarding to panel" << std::endl;
            }
            m_rightPanel->HandleMouseEvent(x, y, button, isDown);
            handled = true;
        }
        
        if (m_bottomPanel && m_bottomPanel->IsVisible() && m_bottomPanel->Contains(x, y)) {
            if (isDown && button == 0 && clickCount <= 30) {
                std::cout << "[AIMovieStudioUI] ✓ HIT BottomPanel at (" << x << "," << y << ") - forwarding to panel" << std::endl;
            }
            m_bottomPanel->HandleMouseEvent(x, y, button, isDown);
            handled = true;
        }
        
        if (m_topPanel && m_topPanel->IsVisible() && m_topPanel->Contains(x, y)) {
            if (isDown && button == 0 && clickCount <= 30) {
                std::cout << "[AIMovieStudioUI] ✓ HIT TopPanel at (" << x << "," << y << ") - forwarding to panel" << std::endl;
            }
            m_topPanel->HandleMouseEvent(x, y, button, isDown);
            handled = true;
        }
        
        if (!handled && isDown && button == 0 && clickCount <= 30) {
            std::cout << "[AIMovieStudioUI] ✗ MISS: Click at (" << x << "," << y << ") not in any panel!" << std::endl;
        }
    }

    void AIMovieStudioUI::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) {
        // Forward to panels
        if (m_leftPanel) {
            m_leftPanel->HandleKeyboardEvent(message, wParam, lParam);
        }
        if (m_centerPanel) {
            m_centerPanel->HandleKeyboardEvent(message, wParam, lParam);
        }
        if (m_rightPanel) {
            m_rightPanel->HandleKeyboardEvent(message, wParam, lParam);
        }
        if (m_bottomPanel) {
            m_bottomPanel->HandleKeyboardEvent(message, wParam, lParam);
        }
        if (m_topPanel) {
            m_topPanel->HandleKeyboardEvent(message, wParam, lParam);
        }
    }

    void AIMovieStudioUI::HandleWindowResize(int width, int height) {
        UpdateLayout(width, height);
    }

    void AIMovieStudioUI::CreatePanels() {
        Engine::Core::Logger::Instance().Debug("Creating UI panels...");
        
        // Create main panels with AI-First layout
        m_topPanel = std::make_shared<UIPanel>("TopPanel", 0, 0, 1540, TOP_PANEL_HEIGHT);
        m_leftPanel = std::make_shared<UIPanel>("LeftPanel", 0, TOP_PANEL_HEIGHT, LEFT_PANEL_WIDTH, 320);
        
        // Calculate center panel width dynamically to eliminate gaps
        int centerPanelWidth = 1540 - LEFT_PANEL_WIDTH - RIGHT_PANEL_WIDTH; // 1540 - 320 - 320 = 900
        int rightPanelX = LEFT_PANEL_WIDTH + centerPanelWidth;
        
        // DEBUG: Log panel dimensions and positions to file
        std::ofstream debugFile("panel_debug.txt", std::ios::app);
        debugFile << "=== PANEL CREATION DEBUG ===" << std::endl;
        debugFile << "Window width: 1540" << std::endl;
        debugFile << "LEFT_PANEL_WIDTH: " << LEFT_PANEL_WIDTH << std::endl;
        debugFile << "RIGHT_PANEL_WIDTH: " << RIGHT_PANEL_WIDTH << std::endl;
        debugFile << "Calculated centerPanelWidth: " << centerPanelWidth << std::endl;
        debugFile << "Right panel X position: " << rightPanelX << std::endl;
        debugFile << "Left panel: (0, " << TOP_PANEL_HEIGHT << ", " << LEFT_PANEL_WIDTH << ", 320)" << std::endl;
        debugFile << "Center panel: (" << LEFT_PANEL_WIDTH << ", " << TOP_PANEL_HEIGHT << ", " << centerPanelWidth << ", 320)" << std::endl;
        debugFile << "Right panel: (" << rightPanelX << ", " << TOP_PANEL_HEIGHT << ", " << RIGHT_PANEL_WIDTH << ", 320)" << std::endl;
        debugFile << "Total width check: " << LEFT_PANEL_WIDTH << " + " << centerPanelWidth << " + " << RIGHT_PANEL_WIDTH << " = " << (LEFT_PANEL_WIDTH + centerPanelWidth + RIGHT_PANEL_WIDTH) << std::endl;
        debugFile << "=== END PANEL CREATION DEBUG ===" << std::endl;
        debugFile.close();
        
        m_centerPanel = std::make_shared<UIPanel>("CenterPanel", LEFT_PANEL_WIDTH, TOP_PANEL_HEIGHT, centerPanelWidth, 320);
        m_rightPanel = std::make_shared<UIPanel>("RightPanel", rightPanelX, TOP_PANEL_HEIGHT, RIGHT_PANEL_WIDTH, 320);
        m_bottomPanel = std::make_shared<UIPanel>("BottomPanel", 0, TOP_PANEL_HEIGHT + 320, 1540, BOTTOM_PANEL_HEIGHT);
        
        // Set window handles for panels
        HWND mainWindow = m_uiManager->GetWindowHandle();
        m_leftPanel->SetWindowHandle(mainWindow);
        m_centerPanel->SetWindowHandle(mainWindow);
        m_rightPanel->SetWindowHandle(mainWindow);
        m_bottomPanel->SetWindowHandle(mainWindow);
        m_topPanel->SetWindowHandle(mainWindow);
        
        // Set panel colors to production values
        m_leftPanel->SetBackgroundColor(0.15f, 0.15f, 0.15f, 1.0f);   // left: dark gray
        m_centerPanel->SetBackgroundColor(0.10f, 0.10f, 0.10f, 1.0f); // center: darker gray
        m_rightPanel->SetBackgroundColor(0.15f, 0.15f, 0.15f, 1.0f);  // right: dark gray
        m_bottomPanel->SetBackgroundColor(0.20f, 0.20f, 0.20f, 1.0f); // bottom: medium gray
        m_topPanel->SetBackgroundColor(0.25f, 0.25f, 0.25f, 1.0f);    // top: light gray
        
        // Set panel borders with thick white borders for debugging
        m_leftPanel->SetBorderWidth(3);
        m_centerPanel->SetBorderWidth(3);
        m_rightPanel->SetBorderWidth(3);
        m_bottomPanel->SetBorderWidth(3);
        m_topPanel->SetBorderWidth(3);
        
        // Set border colors to white for visibility
        m_leftPanel->SetBorderColor(1.0f, 1.0f, 1.0f, 1.0f);
        m_centerPanel->SetBorderColor(1.0f, 1.0f, 1.0f, 1.0f);
        m_rightPanel->SetBorderColor(1.0f, 1.0f, 1.0f, 1.0f);
        m_bottomPanel->SetBorderColor(1.0f, 1.0f, 1.0f, 1.0f);
        m_topPanel->SetBorderColor(1.0f, 1.0f, 1.0f, 1.0f);
        
        Engine::Core::Logger::Instance().Debug("UI panels created successfully");
    }

    void AIMovieStudioUI::CreateComponents() {
        Engine::Core::Logger::Instance().Debug("Creating UI components...");
        
        // Create viewport renderer
        m_viewportRenderer = std::make_shared<ViewportRenderer>();
        if (m_viewportRenderer->Initialize(m_centerPanel.get())) {
            Engine::Core::Logger::Instance().Info("Viewport renderer initialized");
        } else {
            Engine::Core::Logger::Instance().Error("Failed to initialize viewport renderer");
            m_viewportRenderer.reset();
        }
        
        // No SetEventCallback on ViewportRenderer: Not implemented, skip wiring for now.
        
        // Create other components
        m_sceneHierarchy = std::make_shared<SceneHierarchy>();
        m_aiCommandProcessor = std::make_shared<AICommandProcessor>();
        m_timelineSystem = std::make_shared<TimelineSystem>();
        m_assetBrowser = std::make_shared<AssetBrowser>();
        m_propertiesPanel = std::make_shared<PropertiesPanel>();
        m_menuSystem = std::make_shared<MenuSystem>();
        m_toolbarSystem = std::make_shared<ToolbarSystem>();
        
        // Initialize components
        if (m_sceneHierarchy && m_viewportRenderer) {
            std::cout << "DEBUG: About to initialize SceneHierarchy" << std::endl;
            Engine::Core::Logger::Instance().Debug("About to initialize SceneHierarchy");
            
            m_sceneHierarchy->Initialize(m_viewportRenderer.get());
            Engine::Core::Logger::Instance().Debug("SceneHierarchy initialized");
            
            // Add some sample objects for testing
            m_sceneHierarchy->AddObject("Main Camera", "Camera");
            m_sceneHierarchy->AddObject("Cube", "Mesh");
            m_sceneHierarchy->AddObject("Sphere", "Mesh");
            m_sceneHierarchy->AddObject("Light", "Light");
            Engine::Core::Logger::Instance().Debug("Added sample objects to SceneHierarchy");
            
            std::cout << "DEBUG: SceneHierarchy initialization completed" << std::endl;
        }
        
        if (m_propertiesPanel) {
            m_propertiesPanel->Initialize();
            Engine::Core::Logger::Instance().Debug("PropertiesPanel initialized");
            // PropertiesPanel PropertyChanged: Use Command pattern for undo support
            m_propertiesPanel->SetPropertyChangedCallback([this](const std::string& group, const std::string& name, const Engine::UI::PropertiesPanel::PropertyValue& value) {
                extern Engine::Core::Engine* g_engine;
                if (!g_engine || !g_engine->GetCommandHistory() || !g_engine->GetSceneManager()) {
                    Engine::Core::Logger::Instance().Warning("PropertiesPanel: Engine or CommandHistory not available");
                    return;
                }
                
                auto selectedObj = m_propertiesPanel->GetSelectedObject();
                if (!selectedObj) {
                    Engine::Core::Logger::Instance().Warning("PropertiesPanel: No object selected");
                    return;
                }
                
                std::string objName = selectedObj->GetName();
                
                // Create appropriate command based on property name
                std::unique_ptr<Engine::Core::Command> cmd;
                
                if (name == "position") {
                    DirectX::XMFLOAT3 newPos = std::get<DirectX::XMFLOAT3>(value.value);
                    cmd = std::make_unique<Engine::Core::SetPositionCommand>(objName, newPos);
                } else if (name == "rotation") {
                    DirectX::XMFLOAT3 newRot = std::get<DirectX::XMFLOAT3>(value.value);
                    cmd = std::make_unique<Engine::Core::SetRotationCommand>(objName, newRot);
                } else if (name == "scale") {
                    DirectX::XMFLOAT3 newScale = std::get<DirectX::XMFLOAT3>(value.value);
                    cmd = std::make_unique<Engine::Core::SetScaleCommand>(objName, newScale);
                } else {
                    // For other properties, log but don't create command
                    Engine::Core::Logger::Instance().Info("Property changed (no undo): [" + group + "] " + name);
                    return;
                }
                
                // Execute command through history (enables undo)
                if (cmd) {
                    std::cout << "========================================" << std::endl;
                    std::cout << "PROPERTIES PANEL: Editing " << name << " for " << objName << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    bool mergeable = true; // Allow merging for continuous edits
                    if (g_engine->GetCommandHistory()->ExecuteCommand(std::move(cmd), g_engine->GetSceneManager(), mergeable)) {
                        std::cout << ">>> SUCCESS: Property changed via Command: " << name << std::endl;
                        std::cout << ">>> TIP: Press Ctrl+Z to undo this change!" << std::endl;
                        Engine::Core::Logger::Instance().Info(">>> SUCCESS: Property change via Command: " + name + " for " + objName);
                        
                        // Refresh properties panel to reflect new value
                        m_propertiesPanel->SetSelectedObject(selectedObj);
                    } else {
                        std::cout << ">>> ERROR: Failed to execute property change command" << std::endl;
                        Engine::Core::Logger::Instance().Error("Failed to execute property change command");
                    }
                }
            });
        }
        
        if (m_assetBrowser) {
            std::cout << "DEBUG: About to initialize AssetBrowser" << std::endl;
            // Resolve assets path relative to executable working directory
            std::string assetsPath;
            try {
                #ifdef _WIN32
                char buffer[MAX_PATH];
                DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
                std::string exePath = (len > 0) ? std::string(buffer, buffer + len) : std::string();
                size_t pos = exePath.find_last_of("/\\");
                std::string exeDir = (pos != std::string::npos) ? exePath.substr(0, pos) : std::string(".");
                assetsPath = exeDir + "\\assets";
                #else
                assetsPath = "./assets";
                #endif
                // Create directory if missing
                CreateDirectoryA(assetsPath.c_str(), NULL);
            } catch (...) {
                assetsPath = "./assets";
            }
            
            // Write to file for debugging
            std::ofstream debugFile("aimoviestudio_debug.txt", std::ios::app);
            debugFile << "About to call AssetBrowser::Initialize() at " << std::time(nullptr) << std::endl;
            debugFile.close();
            
            m_assetBrowser->Initialize(assetsPath);
            m_assetBrowser->SetAssetImportedCallback([](const std::string& assetPath) {
                extern Engine::Core::Engine* g_engine;
                if (!g_engine) return;
                auto* sceneMgr = g_engine->GetSceneManager();
                if (!sceneMgr) return;

                // Simple file type detection by extension
                auto ext = assetPath.substr(assetPath.find_last_of('.') + 1);
                std::string lowerExt = ext;
                std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (lowerExt == "fbx" || lowerExt == "obj" || lowerExt == "glb" || lowerExt == "gltf") {
                    // Model - import as new SceneObject with actual mesh loading
                    std::string name = assetPath.substr(assetPath.find_last_of("/\\") + 1);
                    // Remove extension from name
                    size_t dotPos = name.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        name = name.substr(0, dotPos);
                    }
                    
                    auto newObj = sceneMgr->CreateObject(name, "Mesh");
                    if (newObj) {
                        // Create and load mesh from file
                        auto mesh = std::make_shared<Engine::Core::Mesh>(name + "_mesh");
                        if (mesh->LoadFromFile(assetPath)) {
                            newObj->SetMesh(mesh);
                            Engine::Core::Logger::Instance().Info("AssetBrowser: Successfully imported mesh: " + name);
                        } else {
                            Engine::Core::Logger::Instance().Error("AssetBrowser: Failed to load mesh from: " + assetPath);
                        }
                    }
                } else if (lowerExt == "png" || lowerExt == "jpg" || lowerExt == "jpeg" || lowerExt == "bmp" || lowerExt == "tga") {
                    // Texture - load via ResourceManager
                    std::string name = assetPath.substr(assetPath.find_last_of("/\\") + 1);
                    // Remove extension from name
                    size_t dotPos = name.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        name = name.substr(0, dotPos);
                    }
                    
                    if (g_engine->GetResourceManager()) {
                        if (g_engine->GetResourceManager()->LoadTexture(assetPath, name)) {
                            Engine::Core::Logger::Instance().Info("AssetBrowser: Successfully imported texture: " + name);
                            // TODO: Create material with this texture or assign to selected object
                        } else {
                            Engine::Core::Logger::Instance().Error("AssetBrowser: Failed to load texture from: " + assetPath);
                        }
                    }
                } else if (lowerExt == "wav" || lowerExt == "mp3" || lowerExt == "ogg") {
                    // Audio - add as audio asset, if supported
                    // TODO: Audio system
                    Engine::Core::Logger::Instance().Info("AssetBrowser: Audio import pending - future phase");
                } // else ignore other
                // Optionally, update scene/refresh UI
            });
            
            // Write to file for debugging
            std::ofstream debugFile2("aimoviestudio_debug.txt", std::ios::app);
            debugFile2 << "AssetBrowser::Initialize() completed at " << std::time(nullptr) << std::endl;
            debugFile2.close();
            
            std::cout << "DEBUG: AssetBrowser initialization completed" << std::endl;
            Engine::Core::Logger::Instance().Debug("AssetBrowser initialized with path: " + assetsPath);
        } else {
            std::cout << "DEBUG: m_assetBrowser is null!" << std::endl;
            
            // Write to file for debugging
            std::ofstream debugFile("aimoviestudio_debug.txt", std::ios::app);
            debugFile << "m_assetBrowser is null! at " << std::time(nullptr) << std::endl;
            debugFile.close();
        }
        
        if (m_timelineSystem && m_viewportRenderer) {
            m_timelineSystem->Initialize(m_viewportRenderer.get());
            Engine::Core::Logger::Instance().Debug("TimelineSystem initialized");
        }
        
        if (m_menuSystem) {
            m_menuSystem->Initialize();
            Engine::Core::Logger::Instance().Debug("MenuSystem initialized");
        }
        
        if (m_toolbarSystem) {
            m_toolbarSystem->Initialize();
            Engine::Core::Logger::Instance().Debug("ToolbarSystem initialized");
        }
        
        if (m_aiCommandProcessor && m_viewportRenderer) {
            m_aiCommandProcessor->Initialize(m_viewportRenderer.get());
            Engine::Core::Logger::Instance().Debug("AICommandProcessor initialized");
        }
        
        // Hook: scene hierarchy selection updates properties panel
        if (m_sceneHierarchy && m_propertiesPanel) {
            m_sceneHierarchy->SetSelectionCallback([this](const std::string& name) {
                extern Engine::Core::Engine* g_engine;
                std::cout << ">>> CALLBACK: SceneHierarchy selection fired for: " << name << std::endl;
                Engine::Core::Logger::Instance().Info(">>> CALLBACK: SceneHierarchy selection fired for: " + name);
                
                if (g_engine && g_engine->GetSceneManager()) {
                    auto* sceneMgr = g_engine->GetSceneManager();
                    
                    // First try to find as a regular object
                    auto obj = sceneMgr->FindObjectByName(name);
                    if (obj) {
                        std::cout << ">>> SUCCESS: Found object in SceneManager: " << name << std::endl;
                        Engine::Core::Logger::Instance().Info(">>> SUCCESS: Found object in SceneManager: " + name);
                        
                        // Select the object in SceneManager
                        sceneMgr->SelectObject(name);
                        
                        if (m_propertiesPanel) {
                            m_propertiesPanel->SetSelectedObject(obj);
                            std::cout << ">>> SUCCESS: Updated Properties Panel with selected object" << std::endl;
                            Engine::Core::Logger::Instance().Info(">>> SUCCESS: Updated Properties Panel with selected object");
                        }
                    } else {
                        // Try to find as a Light (Lights are stored separately)
                        auto light = sceneMgr->GetLight(name);
                        if (light) {
                            std::cout << ">>> SUCCESS: Found light in SceneManager: " << name << std::endl;
                            Engine::Core::Logger::Instance().Info(">>> SUCCESS: Found light in SceneManager: " + name);
                            // Lights are not SceneObjects, so we can't set them in PropertiesPanel directly
                            // But we've selected it in hierarchy, which is fine
                        } else {
                            std::cout << ">>> WARNING: Object/Light not found in SceneManager: " << name << std::endl;
                            Engine::Core::Logger::Instance().Warning("Object/Light '" + name + "' not found in SceneManager");
                        }
                    }
                    // Optionally: track last-selected name for Timeline use
                    m_lastSelectedHierarchyName = name;
                } else {
                    std::cout << ">>> ERROR: Engine or SceneManager not available" << std::endl;
                    Engine::Core::Logger::Instance().Error(">>> ERROR: Engine or SceneManager not available");
                    
                    if (m_propertiesPanel) {
                        m_propertiesPanel->SetSelectedObject(nullptr);
                    }
                    m_lastSelectedHierarchyName.clear();
                }
            });
            
            // Hook: scene hierarchy delete uses Command pattern for undo
            m_sceneHierarchy->SetDeleteCallback([this](const std::string& name) {
                std::cout << "========================================" << std::endl;
                std::cout << "DELETE KEY PRESSED: " << name << std::endl;
                std::cout << "========================================" << std::endl;
                Engine::Core::Logger::Instance().Info("========================================");
                Engine::Core::Logger::Instance().Info("DELETE KEY PRESSED: " + name);
                Engine::Core::Logger::Instance().Info("========================================");
                
                extern Engine::Core::Engine* g_engine;
                if (!g_engine || !g_engine->GetCommandHistory() || !g_engine->GetSceneManager()) {
                    std::cout << ">>> ERROR: Engine/CommandHistory/SceneManager not available" << std::endl;
                    Engine::Core::Logger::Instance().Warning("SceneHierarchy: Cannot delete - engine not available");
                    return;
                }
                
                std::cout << ">>> Creating DeleteObjectCommand for: " << name << std::endl;
                // Create delete command
                auto cmd = std::make_unique<Engine::Core::DeleteObjectCommand>(name);
                if (g_engine->GetCommandHistory()->ExecuteCommand(std::move(cmd), g_engine->GetSceneManager())) {
                    std::cout << ">>> SUCCESS: Object deleted via Command: " << name << std::endl;
                    std::cout << ">>> TIP: Press Ctrl+Z to undo delete!" << std::endl;
                    Engine::Core::Logger::Instance().Info(">>> SUCCESS: Deleted object via Command: " + name);
                    
                    // Remove from hierarchy UI
                    if (m_sceneHierarchy) {
                        m_sceneHierarchy->RemoveObject(name);
                    }
                    
                    // Clear properties panel if this was selected
                    if (m_propertiesPanel) {
                        m_propertiesPanel->SetSelectedObject(nullptr);
                    }
                } else {
                    std::cout << ">>> ERROR: Failed to delete object: " << name << std::endl;
                    Engine::Core::Logger::Instance().Error("Failed to delete object: " + name);
                    MessageBoxA(NULL, ("Failed to delete object: " + name).c_str(), "Delete Error", MB_OK | MB_ICONERROR);
                }
            });
        }
        
        if (m_timelineSystem) {
            // Play/Pause/Stop wiring: patch Play/Pause/Stop to log and display a message
            // No SetPlaybackStateChangedCallback on TimelineSystem: Not implemented, skip wiring for now.
            // m_timelineSystem->SetPlaybackStateChangedCallback(...);
            // Time changed (scrub/playhead movement)
            // m_timelineSystem->SetTimeChangedCallback([](float time) {
            //     char buf[64];
            //     sprintf_s(buf, sizeof(buf), "Playhead moved: %.2f sec", time);
            //     Engine::Core::Logger::Instance().Info(buf);
            //     MessageBoxA(NULL, buf, "Timeline", MB_OK | MB_ICONINFORMATION);
            // });
            // Keyframe edited
            // m_timelineSystem->SetKeyframeChangedCallback([](int trackIdx, int kfIdx, float oldVal, float newVal) {
            //     char buf[128];
            //     sprintf_s(buf, sizeof(buf), "Keyframe changed: track %d, keyframe %d, %.2f -> %.2f", trackIdx, kfIdx, oldVal, newVal);
            //     Engine::Core::Logger::Instance().Info(buf);
            //     MessageBoxA(NULL, buf, "Timeline", MB_OK | MB_ICONINFORMATION);
            // });
        }
        
        // Create sub-panels for better organization
        // Left panel sub-panels (only Scene Hierarchy and Asset Browser, AI Command moved to right panel)
        m_leftTopPanel = std::make_shared<UIPanel>("LeftTopPanel", 0, 0, 320, 160);
        m_leftMiddlePanel = std::make_shared<UIPanel>("LeftMiddlePanel", 0, 160, 320, 160);
        m_leftBottomPanel = nullptr; // No longer needed, AI Command moved to right panel
        
        // Set window handles for sub-panels
        m_leftTopPanel->SetWindowHandle(m_uiManager->GetWindowHandle());
        m_leftMiddlePanel->SetWindowHandle(m_uiManager->GetWindowHandle());
        
        // Set sub-panel colors with distinct backgrounds
        m_leftTopPanel->SetBackgroundColor(0.08f, 0.08f, 0.08f, 1.0f);      // Darker for Scene Hierarchy
        m_leftMiddlePanel->SetBackgroundColor(0.10f, 0.10f, 0.10f, 1.0f);   // Medium for Asset Browser
        
        // Set sub-panel borders for visual separation
        m_leftTopPanel->SetBorderWidth(1);
        m_leftMiddlePanel->SetBorderWidth(1);
        
        // Set border colors
        m_leftTopPanel->SetBorderColor(0.3f, 0.3f, 0.3f, 1.0f);
        m_leftMiddlePanel->SetBorderColor(0.3f, 0.3f, 0.3f, 1.0f);
        
        // Add sub-panels to left panel
        m_leftPanel->AddChild(m_leftTopPanel);
        m_leftPanel->AddChild(m_leftMiddlePanel);
        
        // Add components to their respective sub-panels
        if (m_leftTopPanel && m_sceneHierarchy) {
            m_leftTopPanel->AddChild(m_sceneHierarchy);
            Engine::Core::Logger::Instance().Debug("Added SceneHierarchy to LeftTopPanel");
        }
        
        if (m_leftMiddlePanel && m_assetBrowser) {
            m_leftMiddlePanel->AddChild(m_assetBrowser);
            Engine::Core::Logger::Instance().Debug("Added AssetBrowser to LeftMiddlePanel");
        }
        
        if (m_rightPanel && m_aiCommandProcessor) {
            m_rightPanel->AddChild(m_aiCommandProcessor);
            Engine::Core::Logger::Instance().Debug("Added AICommandProcessor to RightPanel");
        }
        
        if (m_centerPanel && m_viewportRenderer) {
            // Add ViewportRenderer as a child component to the center panel
            m_centerPanel->AddChild(m_viewportRenderer);
            Engine::Core::Logger::Instance().Debug("Added ViewportRenderer to CenterPanel");
        }
        
        if (m_bottomPanel && m_propertiesPanel) {
            m_bottomPanel->AddChild(m_propertiesPanel);
            Engine::Core::Logger::Instance().Debug("Added PropertiesPanel to BottomPanel");
        }
        
        if (m_bottomPanel && m_timelineSystem) {
            m_bottomPanel->AddChild(m_timelineSystem);
            Engine::Core::Logger::Instance().Debug("Added TimelineSystem to BottomPanel");
        }
        
        if (m_topPanel && m_menuSystem) {
            m_topPanel->AddChild(m_menuSystem);
            Engine::Core::Logger::Instance().Debug("Added MenuSystem to TopPanel");
        }
        
        if (m_topPanel && m_toolbarSystem) {
            m_topPanel->AddChild(m_toolbarSystem);
            Engine::Core::Logger::Instance().Debug("Added ToolbarSystem to TopPanel");
        }
        
        if (m_timelineSystem) {
            // Timeline time scrub - sync with animation playback
            m_timelineSystem->SetTimeChangedCallback([this](float time) {
                extern Engine::Core::Engine* g_engine;
                if (g_engine && g_engine->GetAnimationManager()) {
                    // Set time on active animation (for scrubbing)
                    // For now, we'll create a test animation clip if one doesn't exist
                    // In production, this would scrub the currently selected clip
                    Engine::Core::Logger::Instance().Debug("Timeline scrubbed to time: " + std::to_string(time));
                }
            });
            
            // Keyframe changed - update animation clip
            m_timelineSystem->SetKeyframeChangedCallback([this](int trackIdx, int kfIdx, [[maybe_unused]] float oldVal, [[maybe_unused]] float newVal) {
                extern Engine::Core::Engine* g_engine;
                if (g_engine && g_engine->GetAnimationManager()) {
                    // Update keyframe in animation clip
                    // TODO: Map timeline tracks to animation clip curves
                    Engine::Core::Logger::Instance().Debug("Keyframe changed: track " + std::to_string(trackIdx) + 
                                                           ", keyframe " + std::to_string(kfIdx));
                }
            });
        }
        
        Engine::Core::Logger::Instance().Debug("UI components created and added to panels successfully");
    }

    void AIMovieStudioUI::UpdatePanelLayouts(int width, int height) {
        if (!m_leftPanel || !m_centerPanel || !m_rightPanel || !m_bottomPanel || !m_topPanel) {
            return;
        }
        
        // Calculate panel sizes
        int centerWidth = width - LEFT_PANEL_WIDTH - RIGHT_PANEL_WIDTH;
        int centerHeight = height - BOTTOM_PANEL_HEIGHT - TOP_PANEL_HEIGHT;
        
        // Ensure centerWidth and centerHeight are positive
        if (centerWidth <= 0) centerWidth = 100;
        if (centerHeight <= 0) centerHeight = 100;
        
        // DEBUG: Log panel layout calculations to file
        std::ofstream debugFile("panel_debug.txt", std::ios::app);
        debugFile << "=== PANEL LAYOUT UPDATE DEBUG ===" << std::endl;
        debugFile << "Window dimensions: " << width << "x" << height << std::endl;
        debugFile << "LEFT_PANEL_WIDTH: " << LEFT_PANEL_WIDTH << std::endl;
        debugFile << "RIGHT_PANEL_WIDTH: " << RIGHT_PANEL_WIDTH << std::endl;
        debugFile << "Calculated centerWidth: " << centerWidth << std::endl;
        debugFile << "Calculated centerHeight: " << centerHeight << std::endl;
        debugFile << "Right panel X position: " << (LEFT_PANEL_WIDTH + centerWidth) << std::endl;
        debugFile << "Total width check: " << LEFT_PANEL_WIDTH << " + " << centerWidth << " + " << RIGHT_PANEL_WIDTH << " = " << (LEFT_PANEL_WIDTH + centerWidth + RIGHT_PANEL_WIDTH) << std::endl;
        debugFile << "=== END PANEL LAYOUT UPDATE DEBUG ===" << std::endl;
        debugFile.close();
        
        Engine::Core::Logger::Instance().Info("Updating panel layouts: window=" + std::to_string(width) + "x" + std::to_string(height) + 
                                               ", center=" + std::to_string(centerWidth) + "x" + std::to_string(centerHeight));
        
        // Update panel positions and sizes
        m_topPanel->SetBounds(0, 0, width, TOP_PANEL_HEIGHT);

        // Ensure MenuSystem and ToolbarSystem fill the top panel
        if (m_menuSystem) {
            m_menuSystem->SetBounds(0, 0, m_topPanel->GetWidth(), m_topPanel->GetHeight());
        }
        if (m_toolbarSystem) {
            m_toolbarSystem->SetBounds(0, 0, m_topPanel->GetWidth(), m_topPanel->GetHeight());
        }
        
        m_leftPanel->SetBounds(0, TOP_PANEL_HEIGHT, LEFT_PANEL_WIDTH, centerHeight);
        m_centerPanel->SetBounds(LEFT_PANEL_WIDTH, TOP_PANEL_HEIGHT, centerWidth, centerHeight);
        m_rightPanel->SetBounds(LEFT_PANEL_WIDTH + centerWidth, TOP_PANEL_HEIGHT, RIGHT_PANEL_WIDTH, centerHeight);
        m_bottomPanel->SetBounds(0, TOP_PANEL_HEIGHT + centerHeight, width, BOTTOM_PANEL_HEIGHT);
        
        // Update left panel sub-panels (only Scene Hierarchy and Asset Browser)
        if (m_leftTopPanel && m_leftMiddlePanel) {
            int leftPanelHeight = centerHeight;
            int padding = 4;
            int topHeight = (leftPanelHeight - padding) / 2;
            int middleHeight = leftPanelHeight - topHeight - padding;

            m_leftTopPanel->SetBounds(0, 0, LEFT_PANEL_WIDTH, topHeight);
            m_leftMiddlePanel->SetBounds(0, topHeight + padding, LEFT_PANEL_WIDTH, middleHeight);

            // Remove inset: children fill parent
            if (m_sceneHierarchy) {
                m_sceneHierarchy->SetBounds(0, 0, m_leftTopPanel->GetWidth(), m_leftTopPanel->GetHeight());
            }
            if (m_assetBrowser) {
                m_assetBrowser->SetBounds(0, 0, m_leftMiddlePanel->GetWidth(), m_leftMiddlePanel->GetHeight());
            }
        }

        // Update bottom panel sub-panels (Timeline and Properties)
        if (m_timelineSystem && m_propertiesPanel) {
            int bottomPanelWidth = width;
            int padding = 4; // Add padding between sub-panels
            int timelineWidth = (bottomPanelWidth - padding) / 2;
            int propertiesWidth = bottomPanelWidth - timelineWidth - padding;

            m_timelineSystem->SetBounds(0, 0, timelineWidth, BOTTOM_PANEL_HEIGHT);
            m_propertiesPanel->SetBounds(timelineWidth + padding, 0, propertiesWidth, BOTTOM_PANEL_HEIGHT);
        }

        // Center and right panel: children fill parent
        if (m_viewportRenderer) {
            m_viewportRenderer->SetBounds(0, 0, m_centerPanel->GetWidth(), m_centerPanel->GetHeight());
        }
        if (m_aiCommandProcessor) {
            m_aiCommandProcessor->SetBounds(0, 0, m_rightPanel->GetWidth(), m_rightPanel->GetHeight());
        }
        
        Engine::Core::Logger::Instance().Debug("Panels updated: LeftPanel(" + std::to_string(0) + "," + std::to_string(TOP_PANEL_HEIGHT) + "," + 
                                                std::to_string(LEFT_PANEL_WIDTH) + "," + std::to_string(centerHeight) + "), " +
                                                "CenterPanel(" + std::to_string(LEFT_PANEL_WIDTH) + "," + std::to_string(TOP_PANEL_HEIGHT) + "," +
                                                std::to_string(centerWidth) + "," + std::to_string(centerHeight) + "), " +
                                                "RightPanel(" + std::to_string(LEFT_PANEL_WIDTH + centerWidth) + "," + std::to_string(TOP_PANEL_HEIGHT) + "," +
                                                std::to_string(RIGHT_PANEL_WIDTH) + "," + std::to_string(centerHeight) + "), " +
                                                "BottomPanel(0," + std::to_string(TOP_PANEL_HEIGHT + centerHeight) + "," +
                                                std::to_string(width) + "," + std::to_string(BOTTOM_PANEL_HEIGHT) + "), " +
                                                "TopPanel(0,0," + std::to_string(width) + "," + std::to_string(TOP_PANEL_HEIGHT) + ")");
        
        // Update viewport renderer if it exists
        if (m_viewportRenderer) {
            m_viewportRenderer->Resize(centerWidth, centerHeight);
        }
    }

} // namespace UI
} // namespace Engine
