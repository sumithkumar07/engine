#pragma once

#include <string>
#include <fstream>
#include <memory>

namespace Engine {
namespace Core {

    /**
     * @brief Logger class - Same as before, but with proper thread safety
     * 
     * Thread-safe logging system for debugging and information.
     * Same design as the previous implementation, but with proper mutex usage.
     */
    class Logger {
    public:
        enum class Level {
            Debug = 0,
            Info = 1,
            Warning = 2,
            Error = 3
        };

        // Singleton access
        static Logger& Instance();
        
        // Initialization
        bool Initialize(const std::string& logFile = "ai_movie_studio.log");
        void Shutdown();

        // Logging methods
        void Log(Level level, const std::string& message);
        void Debug(const std::string& message);
        void Info(const std::string& message);
        void Warning(const std::string& message);
        void Error(const std::string& message);

        // Configuration
        void SetLevel(Level level) { m_level = level; }
        void SetConsoleOutput(bool enabled) { m_consoleOutput = enabled; }
        void SetFileOutput(bool enabled) { m_fileOutput = enabled; }

    private:
        Logger() : m_level(Level::Info), m_consoleOutput(true), m_fileOutput(false) {}
        ~Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        Level m_level;
        bool m_consoleOutput;
        bool m_fileOutput;
        std::ofstream m_logFile;

        std::string GetLevelString(Level level) const;
        std::string GetTimestamp() const;
    };

    // Convenience macros
    #define LOG_DEBUG(message) Logger::Instance().Debug(message)
    #define LOG_INFO(message) Logger::Instance().Info(message)
    #define LOG_WARNING(message) Logger::Instance().Warning(message)
    #define LOG_ERROR(message) Logger::Instance().Error(message)

} // namespace Core
} // namespace Engine
