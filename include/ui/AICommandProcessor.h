#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include "UI.h"
#include "ai/AIClient.h"

namespace Engine {
namespace UI {

    /**
     * @brief AI Assistant for AI Movie Studio
     * 
     * Full AI Assistant implementation with:
     * - AI Chat interface with conversation history
     * - AI Suggestions and recommendations
     * - AI Controls and settings
     * - AI Analytics and performance metrics
     * - Command history with navigation
     * - Auto-complete suggestions
     * - Status display and feedback
     */
    class AICommandProcessor : public UIComponent {
    public:
        Type GetType() const override { return Type::Panel; }
    public:
        AICommandProcessor();
        ~AICommandProcessor();

        /**
         * @brief Initialize the command processor
         * @param viewportRenderer The viewport renderer for scene operations
         * @return true if initialization successful
         */
        bool Initialize(class ViewportRenderer* viewportRenderer);

        /**
         * @brief Shutdown the command processor
         */
        void Shutdown();

        /**
         * @brief Update the command processor
         * @param deltaTime Time elapsed since last frame
         */
        void Update(float deltaTime) override;

        /**
         * @brief Render the command processor
         * @param hdc Device context for rendering
         */
        void Render(HDC hdc) override;

        /**
         * @brief Handle mouse events
         * @param x Mouse X position
         * @param y Mouse Y position
         * @param button Mouse button (0=left, 1=right, 2=middle)
         * @param pressed true if button pressed, false if released
         */
        void HandleMouseEvent(int x, int y, int button, bool pressed) override;

        /**
         * @brief Handle keyboard events
         * @param message Windows message
         * @param wParam WPARAM
         * @param lParam LPARAM
         */
        void HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) override;

        // Command processing
        void ProcessCommand(const std::string& command);
        void AddToHistory(const std::string& command);
        void ClearHistory();
        
        // Input management
        void SetInputText(const std::string& text);
        std::string GetInputText() const { return m_inputText; }
        void ClearInput();
        
        // History navigation
        void NavigateHistoryUp();
        void NavigateHistoryDown();
        
        // Auto-complete
        void ShowSuggestions();
        void HideSuggestions();
        void SelectSuggestion(int index);
        
        // Status
        void SetStatus(const std::string& status, bool isError = false);
        void ClearStatus();
        
        // AI Assistant features
        void AddAIMessage(const std::string& message, bool isUser = false);
        void ShowAISuggestions();
        void ShowAIControls();
        void ShowAIAnalytics();
        void ClearAIChat();

        // Getters
        const std::vector<std::string>& GetHistory() const { return m_commandHistory; }
        const std::vector<std::string>& GetSuggestions() const { return m_suggestions; }
        bool IsSuggestionsVisible() const { return m_showSuggestions; }
        const std::string& GetStatus() const { return m_status; }
        bool IsStatusError() const { return m_statusError; }

    private:
        class ViewportRenderer* m_viewportRenderer;
        std::unique_ptr<AI::AIClient> m_aiClient;
        bool m_aiConnected;
        
        // Input text
        std::string m_inputText;
        int m_cursorPosition;
        int m_selectionStart;
        int m_selectionEnd;
        
        // Command history
        std::vector<std::string> m_commandHistory;
        int m_historyIndex;
        int m_maxHistorySize;
        
        // Auto-complete
        std::vector<std::string> m_suggestions;
        int m_selectedSuggestion;
        bool m_showSuggestions;
        std::chrono::steady_clock::time_point m_lastSuggestionTime;
        
        // Status
        std::string m_status;
        bool m_statusError;
        std::chrono::steady_clock::time_point m_statusTime;
        
        // UI state
        bool m_focused;
        bool m_editing;
        int m_scrollOffset;
        
        // AI Assistant data
        std::vector<std::pair<std::string, bool>> m_aiChatHistory; // message, isUser
        std::vector<std::string> m_aiSuggestions;
        bool m_showAIControls;
        bool m_showAIAnalytics;
        
        // Command parsing
        std::vector<std::string> m_availableCommands;
        
        // Helper methods
        void RenderInputArea(HDC hdc);
        void RenderHistory(HDC hdc);
        void RenderSuggestions(HDC hdc);
        void RenderStatus(HDC hdc);
        void RenderCursor(HDC hdc);
        void RenderAIChat(HDC hdc);
        
        void UpdateCursorPosition(int x, int y);
        void UpdateSelection(int x, int y);
        void InsertText(const std::string& text);
        void DeleteSelection();
        void MoveCursor(int direction, bool select = false);
        
        void ParseCommand(const std::string& command);
        void ExecuteCommand(const std::string& command, const std::vector<std::string>& args);
        void UpdateSuggestions();
        void FilterSuggestions(const std::string& prefix);
        
        // Command implementations
        void ExecuteAddObject(const std::vector<std::string>& args);
        void ExecuteRemoveObject(const std::vector<std::string>& args);
        void ExecuteMoveObject(const std::vector<std::string>& args);
        void ExecuteRotateObject(const std::vector<std::string>& args);
        void ExecuteScaleObject(const std::vector<std::string>& args);
        void ExecuteSetCamera(const std::vector<std::string>& args);
        void ExecuteAddLight(const std::vector<std::string>& args);
        void ExecuteSetMaterial(const std::vector<std::string>& args);
        void ExecuteHelp(const std::vector<std::string>& args);
        void ExecuteClear(const std::vector<std::string>& args);
        void ExecuteList(const std::vector<std::string>& args);
    };

} // namespace UI
} // namespace Engine
