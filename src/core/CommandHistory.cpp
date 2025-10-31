#include "core/CommandHistory.h"
#include "core/SceneManager.h"
#include "core/Logger.h"

namespace Engine {
namespace Core {

CommandHistory::CommandHistory()
    : m_maxHistorySize(100) {
    Logger::Instance().Debug("CommandHistory created");
}

CommandHistory::~CommandHistory() {
    Clear();
    Logger::Instance().Debug("CommandHistory destroyed");
}

bool CommandHistory::ExecuteCommand(std::unique_ptr<Command> command, SceneManager* sceneManager, bool mergeable) {
    if (!command || !sceneManager) {
        Logger::Instance().Error("CommandHistory::ExecuteCommand - Invalid command or scene manager");
        return false;
    }

    // Try to merge with last command if mergeable
    if (mergeable && !m_undoStack.empty()) {
        Command* lastCmd = m_undoStack.back().get();
        if (lastCmd->CanMergeWith(command.get())) {
            lastCmd->MergeWith(command.get());
            Logger::Instance().Debug("Merged command: " + command->GetDescription());
            NotifyHistoryChanged();
            return true;
        }
    }

    // Execute command
    if (!command->Execute(sceneManager)) {
        Logger::Instance().Error("Failed to execute command: " + command->GetDescription());
        return false;
    }

    // Clear redo stack (new command invalidates redo history)
    m_redoStack.clear();

    // Add to undo stack
    m_undoStack.push_back(std::move(command));
    Logger::Instance().Debug("Executed command: " + m_undoStack.back()->GetDescription());

    // Trim history if needed
    TrimHistory();

    NotifyHistoryChanged();
    return true;
}

bool CommandHistory::Undo(SceneManager* sceneManager) {
    if (m_undoStack.empty() || !sceneManager) {
        return false;
    }

    // Pop command from undo stack
    std::unique_ptr<Command> command = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    // Undo the command
    if (!command->Undo(sceneManager)) {
        Logger::Instance().Error("Failed to undo command: " + command->GetDescription());
        // Put it back
        m_undoStack.push_back(std::move(command));
        return false;
    }

    Logger::Instance().Info("Undone: " + command->GetDescription());

    // Move to redo stack
    m_redoStack.push_back(std::move(command));

    NotifyHistoryChanged();
    return true;
}

bool CommandHistory::Redo(SceneManager* sceneManager) {
    if (m_redoStack.empty() || !sceneManager) {
        return false;
    }

    // Pop command from redo stack
    std::unique_ptr<Command> command = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    // Re-execute the command
    if (!command->Execute(sceneManager)) {
        Logger::Instance().Error("Failed to redo command: " + command->GetDescription());
        // Put it back
        m_redoStack.push_back(std::move(command));
        return false;
    }

    Logger::Instance().Info("Redone: " + command->GetDescription());

    // Move back to undo stack
    m_undoStack.push_back(std::move(command));

    NotifyHistoryChanged();
    return true;
}

void CommandHistory::Clear() {
    m_undoStack.clear();
    m_redoStack.clear();
    Logger::Instance().Info("Command history cleared");
    NotifyHistoryChanged();
}

std::string CommandHistory::GetUndoDescription() const {
    return m_undoStack.empty() ? "" : m_undoStack.back()->GetDescription();
}

std::string CommandHistory::GetRedoDescription() const {
    return m_redoStack.empty() ? "" : m_redoStack.back()->GetDescription();
}

void CommandHistory::TrimHistory() {
    if (m_maxHistorySize > 0 && m_undoStack.size() > m_maxHistorySize) {
        size_t excess = m_undoStack.size() - m_maxHistorySize;
        m_undoStack.erase(m_undoStack.begin(), m_undoStack.begin() + excess);
        Logger::Instance().Debug("Trimmed command history");
    }
}

void CommandHistory::NotifyHistoryChanged() {
    if (m_historyChangedCallback) {
        m_historyChangedCallback();
    }
}

} // namespace Core
} // namespace Engine

