#include "core/Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <windows.h>

namespace Engine {
namespace Core {

    Logger& Logger::Instance() {
        static Logger instance;
        return instance;
    }

    bool Logger::Initialize(const std::string& logFile) {
        try {
            // Initialize default settings
            m_level = Level::Info;
            m_consoleOutput = true;
            m_fileOutput = true;
            
            // Open log file
            if (m_fileOutput) {
                m_logFile.open(logFile, std::ios::out | std::ios::trunc);
                if (!m_logFile.is_open()) {
                    std::cerr << "Failed to open log file: " << logFile << std::endl;
                    m_fileOutput = false;
                }
            }
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Logger initialization failed: " << e.what() << std::endl;
            return false;
        } catch (...) {
            std::cerr << "Logger initialization failed: Unknown exception" << std::endl;
            return false;
        }
    }

    void Logger::Shutdown() {
        Log(Level::Info, "Logger shutting down");
        
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
    }

    void Logger::Log(Level level, const std::string& message) {
        // Check if we should log this level
        if (level < m_level) {
            return;
        }
        
        // Format the log message
        std::stringstream ss;
        ss << "[" << GetTimestamp() << "] ";
        ss << "[" << GetLevelString(level) << "] ";
        ss << message;
        
        std::string formattedMessage = ss.str();
        
        // Output to console
        if (m_consoleOutput) {
            if (level == Level::Error) {
                std::cerr << formattedMessage << std::endl;
            } else {
                std::cout << formattedMessage << std::endl;
            }
        }
        
        // Output to file
        if (m_fileOutput && m_logFile.is_open()) {
            m_logFile << formattedMessage << std::endl;
            m_logFile.flush();
        }
    }

    void Logger::Debug(const std::string& message) {
        Log(Level::Debug, message);
    }

    void Logger::Info(const std::string& message) {
        Log(Level::Info, message);
    }

    void Logger::Warning(const std::string& message) {
        Log(Level::Warning, message);
    }

    void Logger::Error(const std::string& message) {
        Log(Level::Error, message);
    }

    std::string Logger::GetLevelString(Level level) const {
        switch (level) {
            case Level::Debug:   return "DEBUG";
            case Level::Info:    return "INFO ";
            case Level::Warning: return "WARN ";
            case Level::Error:   return "ERROR";
            default:             return "UNKNOWN";
        }
    }

    std::string Logger::GetTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        struct tm timeinfo;
        localtime_s(&timeinfo, &time_t);
        ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        
        return ss.str();
    }

} // namespace Core
} // namespace Engine
