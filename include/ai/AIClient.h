#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>

// Link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

namespace Engine {
namespace AI {

    /**
     * @brief Response from AI service
     */
    struct AIResponse {
        bool success;
        std::string message;
        std::vector<std::string> actions;
        std::vector<std::string> suggestions;
    };

    /**
     * @brief AI Client for communicating with Python AI service
     * 
     * Handles TCP/IP communication with the AI service running on Python
     */
    class AIClient {
    public:
        AIClient();
        ~AIClient();

        // Connection management
        bool Connect(const std::string& host, int port);
        bool Disconnect();
        bool IsConnected() const { return m_connected; }

        // Command processing
        AIResponse SendCommand(const std::string& command);
        AIResponse SendCommandWithContext(const std::string& command, const std::string& sceneContext);

        // Scene state synchronization
        bool UpdateSceneState(const std::string& sceneStateJson);

        // Suggestions
        std::vector<std::string> GetSuggestions(const std::string& partialCommand);

        // Templates
        std::string GetTemplates();

        // Error handling
        std::string GetLastError() const { return m_lastError; }

    private:
        SOCKET m_socket;
        bool m_connected;
        std::string m_host;
        int m_port;
        std::string m_lastError;

        // Internal methods
        bool InitializeWinsock();
        void CleanupWinsock();
        bool SendHTTPRequest(const std::string& method, const std::string& path, const std::string& body, std::string& response);
        std::string BuildHTTPRequest(const std::string& method, const std::string& path, const std::string& body);
        bool ReceiveHTTPResponse(std::string& response);
        std::string ParseJSONResponse(const std::string& response);
    };

} // namespace AI
} // namespace Engine

