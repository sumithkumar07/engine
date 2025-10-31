#include "animation/AnimationCurve.h"
#include "core/Logger.h"
#include <algorithm>
#include <cmath>

namespace Engine {
namespace Animation {

AnimationCurve::AnimationCurve()
    : m_interpolationMode(InterpolationMode::Linear) {
}

AnimationCurve::AnimationCurve(InterpolationMode mode)
    : m_interpolationMode(mode) {
}

AnimationCurve::~AnimationCurve() {
}

void AnimationCurve::AddKeyframe(float time, float value) {
    m_keyframes.push_back(AnimationKeyframe(time, value));
    SortKeyframes();
}

void AnimationCurve::AddKeyframe(const AnimationKeyframe& keyframe) {
    m_keyframes.push_back(keyframe);
    SortKeyframes();
}

void AnimationCurve::RemoveKeyframe(int index) {
    if (index >= 0 && index < static_cast<int>(m_keyframes.size())) {
        m_keyframes.erase(m_keyframes.begin() + index);
    }
}

void AnimationCurve::ClearKeyframes() {
    m_keyframes.clear();
}

void AnimationCurve::SetKeyframeTime(int index, float time) {
    if (index >= 0 && index < static_cast<int>(m_keyframes.size())) {
        m_keyframes[index].time = time;
        SortKeyframes();
    }
}

void AnimationCurve::SetKeyframeValue(int index, float value) {
    if (index >= 0 && index < static_cast<int>(m_keyframes.size())) {
        m_keyframes[index].value = value;
    }
}

void AnimationCurve::SetKeyframeTangents(int index, float inTangent, float outTangent) {
    if (index >= 0 && index < static_cast<int>(m_keyframes.size())) {
        m_keyframes[index].inTangent = inTangent;
        m_keyframes[index].outTangent = outTangent;
    }
}

float AnimationCurve::Evaluate(float time) const {
    if (m_keyframes.empty()) {
        return 0.0f;
    }
    
    if (m_keyframes.size() == 1) {
        return m_keyframes[0].value;
    }
    
    // Clamp to curve range
    if (time <= m_keyframes.front().time) {
        return m_keyframes.front().value;
    }
    if (time >= m_keyframes.back().time) {
        return m_keyframes.back().value;
    }
    
    // Find surrounding keyframes
    int index = FindKeyframeIndex(time);
    if (index < 0 || index >= static_cast<int>(m_keyframes.size()) - 1) {
        return m_keyframes.back().value;
    }
    
    const AnimationKeyframe& a = m_keyframes[index];
    const AnimationKeyframe& b = m_keyframes[index + 1];
    
    // Normalize time between keyframes (0 to 1)
    float t = (time - a.time) / (b.time - a.time);
    
    // Interpolate based on mode
    switch (m_interpolationMode) {
        case InterpolationMode::Linear:
            return LinearInterpolate(a, b, t);
        case InterpolationMode::Smooth:
            return HermiteInterpolate(a, b, t);
        case InterpolationMode::Step:
            return a.value;
        case InterpolationMode::CubicBezier:
            return BezierInterpolate(a, b, t);
        default:
            return LinearInterpolate(a, b, t);
    }
}

float AnimationCurve::GetStartTime() const {
    return m_keyframes.empty() ? 0.0f : m_keyframes.front().time;
}

float AnimationCurve::GetEndTime() const {
    return m_keyframes.empty() ? 0.0f : m_keyframes.back().time;
}

float AnimationCurve::GetDuration() const {
    return GetEndTime() - GetStartTime();
}

float AnimationCurve::LinearInterpolate(const AnimationKeyframe& a, const AnimationKeyframe& b, float t) const {
    return a.value + (b.value - a.value) * t;
}

float AnimationCurve::HermiteInterpolate(const AnimationKeyframe& a, const AnimationKeyframe& b, float t) const {
    // Hermite interpolation (smooth cubic)
    float t2 = t * t;
    float t3 = t2 * t;
    
    float h1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h2 = -2.0f * t3 + 3.0f * t2;
    float h3 = t3 - 2.0f * t2 + t;
    float h4 = t3 - t2;
    
    float dt = b.time - a.time;
    
    return h1 * a.value + h2 * b.value + h3 * a.outTangent * dt + h4 * b.inTangent * dt;
}

float AnimationCurve::BezierInterpolate(const AnimationKeyframe& a, const AnimationKeyframe& b, float t) const {
    // Cubic Bezier interpolation
    float t2 = t * t;
    float t3 = t2 * t;
    float oneMinusT = 1.0f - t;
    float oneMinusT2 = oneMinusT * oneMinusT;
    float oneMinusT3 = oneMinusT2 * oneMinusT;
    
    // Control points
    float p0 = a.value;
    float p1 = a.value + a.outTangent * (b.time - a.time) / 3.0f;
    float p2 = b.value - b.inTangent * (b.time - a.time) / 3.0f;
    float p3 = b.value;
    
    return oneMinusT3 * p0 + 3.0f * oneMinusT2 * t * p1 + 3.0f * oneMinusT * t2 * p2 + t3 * p3;
}

void AnimationCurve::SortKeyframes() {
    std::sort(m_keyframes.begin(), m_keyframes.end(),
        [](const AnimationKeyframe& a, const AnimationKeyframe& b) {
            return a.time < b.time;
        });
}

int AnimationCurve::FindKeyframeIndex(float time) const {
    for (int i = 0; i < static_cast<int>(m_keyframes.size()) - 1; ++i) {
        if (time >= m_keyframes[i].time && time < m_keyframes[i + 1].time) {
            return i;
        }
    }
    return static_cast<int>(m_keyframes.size()) - 1;
}

} // namespace Animation
} // namespace Engine

