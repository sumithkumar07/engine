#include "animation/AnimationClip.h"
#include "core/Logger.h"
#include <algorithm>

namespace Engine {
namespace Animation {

AnimationClip::AnimationClip(const std::string& name)
    : m_name(name)
    , m_duration(0.0f)
    , m_looping(false)
    , m_speed(1.0f) {
}

AnimationClip::~AnimationClip() {
}

void AnimationClip::UpdateDuration() {
    m_duration = 0.0f;
    
    for (const auto& objectPair : m_curves) {
        for (const auto& curvePair : objectPair.second) {
            float curveDuration = curvePair.second.GetDuration();
            if (curveDuration > m_duration) {
                m_duration = curveDuration;
            }
        }
    }
}

void AnimationClip::AddCurve(const std::string& objectName, AnimationProperty property, const AnimationCurve& curve) {
    m_curves[objectName][property] = curve;
    UpdateDuration();
}

void AnimationClip::RemoveCurve(const std::string& objectName, AnimationProperty property) {
    auto objIt = m_curves.find(objectName);
    if (objIt != m_curves.end()) {
        objIt->second.erase(property);
        if (objIt->second.empty()) {
            m_curves.erase(objIt);
        }
    }
    UpdateDuration();
}

AnimationCurve* AnimationClip::GetCurve(const std::string& objectName, AnimationProperty property) {
    auto objIt = m_curves.find(objectName);
    if (objIt != m_curves.end()) {
        auto propIt = objIt->second.find(property);
        if (propIt != objIt->second.end()) {
            return &propIt->second;
        }
    }
    return nullptr;
}

const AnimationCurve* AnimationClip::GetCurve(const std::string& objectName, AnimationProperty property) const {
    auto objIt = m_curves.find(objectName);
    if (objIt != m_curves.end()) {
        auto propIt = objIt->second.find(property);
        if (propIt != objIt->second.end()) {
            return &propIt->second;
        }
    }
    return nullptr;
}

void AnimationClip::AddPositionCurve(const std::string& objectName, const Vector3Curve& curve) {
    m_curves[objectName][AnimationProperty::PositionX] = curve.x;
    m_curves[objectName][AnimationProperty::PositionY] = curve.y;
    m_curves[objectName][AnimationProperty::PositionZ] = curve.z;
    UpdateDuration();
}

void AnimationClip::AddRotationCurve(const std::string& objectName, const Vector3Curve& curve) {
    m_curves[objectName][AnimationProperty::RotationX] = curve.x;
    m_curves[objectName][AnimationProperty::RotationY] = curve.y;
    m_curves[objectName][AnimationProperty::RotationZ] = curve.z;
    UpdateDuration();
}

void AnimationClip::AddScaleCurve(const std::string& objectName, const Vector3Curve& curve) {
    m_curves[objectName][AnimationProperty::ScaleX] = curve.x;
    m_curves[objectName][AnimationProperty::ScaleY] = curve.y;
    m_curves[objectName][AnimationProperty::ScaleZ] = curve.z;
    UpdateDuration();
}

bool AnimationClip::HasAnimationForObject(const std::string& objectName) const {
    return m_curves.find(objectName) != m_curves.end();
}

DirectX::XMFLOAT3 AnimationClip::EvaluatePosition(const std::string& objectName, float time) const {
    DirectX::XMFLOAT3 result(0.0f, 0.0f, 0.0f);
    
    auto objIt = m_curves.find(objectName);
    if (objIt != m_curves.end()) {
        const auto& curves = objIt->second;
        
        auto xIt = curves.find(AnimationProperty::PositionX);
        if (xIt != curves.end()) result.x = xIt->second.Evaluate(time);
        
        auto yIt = curves.find(AnimationProperty::PositionY);
        if (yIt != curves.end()) result.y = yIt->second.Evaluate(time);
        
        auto zIt = curves.find(AnimationProperty::PositionZ);
        if (zIt != curves.end()) result.z = zIt->second.Evaluate(time);
    }
    
    return result;
}

DirectX::XMFLOAT3 AnimationClip::EvaluateRotation(const std::string& objectName, float time) const {
    DirectX::XMFLOAT3 result(0.0f, 0.0f, 0.0f);
    
    auto objIt = m_curves.find(objectName);
    if (objIt != m_curves.end()) {
        const auto& curves = objIt->second;
        
        auto xIt = curves.find(AnimationProperty::RotationX);
        if (xIt != curves.end()) result.x = xIt->second.Evaluate(time);
        
        auto yIt = curves.find(AnimationProperty::RotationY);
        if (yIt != curves.end()) result.y = yIt->second.Evaluate(time);
        
        auto zIt = curves.find(AnimationProperty::RotationZ);
        if (zIt != curves.end()) result.z = zIt->second.Evaluate(time);
    }
    
    return result;
}

DirectX::XMFLOAT3 AnimationClip::EvaluateScale(const std::string& objectName, float time) const {
    DirectX::XMFLOAT3 result(1.0f, 1.0f, 1.0f); // Default scale
    
    auto objIt = m_curves.find(objectName);
    if (objIt != m_curves.end()) {
        const auto& curves = objIt->second;
        
        auto xIt = curves.find(AnimationProperty::ScaleX);
        if (xIt != curves.end()) result.x = xIt->second.Evaluate(time);
        
        auto yIt = curves.find(AnimationProperty::ScaleY);
        if (yIt != curves.end()) result.y = yIt->second.Evaluate(time);
        
        auto zIt = curves.find(AnimationProperty::ScaleZ);
        if (zIt != curves.end()) result.z = zIt->second.Evaluate(time);
    }
    
    return result;
}

std::vector<std::string> AnimationClip::GetAnimatedObjects() const {
    std::vector<std::string> objects;
    objects.reserve(m_curves.size());
    
    for (const auto& pair : m_curves) {
        objects.push_back(pair.first);
    }
    
    return objects;
}

std::string AnimationClip::GetCurveKey(const std::string& objectName, AnimationProperty property) const {
    return objectName + "_" + std::to_string(static_cast<int>(property));
}

} // namespace Animation
} // namespace Engine

