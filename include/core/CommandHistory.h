#pragma once

#include "core/Command.h"
#include <memory>
#include <vector>
#include <functional>

namespace Engine {
namespace Core {

// Forward declaration
class SceneManager;

/**
 * @brief Command history manager for undo/redo functionality
 * 
 * Maintains stacks of executed commands and provides undo/redo operations.
 * Supports command merging for continuous operations (e.g., dragging).
 */
class CommandHistory {
public:
    CommandHistory();
    ~CommandHistory();

    /**
     * @brief Execute and record a command
     * @param command Command to execute
     * @param sceneManager Scene manager to operate on
     * @param mergeable If true, try to merge with last command
     * @return true if successful
     */
    bool ExecuteCommand(std::unique_ptr<Command> command, SceneManager* sceneManager, bool mergeable = false);

    /**
     * @brief Undo the last command
     * @param sceneManager Scene manager to operate on
     * @return true if successful
     */
    bool Undo(SceneManager* sceneManager);

    /**
     * @brief Redo the last undone command
     * @param sceneManager Scene manager to operate on
     * @return true if successful
     */
    bool Redo(SceneManager* sceneManager);

    /**
     * @brief Clear all history
     */
    void Clear();

    /**
     * @brief Check if undo is available
     */
    bool CanUndo() const { return !m_undoStack.empty(); }

    /**
     * @brief Check if redo is available
     */
    bool CanRedo() const { return !m_redoStack.empty(); }

    /**
     * @brief Get description of next undo command
     */
    std::string GetUndoDescription() const;

    /**
     * @brief Get description of next redo command
     */
    std::string GetRedoDescription() const;

    /**
     * @brief Get undo stack size
     */
    size_t GetUndoCount() const { return m_undoStack.size(); }

    /**
     * @brief Get redo stack size
     */
    size_t GetRedoCount() const { return m_redoStack.size(); }

    /**
     * @brief Set maximum history size (0 = unlimited)
     */
    void SetMaxHistorySize(size_t maxSize) { m_maxHistorySize = maxSize; }

    /**
     * @brief Get maximum history size
     */
    size_t GetMaxHistorySize() const { return m_maxHistorySize; }

    /**
     * @brief Set callback for history changed events
     */
    void SetHistoryChangedCallback(std::function<void()> callback) { m_historyChangedCallback = callback; }

private:
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
    size_t m_maxHistorySize;
    std::function<void()> m_historyChangedCallback;

    void TrimHistory();
    void NotifyHistoryChanged();
};

} // namespace Core
} // namespace Engine

