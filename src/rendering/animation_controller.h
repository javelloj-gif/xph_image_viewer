// ============================================================================
// Class: AnimationController
// Description: AnimationController schedules, updates, and dispatches properties interpolated over time for active UI transitions in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Manage active UI transitions and prevent timer collision flags.
// ============================================================================
#pragma once

#include "common/types.h"
#include "animation_curve.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace kv {

struct PropertyAnimation {
    float          startValue   = 0.0f;
    float          endValue     = 1.0f;
    float          currentValue = 0.0f;
    double         duration     = 0.3;    // seconds
    double         elapsed      = 0.0;    // seconds
    EasingFunction easing       = Easing::EaseOutCubic;
    bool           active       = false;
    bool           completed    = false;

    bool Update(double dt);
    void Start(float from, float to, double dur, EasingFunction ease = Easing::EaseOutCubic);
    float Progress() const;
};

class AnimationController {
public:
    AnimationController() = default;
    ~AnimationController() = default;

    void Update(double deltaTime);
    void Start(const std::string& name, float from, float to,
               double duration, EasingFunction easing = Easing::EaseOutCubic,
               std::function<void()> onComplete = nullptr);
    void Cancel(const std::string& name);
    bool IsActive(const std::string& name) const;
    float GetValue(const std::string& name, float defaultValue = 0.0f) const;
    bool HasActiveAnimations() const;
    void CancelAll();

private:
    std::unordered_map<std::string, PropertyAnimation>     m_animations;
    std::unordered_map<std::string, std::function<void()>> m_completionCallbacks;
};

} // namespace kv
