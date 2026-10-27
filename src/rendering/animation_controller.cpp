// ============================================================================
// Class: AnimationController
// Description: AnimationController schedules, updates, and dispatches properties interpolated over time for active UI transitions in Fotassa.
// Responsibility: Manage active UI transitions and prevent timer collision flags.
// ============================================================================
#include "common/pch.h"
#include "animation_controller.h"

namespace kv {

// ─── PropertyAnimation Implementation ───────────────────────────────────────

bool PropertyAnimation::Update(double dt) {
    if (!active || completed) return false;

    elapsed += dt;
    float t = static_cast<float>(std::clamp(elapsed / duration, 0.0, 1.0));
    float easedT = easing(t);
    currentValue = startValue + (endValue - startValue) * easedT;

    if (elapsed >= duration) {
        currentValue = endValue;
        completed = true;
        active = false;
    }
    return active;
}

void PropertyAnimation::Start(float from, float to, double dur, EasingFunction ease) {
    startValue   = from;
    endValue     = to;
    currentValue = from;
    duration     = dur;
    elapsed      = 0.0;
    easing       = ease;
    active       = true;
    completed    = false;
}

float PropertyAnimation::Progress() const {
    return static_cast<float>(std::clamp(elapsed / duration, 0.0, 1.0));
}

// ─── AnimationController Implementation ─────────────────────────────────────

void AnimationController::Update(double deltaTime) {
    std::vector<std::pair<std::string, std::function<void()>>> completedCallbacks;

    for (auto it = m_animations.begin(); it != m_animations.end(); ) {
        it->second.Update(deltaTime);
        if (it->second.completed) {
            auto cbIt = m_completionCallbacks.find(it->first);
            if (cbIt != m_completionCallbacks.end()) {
                completedCallbacks.push_back({ it->first, std::move(cbIt->second) });
                m_completionCallbacks.erase(cbIt);
            }
            it = m_animations.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& pair : completedCallbacks) {
        if (pair.second) {
            pair.second();
        }
    }
}

void AnimationController::Start(const std::string& name, float from, float to,
                                double duration, EasingFunction easing,
                                std::function<void()> onComplete) {
    PropertyAnimation anim;
    anim.Start(from, to, duration, easing);
    m_animations[name] = anim;

    if (onComplete) {
        m_completionCallbacks[name] = std::move(onComplete);
    } else {
        m_completionCallbacks.erase(name);
    }
}

void AnimationController::Cancel(const std::string& name) {
    m_animations.erase(name);
    m_completionCallbacks.erase(name);
}

bool AnimationController::IsActive(const std::string& name) const {
    auto it = m_animations.find(name);
    return it != m_animations.end() && it->second.active;
}

float AnimationController::GetValue(const std::string& name, float defaultValue) const {
    auto it = m_animations.find(name);
    return (it != m_animations.end()) ? it->second.currentValue : defaultValue;
}

bool AnimationController::HasActiveAnimations() const {
    return !m_animations.empty();
}

void AnimationController::CancelAll() {
    m_animations.clear();
    m_completionCallbacks.clear();
}

} // namespace kv
