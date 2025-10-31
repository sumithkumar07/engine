#include "core/Command.h"
#include "core/SceneManager.h"
#include "core/SceneObject.h"
#include "core/Logger.h"

namespace Engine {
namespace Core {

// ============================================================================
// SetPositionCommand
// ============================================================================

SetPositionCommand::SetPositionCommand(const std::string& objectName, const DirectX::XMFLOAT3& newPosition)
    : m_objectName(objectName)
    , m_newPosition(newPosition)
    , m_oldPosition(0, 0, 0)
    , m_hasOldPosition(false) {
}

bool SetPositionCommand::Execute(SceneManager* sceneManager) {
    auto obj = sceneManager->FindObjectByName(m_objectName);
    if (!obj) {
        Logger::Instance().Error("SetPositionCommand: Object not found: " + m_objectName);
        return false;
    }

    // Store old position for undo
    if (!m_hasOldPosition) {
        m_oldPosition = obj->GetPosition();
        m_hasOldPosition = true;
    }

    obj->SetPosition(m_newPosition);
    return true;
}

bool SetPositionCommand::Undo(SceneManager* sceneManager) {
    auto obj = sceneManager->FindObjectByName(m_objectName);
    if (!obj) {
        Logger::Instance().Error("SetPositionCommand::Undo: Object not found: " + m_objectName);
        return false;
    }

    obj->SetPosition(m_oldPosition);
    return true;
}

std::string SetPositionCommand::GetDescription() const {
    return "Move " + m_objectName;
}

bool SetPositionCommand::CanMergeWith(const Command* other) const {
    const SetPositionCommand* otherCmd = dynamic_cast<const SetPositionCommand*>(other);
    return otherCmd && otherCmd->m_objectName == m_objectName;
}

void SetPositionCommand::MergeWith(const Command* other) {
    const SetPositionCommand* otherCmd = static_cast<const SetPositionCommand*>(other);
    m_newPosition = otherCmd->m_newPosition;
}

// ============================================================================
// SetRotationCommand
// ============================================================================

SetRotationCommand::SetRotationCommand(const std::string& objectName, const DirectX::XMFLOAT3& newRotation)
    : m_objectName(objectName)
    , m_newRotation(newRotation)
    , m_oldRotation(0, 0, 0)
    , m_hasOldRotation(false) {
}

bool SetRotationCommand::Execute(SceneManager* sceneManager) {
    auto obj = sceneManager->FindObjectByName(m_objectName);
    if (!obj) {
        Logger::Instance().Error("SetRotationCommand: Object not found: " + m_objectName);
        return false;
    }

    if (!m_hasOldRotation) {
        m_oldRotation = obj->GetRotation();
        m_hasOldRotation = true;
    }

    obj->SetRotation(m_newRotation);
    return true;
}

bool SetRotationCommand::Undo(SceneManager* sceneManager) {
    auto obj = sceneManager->FindObjectByName(m_objectName);
    if (!obj) {
        Logger::Instance().Error("SetRotationCommand::Undo: Object not found: " + m_objectName);
        return false;
    }

    obj->SetRotation(m_oldRotation);
    return true;
}

std::string SetRotationCommand::GetDescription() const {
    return "Rotate " + m_objectName;
}

bool SetRotationCommand::CanMergeWith(const Command* other) const {
    const SetRotationCommand* otherCmd = dynamic_cast<const SetRotationCommand*>(other);
    return otherCmd && otherCmd->m_objectName == m_objectName;
}

void SetRotationCommand::MergeWith(const Command* other) {
    const SetRotationCommand* otherCmd = static_cast<const SetRotationCommand*>(other);
    m_newRotation = otherCmd->m_newRotation;
}

// ============================================================================
// SetScaleCommand
// ============================================================================

SetScaleCommand::SetScaleCommand(const std::string& objectName, const DirectX::XMFLOAT3& newScale)
    : m_objectName(objectName)
    , m_newScale(newScale)
    , m_oldScale(1, 1, 1)
    , m_hasOldScale(false) {
}

bool SetScaleCommand::Execute(SceneManager* sceneManager) {
    auto obj = sceneManager->FindObjectByName(m_objectName);
    if (!obj) {
        Logger::Instance().Error("SetScaleCommand: Object not found: " + m_objectName);
        return false;
    }

    if (!m_hasOldScale) {
        m_oldScale = obj->GetScale();
        m_hasOldScale = true;
    }

    obj->SetScale(m_newScale);
    return true;
}

bool SetScaleCommand::Undo(SceneManager* sceneManager) {
    auto obj = sceneManager->FindObjectByName(m_objectName);
    if (!obj) {
        Logger::Instance().Error("SetScaleCommand::Undo: Object not found: " + m_objectName);
        return false;
    }

    obj->SetScale(m_oldScale);
    return true;
}

std::string SetScaleCommand::GetDescription() const {
    return "Scale " + m_objectName;
}

bool SetScaleCommand::CanMergeWith(const Command* other) const {
    const SetScaleCommand* otherCmd = dynamic_cast<const SetScaleCommand*>(other);
    return otherCmd && otherCmd->m_objectName == m_objectName;
}

void SetScaleCommand::MergeWith(const Command* other) {
    const SetScaleCommand* otherCmd = static_cast<const SetScaleCommand*>(other);
    m_newScale = otherCmd->m_newScale;
}

// ============================================================================
// CreateObjectCommand
// ============================================================================

CreateObjectCommand::CreateObjectCommand(const std::string& objectName, const std::string& objectType)
    : m_objectName(objectName)
    , m_objectType(objectType) {
}

bool CreateObjectCommand::Execute(SceneManager* sceneManager) {
    auto obj = sceneManager->CreateObject(m_objectName, m_objectType);
    return obj != nullptr;
}

bool CreateObjectCommand::Undo(SceneManager* sceneManager) {
    return sceneManager->RemoveObject(m_objectName);
}

std::string CreateObjectCommand::GetDescription() const {
    return "Create " + m_objectName;
}

// ============================================================================
// DeleteObjectCommand
// ============================================================================

DeleteObjectCommand::DeleteObjectCommand(const std::string& objectName)
    : m_objectName(objectName)
    , m_objectType("Unknown")
    , m_position(0, 0, 0)
    , m_rotation(0, 0, 0)
    , m_scale(1, 1, 1)
    , m_wasDeleted(false) {
}

bool DeleteObjectCommand::Execute(SceneManager* sceneManager) {
    // Store object data before deleting
    auto obj = sceneManager->FindObjectByName(m_objectName);
    if (obj) {
        m_objectType = obj->GetType();
        m_position = obj->GetPosition();
        m_rotation = obj->GetRotation();
        m_scale = obj->GetScale();
        m_wasDeleted = true;
    }

    return sceneManager->RemoveObject(m_objectName);
}

bool DeleteObjectCommand::Undo(SceneManager* sceneManager) {
    if (!m_wasDeleted) {
        return false;
    }

    // Recreate object with stored data
    auto obj = sceneManager->CreateObject(m_objectName, m_objectType);
    if (obj) {
        obj->SetPosition(m_position);
        obj->SetRotation(m_rotation);
        obj->SetScale(m_scale);
        return true;
    }

    return false;
}

std::string DeleteObjectCommand::GetDescription() const {
    return "Delete " + m_objectName;
}

// ============================================================================
// CompositeCommand
// ============================================================================

CompositeCommand::CompositeCommand(const std::string& description)
    : m_description(description) {
}

void CompositeCommand::AddCommand(std::unique_ptr<Command> command) {
    m_commands.push_back(std::move(command));
}

bool CompositeCommand::Execute(SceneManager* sceneManager) {
    for (auto& cmd : m_commands) {
        if (!cmd->Execute(sceneManager)) {
            return false;
        }
    }
    return true;
}

bool CompositeCommand::Undo(SceneManager* sceneManager) {
    // Undo in reverse order
    for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
        if (!(*it)->Undo(sceneManager)) {
            return false;
        }
    }
    return true;
}

std::string CompositeCommand::GetDescription() const {
    return m_description;
}

} // namespace Core
} // namespace Engine

