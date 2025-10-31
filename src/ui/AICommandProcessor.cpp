#include "ui/AICommandProcessor.h"
#include "ui/ViewportRenderer.h"
#include "core/Logger.h"
#include "core/Engine.h"
#include "core/SceneManager.h"
#include "core/SceneObject.h"
#include "core/Light.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <chrono>
#include <windows.h>

namespace Engine {
namespace UI {

    AICommandProcessor::AICommandProcessor()
        : UIComponent("AICommandProcessor", 0, 0, 320, 320)
        , m_viewportRenderer(nullptr)
        , m_aiClient(nullptr)
        , m_aiConnected(false)
        , m_cursorPosition(0)
        , m_selectionStart(0)
        , m_selectionEnd(0)
        , m_historyIndex(-1)
        , m_maxHistorySize(100)
        , m_selectedSuggestion(0)
        , m_showSuggestions(false)
        , m_statusError(false)
        , m_focused(false)
        , m_editing(false)
        , m_scrollOffset(0)
        , m_showAIControls(false)
        , m_showAIAnalytics(false)
    {
        Engine::Core::Logger::Instance().Debug("AICommandProcessor created");
        
        // Initialize AI client
        m_aiClient = std::make_unique<AI::AIClient>();
        
        // Initialize available commands
        m_availableCommands = {
            "add", "remove", "move", "rotate", "scale", "camera", "light", "material",
            "help", "clear", "list", "select", "deselect", "duplicate", "group",
            "ungroup", "hide", "show", "lock", "unlock", "focus", "reset"
        };
    }

    AICommandProcessor::~AICommandProcessor()
    {
        Shutdown();
        Engine::Core::Logger::Instance().Debug("AICommandProcessor destroyed");
    }

    bool AICommandProcessor::Initialize(class ViewportRenderer* viewportRenderer)
    {
        if (!viewportRenderer) {
            Engine::Core::Logger::Instance().Error("ViewportRenderer is null");
            return false;
        }

        m_viewportRenderer = viewportRenderer;
        m_focused = true;
        m_editing = true;
        
        // Connect to AI service
        Engine::Core::Logger::Instance().Info("Connecting to AI service...");
        if (m_aiClient && m_aiClient->Connect("127.0.0.1", 8080)) {
            m_aiConnected = true;
            Engine::Core::Logger::Instance().Info("Connected to AI service successfully");
            AddAIMessage("Connected to AI service successfully!", false);
        } else {
            m_aiConnected = false;
            Engine::Core::Logger::Instance().Warning("Failed to connect to AI service (it may not be running)");
            AddAIMessage("AI service not available. Commands will work locally only.", false);
        }
        
        // Add some sample AI suggestions
        ShowAISuggestions();
        
        // Add welcome messages
        AddAIMessage("Welcome to AI Movie Studio! I'm here to help you create amazing movies.", false);
        AddAIMessage("Try asking me to create a scene, add effects, or animate objects.", false);
        
        Engine::Core::Logger::Instance().Info("AICommandProcessor initialized");
        return true;
    }

    void AICommandProcessor::Shutdown()
    {
        // Disconnect from AI service
        if (m_aiClient && m_aiConnected) {
            m_aiClient->Disconnect();
            m_aiConnected = false;
        }
        
        m_viewportRenderer = nullptr;
        m_commandHistory.clear();
        m_suggestions.clear();
        m_inputText.clear();
        Engine::Core::Logger::Instance().Info("AICommandProcessor shutdown");
    }

    void AICommandProcessor::Update(float deltaTime)
    {
        (void)deltaTime; // Suppress unused parameter warning
        
        // Update status display timeout
        if (!m_status.empty()) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_statusTime);
            if (elapsed.count() > 3000) { // 3 seconds
                m_status.clear();
            }
        }
        
        // Update suggestions timeout
        if (m_showSuggestions) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastSuggestionTime);
            if (elapsed.count() > 10000) { // 10 seconds
                HideSuggestions();
            }
        }
    }

    void AICommandProcessor::Render(HDC hdc)
    {
        if (!m_visible) {
            return;
        }

        // Draw background
        HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
        RECT rect = { m_x, m_y, m_x + m_width, m_y + m_height };
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        // Draw border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
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
        RECT titleRect = { m_x + 5, m_y + 5, m_x + m_width - 5, m_y + 25 };
        DrawText(hdc, L"AI Command Processor", -1, &titleRect, DT_LEFT | DT_VCENTER);

        // Render AI chat history
        RenderAIChat(hdc);
        
        // Render input area
        RenderInputArea(hdc);
        
        // Render suggestions if visible
        if (m_showSuggestions && !m_suggestions.empty()) {
            RenderSuggestions(hdc);
        }
        
        // Render status
        if (!m_status.empty()) {
            RenderStatus(hdc);
        }
    }

    void AICommandProcessor::HandleMouseEvent(int x, int y, int button, bool pressed)
    {
        static int clickCount = 0;
        if (pressed && button == 0) {
            clickCount++;
            std::cout << "[AICommandProcessor] HandleMouseEvent #" << clickCount << ": x=" << x << ", y=" << y 
                      << ", visible=" << m_visible << ", bounds: [" << m_x << "," << (m_x + m_width) 
                      << "] x [" << m_y << "," << (m_y + m_height) << "]" << std::endl;
        }
        
        if (!m_visible || !pressed || button != 0) {
            if (pressed && button == 0 && clickCount <= 5) {
                std::cout << "[AICommandProcessor] Rejected: visible=" << m_visible << ", pressed=" << pressed << ", button=" << button << std::endl;
            }
            return;
        }

        // Check if click is within bounds
        if (x < m_x || x > m_x + m_width || y < m_y || y > m_y + m_height) {
            // Click outside - lose focus
            if (m_focused) {
                std::cout << "[AICommandProcessor] Click outside bounds - losing focus" << std::endl;
                m_focused = false;
                m_editing = false;
            }
            return;
        }

        std::cout << "[AICommandProcessor] ✓ Click INSIDE bounds - checking input area..." << std::endl;

        // Check if click is on input area
        if (y >= m_y + 30 && y <= m_y + 55) {
            std::cout << "[AICommandProcessor] ✓ Click on input area - setting focus and editing" << std::endl;
            m_focused = true;
            m_editing = true;
            UpdateCursorPosition(x, y);
            std::cout << "[AICommandProcessor] Focus set, cursor position updated, inputText='" << m_inputText << "', length=" << m_inputText.length() << std::endl;
        } else if (y >= m_y && y <= m_y + m_height) {
            // Click anywhere in AICommandProcessor panel should focus it
            std::cout << "[AICommandProcessor] ✓ Click on panel (not input area) - setting focus and editing" << std::endl;
            m_focused = true;
            m_editing = true;
        }
        // Check if click is on suggestions
        else if (m_showSuggestions && y >= m_y + 60 && y <= m_y + 60 + static_cast<int>(m_suggestions.size()) * 20) {
            int suggestionIndex = (y - m_y - 60) / 20;
            if (suggestionIndex >= 0 && suggestionIndex < static_cast<int>(m_suggestions.size())) {
                std::cout << "[AICommandProcessor] ✓ Click on suggestion #" << suggestionIndex << std::endl;
                SelectSuggestion(suggestionIndex);
            }
        }
    }

    void AICommandProcessor::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam)
    {
        (void)lParam; // Suppress unused parameter warning
        
        // DEBUG: Log when keyboard events arrive
        static int eventCount = 0;
        eventCount++;
        if (eventCount <= 5 || (eventCount % 20 == 0)) {
            std::cout << "[AICommandProcessor] HandleKeyboardEvent #" << eventCount << " received: msg=0x" << std::hex << message 
                      << ", visible=" << m_visible << ", focused=" << m_focused << ", editing=" << m_editing << std::dec << std::endl;
            Engine::Core::Logger::Instance().Debug("AICommandProcessor::HandleKeyboardEvent: visible=" + std::to_string(m_visible) + 
                                                    ", focused=" + std::to_string(m_focused) + ", editing=" + std::to_string(m_editing));
        }
        
        if (!m_visible || !m_focused || !m_editing) {
            if (!m_visible) {
                static bool loggedVisible = false;
                if (!loggedVisible) {
                    std::cout << "[AICommandProcessor] Rejected: not visible" << std::endl;
                    loggedVisible = true;
                }
            } else if (!m_focused) {
                static bool loggedFocused = false;
                if (!loggedFocused) {
                    std::cout << "[AICommandProcessor] Rejected: not focused (click on AI panel to focus)" << std::endl;
                    loggedFocused = true;
                }
            }
            return;
        }

        if (message == WM_KEYDOWN) {
            std::cout << "[AICommandProcessor] WM_KEYDOWN: key=" << wParam << " (VK_RETURN=" << VK_RETURN 
                      << ", VK_ESCAPE=" << VK_ESCAPE << ", VK_BACK=" << VK_BACK << ")" << std::endl;
            switch (wParam) {
                case VK_RETURN:
                    std::cout << "[AICommandProcessor] ✓ ENTER pressed, inputText='" << m_inputText << "' (length=" << m_inputText.length() << ")" << std::endl;
                    if (!m_inputText.empty()) {
                        std::cout << "[AICommandProcessor] Processing command: '" << m_inputText << "'" << std::endl;
                        ProcessCommand(m_inputText);
                        ClearInput();
                    } else {
                        std::cout << "[AICommandProcessor] Input text is empty, ignoring ENTER" << std::endl;
                    }
                    break;
                    
                case VK_ESCAPE:
                    std::cout << "[AICommandProcessor] ✓ ESCAPE pressed - clearing input and hiding suggestions" << std::endl;
                    ClearInput();
                    HideSuggestions();
                    break;
                    
                case VK_UP:
                    std::cout << "[AICommandProcessor] ✓ UP arrow - navigating history up" << std::endl;
                    NavigateHistoryUp();
                    break;
                    
                case VK_DOWN:
                    std::cout << "[AICommandProcessor] ✓ DOWN arrow - navigating history down" << std::endl;
                    NavigateHistoryDown();
                    break;
                    
                case VK_TAB:
                    std::cout << "[AICommandProcessor] ✓ TAB pressed - handling suggestions" << std::endl;
                    if (m_showSuggestions && !m_suggestions.empty()) {
                        std::cout << "[AICommandProcessor]   Selecting suggestion #" << m_selectedSuggestion << std::endl;
                        SelectSuggestion(m_selectedSuggestion);
                    } else {
                        std::cout << "[AICommandProcessor]   Showing suggestions" << std::endl;
                        ShowSuggestions();
                    }
                    break;
                    
                case VK_BACK:
                    std::cout << "[AICommandProcessor] ✓ BACKSPACE - deleting char at position " << m_cursorPosition 
                              << ", inputText='" << m_inputText << "'" << std::endl;
                    if (m_cursorPosition > 0) {
                        m_inputText.erase(m_cursorPosition - 1, 1);
                        m_cursorPosition--;
                        UpdateSuggestions();
                        std::cout << "[AICommandProcessor]   After delete: inputText='" << m_inputText << "'" << std::endl;
                    }
                    break;
                    
                case VK_DELETE:
                    std::cout << "[AICommandProcessor] ✓ DELETE - deleting char at position " << m_cursorPosition << std::endl;
                    if (m_cursorPosition < static_cast<int>(m_inputText.length())) {
                        m_inputText.erase(m_cursorPosition, 1);
                        UpdateSuggestions();
                    }
                    break;
                    
                case VK_LEFT:
                    std::cout << "[AICommandProcessor] ✓ LEFT arrow - moving cursor left" << std::endl;
                    MoveCursor(-1, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
                    break;
                    
                case VK_RIGHT:
                    std::cout << "[AICommandProcessor] ✓ RIGHT arrow - moving cursor right" << std::endl;
                    MoveCursor(1, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
                    break;
                    
                case VK_HOME:
                    m_cursorPosition = 0;
                    break;
                    
                case VK_END:
                    m_cursorPosition = static_cast<int>(m_inputText.length());
                    break;
                    
                case 0x41: // VK_A
                    if (GetKeyState(VK_CONTROL) & 0x8000) {
                        m_selectionStart = 0;
                        m_selectionEnd = static_cast<int>(m_inputText.length());
                    }
                    break;
                    
                case 0x43: // VK_C
                    if (GetKeyState(VK_CONTROL) & 0x8000) {
                        // Copy selection to clipboard
                        if (m_selectionStart != m_selectionEnd) {
                            std::string selection = m_inputText.substr(m_selectionStart, m_selectionEnd - m_selectionStart);
                            // TODO: Implement clipboard copy
                        }
                    }
                    break;
                    
                case 0x56: // VK_V
                    if (GetKeyState(VK_CONTROL) & 0x8000) {
                        // Paste from clipboard
                        // TODO: Implement clipboard paste
                    }
                    break;
            }
        }
        else if (message == WM_CHAR) {
            char ch = static_cast<char>(wParam);
            std::cout << "[AICommandProcessor] WM_CHAR: char='" << ch << "' (code=" << static_cast<int>(ch) 
                      << "), inputText before='" << m_inputText << "' (length=" << m_inputText.length() 
                      << "), cursorPos=" << m_cursorPosition << std::endl;
            if (ch >= 32 && ch <= 126) { // Printable characters
                std::cout << "[AICommandProcessor] ✓ Inserting character '" << ch << "' at position " << m_cursorPosition << std::endl;
                InsertText(std::string(1, ch));
                std::cout << "[AICommandProcessor] After insert: inputText='" << m_inputText << "' (length=" << m_inputText.length() << ")" << std::endl;
                UpdateSuggestions();
                std::cout << "[AICommandProcessor] Suggestions updated, count=" << m_suggestions.size() << std::endl;
            } else {
                std::cout << "[AICommandProcessor] ✗ Character '" << ch << "' (code=" << static_cast<int>(ch) << ") not printable, ignoring" << std::endl;
            }
        }
    }

    void AICommandProcessor::ProcessCommand(const std::string& command)
    {
        if (command.empty()) {
            return;
        }

        AddToHistory(command);
        AddAIMessage(command, true); // Add user message to chat
        SetStatus("Processing: " + command);
        
        // Send command to AI service if connected
        if (m_aiConnected && m_aiClient) {
            Engine::Core::Logger::Instance().Info("Sending command to AI service: " + command);
            
            auto response = m_aiClient->SendCommand(command);
            
            if (response.success) {
                AddAIMessage(response.message, false); // Add AI response to chat
                SetStatus(response.message);
                
                // Execute actions returned by AI (simple verb-based executor)
                for (const auto& action : response.actions) {
                    Engine::Core::Logger::Instance().Info("Executing AI action: " + action);

                    extern Engine::Core::Engine* g_engine;
                    if (!g_engine) {
                        continue;
                    }
                    auto* sceneMgr = g_engine->GetSceneManager();
                    if (!sceneMgr) {
                        continue;
                    }

                    // Very simple action parsing: "create_object:type=x,pos=a,b,c" style
                    // If backend sends plain verbs like "create_object", we default to cube at origin
                    auto toLower = [](std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); }); return s; };
                    std::string act = toLower(action);

                    auto parseVec3 = [](const std::string& s) -> DirectX::XMFLOAT3 {
                        DirectX::XMFLOAT3 v{0,0,0};
                        float x=0.f,y=0.f,z=0.f; char c=0;
                        std::stringstream ss(s);
                        if ((ss >> x >> c >> y >> c >> z)) {
                            v = {x,y,z};
                        }
                        return v;
                    };

                    if (act.rfind("create_object", 0) == 0) {
                        // Determine type
                        std::string type = "Mesh";
                        DirectX::XMFLOAT3 pos{0,0,0};

                        auto typePos = act.find("type=");
                        if (typePos != std::string::npos) {
                            auto end = act.find_first_of(", ", typePos);
                            type = act.substr(typePos + 5, end == std::string::npos ? std::string::npos : end - (typePos + 5));
                        }
                        auto posPos = act.find("pos=");
                        if (posPos != std::string::npos) {
                            auto end = act.find_first_of(" ", posPos);
                            auto vec = act.substr(posPos + 4, end == std::string::npos ? std::string::npos : end - (posPos + 4));
                            pos = parseVec3(vec);
                        }

                        std::string name = "AI_" + type + "_" + std::to_string(static_cast<unsigned>(::GetTickCount64() & 0xffffffff));
                        auto obj = sceneMgr->CreateObject(name, type);
                        if (obj) {
                            obj->SetPosition(pos);
                            AddAIMessage("Created object '" + name + "'", false);
                        }
                    } else if (act.rfind("create_light", 0) == 0) {
                        std::string lightType = "Directional";
                        DirectX::XMFLOAT3 pos{2,2,2};

                        auto typePos = act.find("type=");
                        if (typePos != std::string::npos) {
                            auto end = act.find_first_of(", ", typePos);
                            lightType = act.substr(typePos + 5, end == std::string::npos ? std::string::npos : end - (typePos + 5));
                        }
                        auto posPos = act.find("pos=");
                        if (posPos != std::string::npos) {
                            auto end = act.find_first_of(" ", posPos);
                            auto vec = act.substr(posPos + 4, end == std::string::npos ? std::string::npos : end - (posPos + 4));
                            pos = parseVec3(vec);
                        }

                        std::string name = "AI_" + lightType + "_Light";
                        auto light = sceneMgr->CreateLight(name, lightType);
                        if (light) {
                            light->SetPosition(pos);
                            AddAIMessage("Created light '" + name + "'", false);
                        }
                    } else if (act.rfind("create_camera", 0) == 0) {
                        // For now we create an object named Camera; real camera system can be added
                        DirectX::XMFLOAT3 pos{0,2,5};
                        auto posPos = act.find("pos=");
                        if (posPos != std::string::npos) {
                            auto end = act.find_first_of(" ", posPos);
                            auto vec = act.substr(posPos + 4, end == std::string::npos ? std::string::npos : end - (posPos + 4));
                            pos = parseVec3(vec);
                        }
                        auto obj = sceneMgr->CreateObject("AI_Camera", "Camera");
                        if (obj) {
                            obj->SetPosition(pos);
                            AddAIMessage("Created camera placeholder at position", false);
                        }
                    } else if (act.rfind("create_scene", 0) == 0 || act.find("sunset") != std::string::npos) {
                        // Special handling for sunset scene creation
                        AddAIMessage("Creating sunset scene...", false);
                        
                        // Create a ground plane (large flat cube)
                        auto ground = sceneMgr->CreateObject("Ground_Plane", "Mesh");
                        if (ground) {
                            ground->SetPosition({0.0f, -0.5f, 0.0f});
                            ground->SetScale({10.0f, 0.1f, 10.0f});
                        }
                        
                        // Create sun (warm directional light)
                        auto sun = sceneMgr->CreateLight("Sunset_Sun", "Directional");
                        if (sun) {
                            sun->SetPosition({10.0f, 5.0f, 10.0f});
                            sun->SetColor({1.0f, 0.6f, 0.3f}); // Orange sunset color
                            sun->SetIntensity(1.2f);
                        }
                        
                        // Create ambient light (purple/pink ambient for sunset)
                        auto ambient = sceneMgr->CreateLight("Sunset_Ambient", "Ambient");
                        if (ambient) {
                            ambient->SetColor({0.6f, 0.4f, 0.7f}); // Purple/pink ambient
                            ambient->SetIntensity(0.5f);
                        }
                        
                        // Create some silhouette objects
                        auto tree1 = sceneMgr->CreateObject("Tree_Silhouette_1", "Mesh");
                        if (tree1) {
                            tree1->SetPosition({-3.0f, 1.0f, 2.0f});
                            tree1->SetScale({0.5f, 2.0f, 0.5f});
                        }
                        
                        auto tree2 = sceneMgr->CreateObject("Tree_Silhouette_2", "Mesh");
                        if (tree2) {
                            tree2->SetPosition({4.0f, 1.5f, -3.0f});
                            tree2->SetScale({0.6f, 3.0f, 0.6f});
                        }
                        
                        // Update viewport lighting
                        if (m_viewportRenderer) {
                            m_viewportRenderer->SetLightPosition({10.0f, 5.0f, 10.0f});
                            m_viewportRenderer->SetLightColor({1.0f, 0.6f, 0.3f, 1.0f});
                            m_viewportRenderer->SetAmbientColor({0.6f, 0.4f, 0.7f, 1.0f});
                        }
                        
                        AddAIMessage("✓ Sunset scene created with warm lighting and silhouettes!", false);
                    }
                }
                
                // Show AI suggestions
                if (!response.suggestions.empty()) {
                    m_aiSuggestions = response.suggestions;
                }
            } else {
                AddAIMessage("AI service error: " + response.message, false);
                SetStatus("AI service error", true);
            }
        } else {
            // Fallback to local command parsing if AI not connected
            std::istringstream iss(command);
            std::vector<std::string> args;
            std::string arg;
            
            while (iss >> arg) {
                args.push_back(arg);
            }
            
            if (args.empty()) {
                return;
            }
            
            // Execute command locally
            std::string cmd = args[0];
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            
            try {
                ExecuteCommand(cmd, args);
            }
            catch (const std::exception& e) {
                SetStatus("Error: " + std::string(e.what()), true);
                AddAIMessage("Error: " + std::string(e.what()), false);
            }
        }
    }

    void AICommandProcessor::AddToHistory(const std::string& command)
    {
        if (command.empty()) {
            return;
        }
        
        // Don't add duplicate consecutive commands
        if (!m_commandHistory.empty() && m_commandHistory.back() == command) {
            return;
        }
        
        m_commandHistory.push_back(command);
        if (m_commandHistory.size() > static_cast<size_t>(m_maxHistorySize)) {
            m_commandHistory.erase(m_commandHistory.begin());
        }
        
        m_historyIndex = static_cast<int>(m_commandHistory.size());
    }

    void AICommandProcessor::ClearHistory()
    {
        m_commandHistory.clear();
        m_historyIndex = -1;
    }

    void AICommandProcessor::SetInputText(const std::string& text)
    {
        m_inputText = text;
        m_cursorPosition = static_cast<int>(text.length());
        m_selectionStart = 0;
        m_selectionEnd = 0;
        UpdateSuggestions();
    }

    void AICommandProcessor::ClearInput()
    {
        m_inputText.clear();
        m_cursorPosition = 0;
        m_selectionStart = 0;
        m_selectionEnd = 0;
        HideSuggestions();
    }

    void AICommandProcessor::NavigateHistoryUp()
    {
        if (m_historyIndex > 0) {
            m_historyIndex--;
            SetInputText(m_commandHistory[m_historyIndex]);
        }
    }

    void AICommandProcessor::NavigateHistoryDown()
    {
        if (m_historyIndex < static_cast<int>(m_commandHistory.size()) - 1) {
            m_historyIndex++;
            SetInputText(m_commandHistory[m_historyIndex]);
        } else {
            ClearInput();
        }
    }

    void AICommandProcessor::ShowSuggestions()
    {
        UpdateSuggestions();
        m_showSuggestions = true;
        m_selectedSuggestion = 0;
        m_lastSuggestionTime = std::chrono::steady_clock::now();
    }

    void AICommandProcessor::HideSuggestions()
    {
        m_showSuggestions = false;
        m_suggestions.clear();
    }

    void AICommandProcessor::SelectSuggestion(int index)
    {
        if (index >= 0 && index < static_cast<int>(m_suggestions.size())) {
            m_inputText = m_suggestions[index];
            m_cursorPosition = static_cast<int>(m_inputText.length());
            HideSuggestions();
        }
    }

    void AICommandProcessor::SetStatus(const std::string& status, bool isError)
    {
        m_status = status;
        m_statusError = isError;
        m_statusTime = std::chrono::steady_clock::now();
    }

    void AICommandProcessor::ClearStatus()
    {
        m_status.clear();
        m_statusError = false;
    }

    void AICommandProcessor::RenderInputArea(HDC hdc)
    {
        // Draw input background
        HBRUSH hBrush = CreateSolidBrush(RGB(35, 35, 35));
        RECT inputRect = { m_x + 5, m_y + 30, m_x + m_width - 5, m_y + 55 };
        FillRect(hdc, &inputRect, hBrush);
        DeleteObject(hBrush);

        // Draw input border
        HPEN hPen = CreatePen(PS_SOLID, 1, m_focused ? RGB(100, 150, 255) : RGB(60, 60, 60));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, inputRect.left, inputRect.top, NULL);
        LineTo(hdc, inputRect.right, inputRect.top);
        LineTo(hdc, inputRect.right, inputRect.bottom);
        LineTo(hdc, inputRect.left, inputRect.bottom);
        LineTo(hdc, inputRect.left, inputRect.top);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        // Draw input text
        ::SetTextColor(hdc, RGB(255, 255, 255));
        RECT textRect = { m_x + 8, m_y + 32, m_x + m_width - 8, m_y + 53 };
        
        if (!m_inputText.empty()) {
            std::wstring wideText(m_inputText.begin(), m_inputText.end());
            DrawText(hdc, wideText.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
        }

        // Draw cursor if focused
        if (m_focused) {
            RenderCursor(hdc);
        }
    }

    void AICommandProcessor::RenderSuggestions(HDC hdc)
    {
        if (m_suggestions.empty()) {
            return;
        }

        int suggestionY = m_y + 60;
        int maxSuggestions = std::min(static_cast<int>(m_suggestions.size()), 5);
        
        for (int i = 0; i < maxSuggestions; ++i) {
            RECT suggestionRect = { m_x + 5, suggestionY + i * 20, m_x + m_width - 5, suggestionY + (i + 1) * 20 };
            
            // Highlight selected suggestion
            if (i == m_selectedSuggestion) {
                HBRUSH hBrush = CreateSolidBrush(RGB(60, 100, 150));
                FillRect(hdc, &suggestionRect, hBrush);
                DeleteObject(hBrush);
            }
            
            // Draw suggestion text
            ::SetTextColor(hdc, RGB(200, 200, 200));
            std::wstring wideText(m_suggestions[i].begin(), m_suggestions[i].end());
            DrawText(hdc, wideText.c_str(), -1, &suggestionRect, DT_LEFT | DT_VCENTER);
        }
    }

    void AICommandProcessor::RenderStatus(HDC hdc)
    {
        RECT statusRect = { m_x + 5, m_y + m_height - 25, m_x + m_width - 5, m_y + m_height - 5 };
        
        // Draw status background
        HBRUSH hBrush = CreateSolidBrush(m_statusError ? RGB(80, 40, 40) : RGB(40, 80, 40));
        FillRect(hdc, &statusRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw status text
        ::SetTextColor(hdc, m_statusError ? RGB(255, 150, 150) : RGB(150, 255, 150));
        std::wstring wideStatus(m_status.begin(), m_status.end());
        DrawText(hdc, wideStatus.c_str(), -1, &statusRect, DT_LEFT | DT_VCENTER);
    }

    void AICommandProcessor::RenderAIChat(HDC hdc)
    {
        int chatY = m_y + 30;
        int maxHeight = m_height - 80; // Leave space for input area and status
        int currentY = chatY;
        
        // Draw chat background
        HBRUSH hBrush = CreateSolidBrush(RGB(40, 40, 40));
        RECT chatRect = { m_x + 5, chatY, m_x + m_width - 5, chatY + maxHeight };
        FillRect(hdc, &chatRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw chat messages
        for (const auto& message : m_aiChatHistory) {
            if (currentY > chatY + maxHeight - 20) break; // Don't render if outside visible area
            
            // Set text color based on sender
            if (message.second) { // User message
                ::SetTextColor(hdc, RGB(100, 150, 255));
            } else { // AI message
                ::SetTextColor(hdc, RGB(255, 255, 255));
            }
            
            SetBkMode(hdc, TRANSPARENT);
            RECT messageRect = { m_x + 10, currentY, m_x + m_width - 10, currentY + 20 };
            std::wstring wideMessage(message.first.begin(), message.first.end());
            DrawText(hdc, wideMessage.c_str(), -1, &messageRect, DT_LEFT | DT_VCENTER | DT_WORDBREAK);
            
            currentY += 25;
        }
        
        // Draw AI suggestions
        if (!m_aiSuggestions.empty()) {
            currentY += 10;
            ::SetTextColor(hdc, RGB(255, 255, 0));
            RECT suggestionsTitleRect = { m_x + 10, currentY, m_x + m_width - 10, currentY + 20 };
            DrawText(hdc, L"AI Suggestions:", -1, &suggestionsTitleRect, DT_LEFT | DT_VCENTER);
            currentY += 25;
            
            for (const auto& suggestion : m_aiSuggestions) {
                if (currentY > chatY + maxHeight - 20) break;
                
                ::SetTextColor(hdc, RGB(200, 200, 200));
                RECT suggestionRect = { m_x + 20, currentY, m_x + m_width - 20, currentY + 20 };
                std::wstring wideSuggestion(suggestion.begin(), suggestion.end());
                DrawText(hdc, wideSuggestion.c_str(), -1, &suggestionRect, DT_LEFT | DT_VCENTER | DT_WORDBREAK);
                currentY += 20;
            }
        }
    }

    void AICommandProcessor::RenderCursor(HDC hdc)
    {
        // Calculate cursor position
        RECT textRect = { m_x + 8, m_y + 32, m_x + m_width - 8, m_y + 53 };
        
        // Simple cursor positioning (in real implementation, would measure text width)
        int cursorX = m_x + 8 + m_cursorPosition * 8; // Approximate character width
        int cursorY = m_y + 32;
        
        // Draw cursor line
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, cursorX, cursorY, NULL);
        LineTo(hdc, cursorX, cursorY + 20);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }

    void AICommandProcessor::UpdateCursorPosition(int x, int y)
    {
        (void)y; // Suppress unused parameter warning
        
        // Simple cursor positioning
        int relativeX = x - m_x - 8;
        m_cursorPosition = std::max(0, std::min(static_cast<int>(m_inputText.length()), relativeX / 8));
    }

    void AICommandProcessor::UpdateSelection(int x, int y)
    {
        (void)y; // Suppress unused parameter warning
        
        int relativeX = x - m_x - 8;
        int newPos = std::max(0, std::min(static_cast<int>(m_inputText.length()), relativeX / 8));
        
        if (m_selectionStart == m_selectionEnd) {
            m_selectionStart = m_cursorPosition;
        }
        m_selectionEnd = newPos;
    }

    void AICommandProcessor::InsertText(const std::string& text)
    {
        if (m_selectionStart != m_selectionEnd) {
            DeleteSelection();
        }
        
        m_inputText.insert(m_cursorPosition, text);
        m_cursorPosition += static_cast<int>(text.length());
        m_selectionStart = m_cursorPosition;
        m_selectionEnd = m_cursorPosition;
    }

    void AICommandProcessor::DeleteSelection()
    {
        if (m_selectionStart != m_selectionEnd) {
            int start = std::min(m_selectionStart, m_selectionEnd);
            int end = std::max(m_selectionStart, m_selectionEnd);
            m_inputText.erase(start, end - start);
            m_cursorPosition = start;
        }
        m_selectionStart = m_cursorPosition;
        m_selectionEnd = m_cursorPosition;
    }

    void AICommandProcessor::MoveCursor(int direction, bool select)
    {
        int newPos = m_cursorPosition + direction;
        newPos = std::max(0, std::min(static_cast<int>(m_inputText.length()), newPos));
        
        if (select) {
            if (m_selectionStart == m_selectionEnd) {
                m_selectionStart = m_cursorPosition;
            }
            m_selectionEnd = newPos;
        } else {
            m_selectionStart = newPos;
            m_selectionEnd = newPos;
        }
        
        m_cursorPosition = newPos;
    }

    void AICommandProcessor::ParseCommand(const std::string& command)
    {
        (void)command; // Suppress unused parameter warning
        // Command parsing is handled in ProcessCommand
    }

    void AICommandProcessor::ExecuteCommand(const std::string& command, const std::vector<std::string>& args)
    {
        if (command == "add") {
            ExecuteAddObject(args);
        } else if (command == "remove") {
            ExecuteRemoveObject(args);
        } else if (command == "move") {
            ExecuteMoveObject(args);
        } else if (command == "rotate") {
            ExecuteRotateObject(args);
        } else if (command == "scale") {
            ExecuteScaleObject(args);
        } else if (command == "camera") {
            ExecuteSetCamera(args);
        } else if (command == "light") {
            ExecuteAddLight(args);
        } else if (command == "material") {
            ExecuteSetMaterial(args);
        } else if (command == "help") {
            ExecuteHelp(args);
        } else if (command == "clear") {
            ExecuteClear(args);
        } else if (command == "list") {
            ExecuteList(args);
        } else {
            SetStatus("Unknown command: " + command, true);
        }
    }

    void AICommandProcessor::UpdateSuggestions()
    {
        if (m_inputText.empty()) {
            HideSuggestions();
            return;
        }
        
        FilterSuggestions(m_inputText);
        if (!m_suggestions.empty()) {
            m_showSuggestions = true;
            m_selectedSuggestion = 0;
            m_lastSuggestionTime = std::chrono::steady_clock::now();
        }
    }

    void AICommandProcessor::FilterSuggestions(const std::string& prefix)
    {
        m_suggestions.clear();
        
        for (const auto& cmd : m_availableCommands) {
            if (cmd.find(prefix) == 0) {
                m_suggestions.push_back(cmd);
            }
        }
    }

    // Command implementations
    void AICommandProcessor::ExecuteAddObject(const std::vector<std::string>& args)
    {
        if (args.size() < 2) {
            SetStatus("Usage: add <type> [name]", true);
            return;
        }
        
        std::string type = args[1];
        std::string name = args.size() > 2 ? args[2] : type + "_" + std::to_string(rand() % 1000);
        
        // TODO: Actually add object to scene
        SetStatus("Added " + type + " as " + name);
    }

    void AICommandProcessor::ExecuteRemoveObject(const std::vector<std::string>& args)
    {
        if (args.size() < 2) {
            SetStatus("Usage: remove <name>", true);
            return;
        }
        
        std::string name = args[1];
        // TODO: Actually remove object from scene
        SetStatus("Removed " + name);
    }

    void AICommandProcessor::ExecuteMoveObject(const std::vector<std::string>& args)
    {
        if (args.size() < 5) {
            SetStatus("Usage: move <name> <x> <y> <z>", true);
            return;
        }
        
        std::string name = args[1];
        // TODO: Actually move object
        SetStatus("Moved " + name);
    }

    void AICommandProcessor::ExecuteRotateObject(const std::vector<std::string>& args)
    {
        if (args.size() < 5) {
            SetStatus("Usage: rotate <name> <x> <y> <z>", true);
            return;
        }
        
        std::string name = args[1];
        // TODO: Actually rotate object
        SetStatus("Rotated " + name);
    }

    void AICommandProcessor::ExecuteScaleObject(const std::vector<std::string>& args)
    {
        if (args.size() < 5) {
            SetStatus("Usage: scale <name> <x> <y> <z>", true);
            return;
        }
        
        std::string name = args[1];
        // TODO: Actually scale object
        SetStatus("Scaled " + name);
    }

    void AICommandProcessor::ExecuteSetCamera(const std::vector<std::string>& args)
    {
        if (args.size() < 2) {
            SetStatus("Usage: camera <position|target|fov> <values>", true);
            return;
        }
        
        std::string param = args[1];
        // TODO: Actually set camera parameters
        SetStatus("Set camera " + param);
    }

    void AICommandProcessor::ExecuteAddLight(const std::vector<std::string>& args)
    {
        if (args.size() < 2) {
            SetStatus("Usage: light <type> [name]", true);
            return;
        }
        
        std::string type = args[1];
        std::string name = args.size() > 2 ? args[2] : type + "_light_" + std::to_string(rand() % 1000);
        
        // TODO: Actually add light to scene
        SetStatus("Added " + type + " light as " + name);
    }

    void AICommandProcessor::ExecuteSetMaterial(const std::vector<std::string>& args)
    {
        if (args.size() < 3) {
            SetStatus("Usage: material <object> <material>", true);
            return;
        }
        
        std::string object = args[1];
        std::string material = args[2];
        
        // TODO: Actually set material
        SetStatus("Set " + object + " material to " + material);
    }

    void AICommandProcessor::ExecuteHelp(const std::vector<std::string>& args)
    {
        (void)args; // Suppress unused parameter warning
        
        std::string helpText = "Available commands: add, remove, move, rotate, scale, camera, light, material, help, clear, list";
        SetStatus(helpText);
    }

    void AICommandProcessor::ExecuteClear(const std::vector<std::string>& args)
    {
        (void)args; // Suppress unused parameter warning
        
        // TODO: Actually clear scene
        SetStatus("Cleared scene");
    }

    void AICommandProcessor::ExecuteList(const std::vector<std::string>& args)
    {
        (void)args; // Suppress unused parameter warning
        
        // TODO: Actually list scene objects
        SetStatus("Scene objects: (list would appear here)");
    }

    // AI Assistant methods
    void AICommandProcessor::AddAIMessage(const std::string& message, bool isUser)
    {
        m_aiChatHistory.push_back({message, isUser});
        
        // Keep only last 50 messages
        if (m_aiChatHistory.size() > 50) {
            m_aiChatHistory.erase(m_aiChatHistory.begin());
        }
        
        Engine::Core::Logger::Instance().Debug("Added AI message: " + message);
    }

    void AICommandProcessor::ShowAISuggestions()
    {
        m_aiSuggestions = {
            "Create a sunset scene",
            "Add a car chase",
            "Generate explosion effects",
            "Create a forest environment",
            "Add character animation",
            "Generate particle effects"
        };
        
        Engine::Core::Logger::Instance().Debug("Showing AI suggestions");
    }

    void AICommandProcessor::ShowAIControls()
    {
        m_showAIControls = true;
        Engine::Core::Logger::Instance().Debug("Showing AI controls");
    }

    void AICommandProcessor::ShowAIAnalytics()
    {
        m_showAIAnalytics = true;
        Engine::Core::Logger::Instance().Debug("Showing AI analytics");
    }

    void AICommandProcessor::ClearAIChat()
    {
        m_aiChatHistory.clear();
        Engine::Core::Logger::Instance().Debug("Cleared AI chat");
    }

} // namespace UI
} // namespace Engine
