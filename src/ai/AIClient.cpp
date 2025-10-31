#include "ai/AIClient.h"
#include "core/Logger.h"
#include <iostream>
#include <sstream>

namespace Engine {
namespace AI {

    static std::string EscapeJSONString(const std::string& s)
    {
        std::string out;
        out.reserve(s.size() + 8);
        for (char c : s) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char buf[7];
                        sprintf_s(buf, "\\u%04x", static_cast<unsigned char>(c));
                        out += buf;
                    } else {
                        out.push_back(c);
                    }
            }
        }
        return out;
    }

    AIClient::AIClient()
        : m_socket(INVALID_SOCKET)
        , m_connected(false)
        , m_host("127.0.0.1")
        , m_port(8080)
    {
        InitializeWinsock();
        Core::Logger::Instance().Info("AIClient created");
    }

    AIClient::~AIClient()
    {
        Disconnect();
        CleanupWinsock();
        Core::Logger::Instance().Info("AIClient destroyed");
    }

    bool AIClient::InitializeWinsock()
    {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            m_lastError = "WSAStartup failed: " + std::to_string(result);
            Core::Logger::Instance().Error(m_lastError);
            return false;
        }
        return true;
    }

    void AIClient::CleanupWinsock()
    {
        WSACleanup();
    }

    bool AIClient::Connect(const std::string& host, int port)
    {
        m_host = host;
        m_port = port;

        Core::Logger::Instance().Info("Connecting to AI service at " + host + ":" + std::to_string(port));

        // Create socket
        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET) {
            m_lastError = "Failed to create socket: " + std::to_string(WSAGetLastError());
            Core::Logger::Instance().Error(m_lastError);
            return false;
        }

        // Setup server address
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(static_cast<u_short>(port));
        
        // Convert IP address
        int result = inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
        if (result <= 0) {
            m_lastError = "Invalid address: " + host;
            Core::Logger::Instance().Error(m_lastError);
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }

        // Connect (with simple retry)
        int attempts = 0;
        const int maxAttempts = 3;
        do {
            result = connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (result == 0) break;
            attempts++;
            Core::Logger::Instance().Warning("AI service connect attempt failed, retrying... (" + std::to_string(attempts) + "/" + std::to_string(maxAttempts) + ")");
            Sleep(300);
        } while (attempts < maxAttempts);
        if (result == SOCKET_ERROR) {
            m_lastError = "Failed to connect: " + std::to_string(WSAGetLastError());
            Core::Logger::Instance().Error(m_lastError);
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }

        // Configure socket timeouts
        DWORD recvTimeout = 5000; // 5s
        DWORD sendTimeout = 5000;
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout));
        setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sendTimeout, sizeof(sendTimeout));

        // Health check: GET /health to ensure service + Ollama availability
        {
            std::string healthResponse;
            if (!SendHTTPRequest("GET", "/health", "", healthResponse)) {
                Core::Logger::Instance().Error("AI service health check failed");
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                return false;
            }
            std::string body = ParseJSONResponse(healthResponse);
            if (body.find("\"status\": \"healthy\"") == std::string::npos) {
                Core::Logger::Instance().Warning("AI service responded but not healthy: " + body.substr(0, 200));
            }
            // Optional: ensure Ollama available
            if (body.find("\"available\": true") == std::string::npos) {
                Core::Logger::Instance().Warning("Ollama not reported available by AI service; fallback may be used.");
            }
        }

        m_connected = true;
        Core::Logger::Instance().Info("Connected to AI service successfully");
        return true;
    }

    bool AIClient::Disconnect()
    {
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
        m_connected = false;
        Core::Logger::Instance().Info("Disconnected from AI service");
        return true;
    }

    AIResponse AIClient::SendCommand(const std::string& command)
    {
        return SendCommandWithContext(command, "{}");
    }

    AIResponse AIClient::SendCommandWithContext(const std::string& command, const std::string& sceneContext)
    {
        AIResponse response;
        response.success = false;

        if (!m_connected) {
            m_lastError = "Not connected to AI service";
            Core::Logger::Instance().Error(m_lastError);
            response.message = m_lastError;
            return response;
        }

        try {
            // Build JSON request body (escape command safely; sceneContext is already JSON)
            std::stringstream body;
            body << "{";
            body << "\"command\": \"" << EscapeJSONString(command) << "\",";
            body << "\"scene_state\": " << sceneContext;
            body << "}";

            std::string responseStr;
            if (SendHTTPRequest("POST", "/api/v1/command", body.str(), responseStr)) {
                std::string json = ParseJSONResponse(responseStr);
                response.success = json.find("\"success\": true") != std::string::npos;
                
                // Extract message
                size_t msgKey = json.find("\"message\"");
                if (msgKey != std::string::npos) {
                    size_t colon = json.find(':', msgKey);
                    size_t quote1 = json.find('"', colon + 1);
                    size_t quote2 = json.find('"', quote1 + 1);
                    if (quote1 != std::string::npos && quote2 != std::string::npos && quote2 > quote1) {
                        response.message = json.substr(quote1 + 1, quote2 - quote1 - 1);
                    }
                }
                if (response.message.empty()) {
                    response.message = response.success ? "Command processed" : "Command failed";
                }

                // Extract actions array (very simple parsing of strings inside \"actions\":[...])
                size_t actionsPos = json.find("\"actions\"");
                if (actionsPos != std::string::npos) {
                    size_t bracketL = json.find('[', actionsPos);
                    size_t bracketR = json.find(']', bracketL);
                    if (bracketL != std::string::npos && bracketR != std::string::npos && bracketR > bracketL) {
                        std::string arr = json.substr(bracketL + 1, bracketR - bracketL - 1);
                        size_t pos = 0;
                        while (true) {
                            size_t q1 = arr.find('"', pos);
                            if (q1 == std::string::npos) break;
                            size_t q2 = arr.find('"', q1 + 1);
                            if (q2 == std::string::npos) break;
                            std::string act = arr.substr(q1 + 1, q2 - q1 - 1);
                            if (!act.empty()) response.actions.push_back(act);
                            pos = q2 + 1;
                        }
                    }
                }

                // Extract suggestions if provided
                size_t suggPos = json.find("\"suggestions\"");
                if (suggPos != std::string::npos) {
                    size_t bracketL = json.find('[', suggPos);
                    size_t bracketR = json.find(']', bracketL);
                    if (bracketL != std::string::npos && bracketR != std::string::npos && bracketR > bracketL) {
                        std::string arr = json.substr(bracketL + 1, bracketR - bracketL - 1);
                        size_t pos = 0;
                        while (true) {
                            size_t q1 = arr.find('"', pos);
                            if (q1 == std::string::npos) break;
                            size_t q2 = arr.find('"', q1 + 1);
                            if (q2 == std::string::npos) break;
                            response.suggestions.push_back(arr.substr(q1 + 1, q2 - q1 - 1));
                            pos = q2 + 1;
                        }
                    }
                }

                Core::Logger::Instance().Info("AI command processed (success=" + std::string(response.success ? "true" : "false") + ")");
            } else {
                response.message = "Failed to send command";
                Core::Logger::Instance().Error(response.message);
            }
        } catch (const std::exception& e) {
            response.message = "Exception: " + std::string(e.what());
            Core::Logger::Instance().Error(response.message);
        }

        return response;
    }

    bool AIClient::UpdateSceneState(const std::string& sceneStateJson)
    {
        if (!m_connected) {
            return false;
        }

        std::string response;
        return SendHTTPRequest("POST", "/api/v1/scene/update", sceneStateJson, response);
    }

    std::vector<std::string> AIClient::GetSuggestions(const std::string& partialCommand)
    {
        std::vector<std::string> suggestions;

        if (!m_connected) {
            return suggestions;
        }

        std::stringstream body;
        body << "{\"partial_command\": \"" << partialCommand << "\"}";

        std::string response;
        if (SendHTTPRequest("POST", "/api/v1/suggestions", body.str(), response)) {
            std::string json = ParseJSONResponse(response);
            size_t suggPos = json.find("\"suggestions\"");
            if (suggPos != std::string::npos) {
                size_t bracketL = json.find('[', suggPos);
                size_t bracketR = json.find(']', bracketL);
                if (bracketL != std::string::npos && bracketR != std::string::npos && bracketR > bracketL) {
                    std::string arr = json.substr(bracketL + 1, bracketR - bracketL - 1);
                    size_t pos = 0;
                    while (true) {
                        size_t q1 = arr.find('"', pos);
                        if (q1 == std::string::npos) break;
                        size_t q2 = arr.find('"', q1 + 1);
                        if (q2 == std::string::npos) break;
                        suggestions.push_back(arr.substr(q1 + 1, q2 - q1 - 1));
                        pos = q2 + 1;
                    }
                }
            }
        }

        return suggestions;
    }

    std::string AIClient::GetTemplates()
    {
        if (!m_connected) {
            return "{}";
        }

        std::string response;
        if (SendHTTPRequest("GET", "/api/v1/templates", "", response)) {
            return response;
        }

        return "{}";
    }

    std::string AIClient::BuildHTTPRequest(const std::string& method, const std::string& path, const std::string& body)
    {
        std::stringstream request;
        
        request << method << " " << path << " HTTP/1.1\r\n";
        request << "Host: " << m_host << ":" << m_port << "\r\n";
        request << "Content-Type: application/json\r\n";
        request << "Content-Length: " << body.length() << "\r\n";
        request << "Connection: keep-alive\r\n";
        request << "\r\n";
        request << body;

        return request.str();
    }

    bool AIClient::SendHTTPRequest(const std::string& method, const std::string& path, const std::string& body, std::string& response)
    {
        if (!m_connected) {
            return false;
        }

        try {
            // Build HTTP request
            std::string request = BuildHTTPRequest(method, path, body);

            Core::Logger::Instance().Debug("Sending HTTP request: " + request.substr(0, 100) + "...");

            // Send request
            int result = send(m_socket, request.c_str(), static_cast<int>(request.length()), 0);
            if (result == SOCKET_ERROR) {
                m_lastError = "Failed to send: " + std::to_string(WSAGetLastError());
                Core::Logger::Instance().Error(m_lastError);
                return false;
            }

            // Receive response
            if (!ReceiveHTTPResponse(response)) {
                return false;
            }

            Core::Logger::Instance().Debug("Received HTTP response: " + response.substr(0, 100) + "...");
            return true;

        } catch (const std::exception& e) {
            m_lastError = "Exception: " + std::string(e.what());
            Core::Logger::Instance().Error(m_lastError);
            return false;
        }
    }

    bool AIClient::ReceiveHTTPResponse(std::string& response)
    {
        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        std::stringstream responseStream;

        // Receive data
        while (true) {
            int bytesReceived = recv(m_socket, buffer, BUFFER_SIZE - 1, 0);
            
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                responseStream << buffer;

                // Check if we received the end of HTTP response
                std::string currentResponse = responseStream.str();
                if (currentResponse.find("\r\n\r\n") != std::string::npos) {
                    // Check for Content-Length to know when we have the full body
                    size_t contentLengthPos = currentResponse.find("Content-Length: ");
                    if (contentLengthPos != std::string::npos) {
                        // Parse content length
                        size_t endPos = currentResponse.find("\r\n", contentLengthPos);
                        std::string lengthStr = currentResponse.substr(contentLengthPos + 16, endPos - contentLengthPos - 16);
                        int contentLength = std::stoi(lengthStr);

                        // Check if we have the full body
                        size_t bodyStart = currentResponse.find("\r\n\r\n") + 4;
                        if (currentResponse.length() - bodyStart >= static_cast<size_t>(contentLength)) {
                            break;
                        }
                    } else {
                        // No Content-Length, assume we got everything
                        break;
                    }
                }
            } else if (bytesReceived == 0) {
                // Connection closed
                break;
            } else {
                m_lastError = "Receive failed: " + std::to_string(WSAGetLastError());
                Core::Logger::Instance().Error(m_lastError);
                return false;
            }
        }

        response = responseStream.str();
        return true;
    }

    std::string AIClient::ParseJSONResponse(const std::string& response)
    {
        // Find the JSON body (after HTTP headers)
        size_t bodyStart = response.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            return response.substr(bodyStart + 4);
        }
        return response;
    }

} // namespace AI
} // namespace Engine

