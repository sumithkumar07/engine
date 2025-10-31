#pragma once

#include <vector>
#include <DirectXMath.h>

namespace Engine {
namespace Animation {

/**
 * @brief Keyframe for animation curves
 */
struct AnimationKeyframe {
    float time;           // Time in seconds
    float value;          // Keyframe value
    float inTangent;      // Incoming tangent (for smooth interpolation)
    float outTangent;     // Outgoing tangent
    
    AnimationKeyframe() : time(0.0f), value(0.0f), inTangent(0.0f), outTangent(0.0f) {}
    AnimationKeyframe(float t, float v) : time(t), value(v), inTangent(0.0f), outTangent(0.0f) {}
    AnimationKeyframe(float t, float v, float in, float out) : time(t), value(v), inTangent(in), outTangent(out) {}
};

/**
 * @brief Interpolation modes for animation curves
 */
enum class InterpolationMode {
    Linear,       // Linear interpolation between keyframes
    Smooth,       // Hermite/Bezier smooth interpolation
    Step,         // Step (no interpolation, hold value)
    CubicBezier   // Full cubic Bezier control
};

/**
 * @brief Animation curve for single property
 * 
 * Stores keyframes and evaluates values at any time point.
 * Used for animating float properties (position.x, rotation.y, etc.)
 */
class AnimationCurve {
public:
    AnimationCurve();
    AnimationCurve(InterpolationMode mode);
    ~AnimationCurve();

    // Keyframe management
    void AddKeyframe(float time, float value);
    void AddKeyframe(const AnimationKeyframe& keyframe);
    void RemoveKeyframe(int index);
    void ClearKeyframes();
    
    // Keyframe editing
    void SetKeyframeTime(int index, float time);
    void SetKeyframeValue(int index, float value);
    void SetKeyframeTangents(int index, float inTangent, float outTangent);
    
    // Evaluation
    float Evaluate(float time) const;
    
    // Getters
    const std::vector<AnimationKeyframe>& GetKeyframes() const { return m_keyframes; }
    int GetKeyframeCount() const { return static_cast<int>(m_keyframes.size()); }
    InterpolationMode GetInterpolationMode() const { return m_interpolationMode; }
    void SetInterpolationMode(InterpolationMode mode) { m_interpolationMode = mode; }
    
    // Time range
    float GetStartTime() const;
    float GetEndTime() const;
    float GetDuration() const;

private:
    std::vector<AnimationKeyframe> m_keyframes;
    InterpolationMode m_interpolationMode;
    
    // Interpolation methods
    float LinearInterpolate(const AnimationKeyframe& a, const AnimationKeyframe& b, float t) const;
    float HermiteInterpolate(const AnimationKeyframe& a, const AnimationKeyframe& b, float t) const;
    float BezierInterpolate(const AnimationKeyframe& a, const AnimationKeyframe& b, float t) const;
    
    // Helper methods
    void SortKeyframes();
    int FindKeyframeIndex(float time) const;
};

/**
 * @brief Vector3 animation curve (for position, rotation, scale)
 */
class Vector3Curve {
public:
    AnimationCurve x;
    AnimationCurve y;
    AnimationCurve z;
    
    Vector3Curve() {}
    Vector3Curve(InterpolationMode mode) : x(mode), y(mode), z(mode) {}
    
    void AddKeyframe(float time, const DirectX::XMFLOAT3& value) {
        x.AddKeyframe(time, value.x);
        y.AddKeyframe(time, value.y);
        z.AddKeyframe(time, value.z);
    }
    
    DirectX::XMFLOAT3 Evaluate(float time) const {
        return DirectX::XMFLOAT3(
            x.Evaluate(time),
            y.Evaluate(time),
            z.Evaluate(time)
        );
    }
    
    float GetDuration() const {
        return std::max({x.GetDuration(), y.GetDuration(), z.GetDuration()});
    }
};

} // namespace Animation
} // namespace Engine

