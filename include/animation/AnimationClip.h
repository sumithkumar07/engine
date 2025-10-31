#pragma once

#include "animation/AnimationCurve.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <DirectXMath.h>

namespace Engine {
namespace Animation {

/**
 * @brief Animation property types
 */
enum class AnimationProperty {
    PositionX, PositionY, PositionZ,
    RotationX, RotationY, RotationZ,
    ScaleX, ScaleY, ScaleZ,
    Custom  // For custom float properties
};

/**
 * @brief Animation clip containing multiple curves
 * 
 * Represents a complete animation for one or more objects.
 * Contains curves for different properties (position, rotation, scale, etc.)
 */
class AnimationClip {
public:
    AnimationClip(const std::string& name);
    ~AnimationClip();

    // Basic properties
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    
    float GetDuration() const { return m_duration; }
    void UpdateDuration(); // Recalculate from curves
    
    bool IsLooping() const { return m_looping; }
    void SetLooping(bool looping) { m_looping = looping; }
    
    float GetSpeed() const { return m_speed; }
    void SetSpeed(float speed) { m_speed = speed; }

    // Curve management
    void AddCurve(const std::string& objectName, AnimationProperty property, const AnimationCurve& curve);
    void RemoveCurve(const std::string& objectName, AnimationProperty property);
    AnimationCurve* GetCurve(const std::string& objectName, AnimationProperty property);
    const AnimationCurve* GetCurve(const std::string& objectName, AnimationProperty property) const;
    
    // Convenience methods for Vector3 properties
    void AddPositionCurve(const std::string& objectName, const Vector3Curve& curve);
    void AddRotationCurve(const std::string& objectName, const Vector3Curve& curve);
    void AddScaleCurve(const std::string& objectName, const Vector3Curve& curve);
    
    // Evaluation
    bool HasAnimationForObject(const std::string& objectName) const;
    DirectX::XMFLOAT3 EvaluatePosition(const std::string& objectName, float time) const;
    DirectX::XMFLOAT3 EvaluateRotation(const std::string& objectName, float time) const;
    DirectX::XMFLOAT3 EvaluateScale(const std::string& objectName, float time) const;
    
    // All animated objects
    std::vector<std::string> GetAnimatedObjects() const;

private:
    std::string m_name;
    float m_duration;
    bool m_looping;
    float m_speed;
    
    // Map: ObjectName -> Property -> Curve
    std::unordered_map<std::string, std::unordered_map<AnimationProperty, AnimationCurve>> m_curves;
    
    // Helper to generate curve key
    std::string GetCurveKey(const std::string& objectName, AnimationProperty property) const;
};

} // namespace Animation
} // namespace Engine

