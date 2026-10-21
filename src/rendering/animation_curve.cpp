// ============================================================================
// Class: AnimationCurve
// Description: AnimationCurve provides mathematical easing curves to drive smooth UI animations in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Implement easing curves (cubic, expo, quadratic, elastic back, etc.).
// ============================================================================
#include "common/pch.h"
#include "animation_curve.h"

namespace kv {

namespace Easing {

float Linear(float t) { return t; }

float EaseInQuad(float t)    { return t * t; }
float EaseOutQuad(float t)   { return t * (2.0f - t); }
float EaseInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

float EaseInCubic(float t)    { return t * t * t; }
float EaseOutCubic(float t)   { float u = t - 1.0f; return u * u * u + 1.0f; }
float EaseInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t
                    : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

float EaseInExpo(float t) {
    return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
}
float EaseOutExpo(float t) {
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}
float EaseInOutExpo(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return t < 0.5f
        ? 0.5f * std::pow(2.0f, 20.0f * t - 10.0f)
        : 1.0f - 0.5f * std::pow(2.0f, -20.0f * t + 10.0f);
}

float EaseOutBack(float t) {
    constexpr float c = 1.70158f;
    float u = t - 1.0f;
    return 1.0f + (c + 1.0f) * u * u * u + c * u * u;
}

float EaseInBack(float t) {
    constexpr float c = 1.70158f;
    return (c + 1.0f) * t * t * t - c * t * t;
}

float EaseOutElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    return std::pow(2.0f, -10.0f * t) *
           std::sin((t - 0.075f) * (2.0f * std::numbers::pi_v<float>) / 0.3f) + 1.0f;
}

float EaseInElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    return -std::pow(2.0f, 10.0f * (t - 1.0f)) *
            std::sin((t - 1.075f) * (2.0f * std::numbers::pi_v<float>) / 0.3f);
}

} // namespace Easing

} // namespace kv
