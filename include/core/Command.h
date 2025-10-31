#pragma once

#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

namespace Engine {
namespace Core {

// Forward declarations
class SceneObject;
class SceneManager;

/**
 * @brief Base command interface for undo/redo system
 * 
 * Implements the Command pattern for undoable operations.
 * All modifications to the scene should create commands.
 */
class Command {
public:
    virtual ~Command() = default;

    /**
     * @brief Execute the command
     * @param sceneManager Scene manager to operate on
     * @return true if successful
     */
    virtual bool Execute(SceneManager* sceneManager) = 0;

    /**
     * @brief Undo the command (reverse the operation)
     * @param sceneManager Scene manager to operate on
     * @return true if successful
     */
    virtual bool Undo(SceneManager* sceneManager) = 0;

    /**
     * @brief Get command description for UI display
     * @return Human-readable description
     */
    virtual std::string GetDescription() const = 0;

    /**
     * @brief Check if command can be merged with another
     * @param other Other command to check
     * @return true if commands can be merged
     */
    virtual bool CanMergeWith(const Command* other) const { (void)other; return false; }

    /**
     * @brief Merge this command with another (for continuous operations)
     * @param other Other command to merge
     */
    virtual void MergeWith(const Command* other) { (void)other; }
};

// ============================================================================
// TRANSFORM COMMANDS
// ============================================================================

/**
 * @brief Command to set object position
 */
class SetPositionCommand : public Command {
public:
    SetPositionCommand(const std::string& objectName, const DirectX::XMFLOAT3& newPosition);
    
    bool Execute(SceneManager* sceneManager) override;
    bool Undo(SceneManager* sceneManager) override;
    std::string GetDescription() const override;
    bool CanMergeWith(const Command* other) const override;
    void MergeWith(const Command* other) override;

private:
    std::string m_objectName;
    DirectX::XMFLOAT3 m_newPosition;
    DirectX::XMFLOAT3 m_oldPosition;
    bool m_hasOldPosition;
};

/**
 * @brief Command to set object rotation
 */
class SetRotationCommand : public Command {
public:
    SetRotationCommand(const std::string& objectName, const DirectX::XMFLOAT3& newRotation);
    
    bool Execute(SceneManager* sceneManager) override;
    bool Undo(SceneManager* sceneManager) override;
    std::string GetDescription() const override;
    bool CanMergeWith(const Command* other) const override;
    void MergeWith(const Command* other) override;

private:
    std::string m_objectName;
    DirectX::XMFLOAT3 m_newRotation;
    DirectX::XMFLOAT3 m_oldRotation;
    bool m_hasOldRotation;
};

/**
 * @brief Command to set object scale
 */
class SetScaleCommand : public Command {
public:
    SetScaleCommand(const std::string& objectName, const DirectX::XMFLOAT3& newScale);
    
    bool Execute(SceneManager* sceneManager) override;
    bool Undo(SceneManager* sceneManager) override;
    std::string GetDescription() const override;
    bool CanMergeWith(const Command* other) const override;
    void MergeWith(const Command* other) override;

private:
    std::string m_objectName;
    DirectX::XMFLOAT3 m_newScale;
    DirectX::XMFLOAT3 m_oldScale;
    bool m_hasOldScale;
};

// ============================================================================
// OBJECT LIFECYCLE COMMANDS
// ============================================================================

/**
 * @brief Command to create an object
 */
class CreateObjectCommand : public Command {
public:
    CreateObjectCommand(const std::string& objectName, const std::string& objectType);
    
    bool Execute(SceneManager* sceneManager) override;
    bool Undo(SceneManager* sceneManager) override;
    std::string GetDescription() const override;

private:
    std::string m_objectName;
    std::string m_objectType;
};

/**
 * @brief Command to delete an object
 */
class DeleteObjectCommand : public Command {
public:
    DeleteObjectCommand(const std::string& objectName);
    
    bool Execute(SceneManager* sceneManager) override;
    bool Undo(SceneManager* sceneManager) override;
    std::string GetDescription() const override;

private:
    std::string m_objectName;
    std::string m_objectType;
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT3 m_rotation;
    DirectX::XMFLOAT3 m_scale;
    bool m_wasDeleted;
};

// ============================================================================
// COMPOSITE COMMAND
// ============================================================================

/**
 * @brief Composite command (groups multiple commands)
 */
class CompositeCommand : public Command {
public:
    CompositeCommand(const std::string& description);
    
    void AddCommand(std::unique_ptr<Command> command);
    
    bool Execute(SceneManager* sceneManager) override;
    bool Undo(SceneManager* sceneManager) override;
    std::string GetDescription() const override;

private:
    std::string m_description;
    std::vector<std::unique_ptr<Command>> m_commands;
};

} // namespace Core
} // namespace Engine

