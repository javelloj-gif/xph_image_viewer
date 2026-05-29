// ============================================================================
// animation.h — Animation System
// Krumassan Image Viewer — Native C++20
//
// Provides:
//   1. Easing functions (matching the Python animate_property behavior)
//   2. PropertyAnimation — animate a float value over time with easing
//   3. AnimationManager — manages multiple named animations with unique IDs
//   4. FrameSequencer — playback controller for animated images (GIF/WebP/APNG)
//
// The animation ID system prevents the timer conflicts documented in
// MANUAL_TESTING.md of the Python version. Each animation has a unique ID,
// and starting a new animation with the same ID cancels the previous one.
// ============================================================================
#pragma once

#include "types.h"

namespace kv {

// ─── Easing Functions ────────────────────────────────────────────────────────
// All easing functions take t in [0, 1] and return a value in [0, 1].
// These match the Python app's easing functions used in animate_property.

namespace Easing {

inline float Linear(float t) { return t; }

inline float EaseInQuad(float t)    { return t * t; }
inline float EaseOutQuad(float t)   { return t * (2.0f - t); }
inline float EaseInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

inline float EaseInCubic(float t)    { return t * t * t; }
inline float EaseOutCubic(float t)   { float u = t - 1.0f; return u * u * u + 1.0f; }
inline float EaseInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t
                    : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

inline float EaseInExpo(float t) {
    return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
}
inline float EaseOutExpo(float t) {
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}
inline float EaseInOutExpo(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return t < 0.5f
        ? 0.5f * std::pow(2.0f, 20.0f * t - 10.0f)
        : 1.0f - 0.5f * std::pow(2.0f, -20.0f * t + 10.0f);
}

inline float EaseOutBack(float t) {
    constexpr float c = 1.70158f;
    float u = t - 1.0f;
    return 1.0f + (c + 1.0f) * u * u * u + c * u * u;
}

inline float EaseInBack(float t) {
    constexpr float c = 1.70158f;
    return (c + 1.0f) * t * t * t - c * t * t;
}

inline float EaseOutElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    return std::pow(2.0f, -10.0f * t) *
           std::sin((t - 0.075f) * (2.0f * std::numbers::pi_v<float>) / 0.3f) + 1.0f;
}

inline float EaseInElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    return -std::pow(2.0f, 10.0f * (t - 1.0f)) *
            std::sin((t - 1.075f) * (2.0f * std::numbers::pi_v<float>) / 0.3f);
}

} // namespace Easing

// ─── Easing Function Type ────────────────────────────────────────────────────

using EasingFunction = float(*)(float);

// ─── Property Animation ─────────────────────────────────────────────────────
// Animates a single float value from start to end over a duration.

struct PropertyAnimation {
    float          startValue   = 0.0f;
    float          endValue     = 1.0f;
    float          currentValue = 0.0f;
    double         duration     = 0.3;    // seconds
    double         elapsed      = 0.0;    // seconds
    EasingFunction easing       = Easing::EaseOutCubic;
    bool           active       = false;
    bool           completed    = false;

    /// Update the animation by deltaTime seconds. Returns true if still active.
    bool Update(double dt) {
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

    /// Reset and start the animation
    void Start(float from, float to, double dur, EasingFunction ease = Easing::EaseOutCubic) {
        startValue   = from;
        endValue     = to;
        currentValue = from;
        duration     = dur;
        elapsed      = 0.0;
        easing       = ease;
        active       = true;
        completed    = false;
    }

    /// Get the normalized progress [0, 1]
    float Progress() const { return static_cast<float>(std::clamp(elapsed / duration, 0.0, 1.0)); }
};

// ─── Animation Manager ──────────────────────────────────────────────────────
// Manages multiple named animations. Starting an animation with an existing
// name replaces the old one (preventing timer conflicts from the Python version).

class AnimationManager {
public:
    /// Update all active animations. Call once per frame.
    void Update(double deltaTime) {
        for (auto it = m_animations.begin(); it != m_animations.end(); ) {
            it->second.Update(deltaTime);
            if (it->second.completed) {
                // Notify completion callback if registered
                auto cbIt = m_completionCallbacks.find(it->first);
                if (cbIt != m_completionCallbacks.end()) {
                    cbIt->second();
                    m_completionCallbacks.erase(cbIt);
                }
                it = m_animations.erase(it);
            } else {
                ++it;
            }
        }
    }

    /// Start a new animation (or replace an existing one with the same name).
    void Start(const std::string& name, float from, float to,
               double duration, EasingFunction easing = Easing::EaseOutCubic,
               std::function<void()> onComplete = nullptr)
    {
        PropertyAnimation anim;
        anim.Start(from, to, duration, easing);
        m_animations[name] = anim;

        if (onComplete) {
            m_completionCallbacks[name] = std::move(onComplete);
        } else {
            m_completionCallbacks.erase(name);
        }
    }

    /// Cancel an animation by name
    void Cancel(const std::string& name) {
        m_animations.erase(name);
        m_completionCallbacks.erase(name);
    }

    /// Check if an animation is currently active
    bool IsActive(const std::string& name) const {
        auto it = m_animations.find(name);
        return it != m_animations.end() && it->second.active;
    }

    /// Get the current value of an animation (or defaultValue if not active)
    float GetValue(const std::string& name, float defaultValue = 0.0f) const {
        auto it = m_animations.find(name);
        return (it != m_animations.end()) ? it->second.currentValue : defaultValue;
    }

    /// Check if any animation is currently running
    bool HasActiveAnimations() const { return !m_animations.empty(); }

    /// Cancel all animations
    void CancelAll() {
        m_animations.clear();
        m_completionCallbacks.clear();
    }

private:
    std::unordered_map<std::string, PropertyAnimation>     m_animations;
    std::unordered_map<std::string, std::function<void()>> m_completionCallbacks;
};

// ─── Frame Sequencer ─────────────────────────────────────────────────────────
// Controls playback of animated images (GIF, animated WebP, APNG).
// Handles frame timing, speed control, pause/resume, and loop counting.

class FrameSequencer {
public:
    FrameSequencer() = default;

    /// Set up the sequencer with frame delays and loop count
    void SetFrames(const std::vector<double>& frameDelaysMs, uint32_t loopCount = 0) {
        m_frameDelays = frameDelaysMs;
        m_loopCount = loopCount;
        m_totalDuration = 0;
        for (double d : m_frameDelays) m_totalDuration += d;
        Reset();
    }

    /// Update the sequencer by deltaTime (seconds). Returns true if frame changed.
    bool Update(double deltaTimeSec) {
        if (m_frameDelays.empty() || m_paused || m_complete) return false;

        double dt = deltaTimeSec * 1000.0 * m_speed; // convert to ms and apply speed
        m_elapsed += dt;

        // Check if we've passed the current frame's duration
        double currentDelay = m_frameDelays[m_currentFrame];
        if (m_elapsed >= currentDelay) {
            m_elapsed -= currentDelay;
            m_currentFrame++;

            if (m_currentFrame >= static_cast<int>(m_frameDelays.size())) {
                m_currentFrame = 0;
                m_loopsCompleted++;

                if (m_loopCount > 0 && m_loopsCompleted >= m_loopCount) {
                    m_currentFrame = static_cast<int>(m_frameDelays.size()) - 1;
                    m_complete = true;
                }
            }
            return true; // frame changed
        }
        return false;
    }

    // ── Playback Control ──────────────────────────────────────────────────
    void Pause()  { m_paused = true; }
    void Resume() { m_paused = false; }
    void TogglePause() { m_paused = !m_paused; }

    void Reset() {
        m_currentFrame = 0;
        m_elapsed = 0;
        m_loopsCompleted = 0;
        m_complete = false;
        m_paused = false;
    }

    /// Set playback speed multiplier (1.0 = normal, 2.0 = double speed)
    void SetSpeed(double speed) { m_speed = std::max(0.1, speed); }
    double GetSpeed() const { return m_speed; }

    // ── Accessors ─────────────────────────────────────────────────────────
    int      GetCurrentFrame()    const { return m_currentFrame; }
    int      GetFrameCount()      const { return static_cast<int>(m_frameDelays.size()); }
    bool     IsPaused()           const { return m_paused; }
    bool     IsComplete()         const { return m_complete; }
    double   GetTotalDuration()   const { return m_totalDuration; }
    uint32_t GetLoopsCompleted()  const { return m_loopsCompleted; }

    /// Get playback progress [0, 1] within the current loop
    double GetProgress() const {
        if (m_totalDuration <= 0) return 0;
        double pos = 0;
        for (int i = 0; i < m_currentFrame; ++i) pos += m_frameDelays[i];
        pos += m_elapsed;
        return std::clamp(pos / m_totalDuration, 0.0, 1.0);
    }

private:
    std::vector<double> m_frameDelays;    // ms per frame
    int                 m_currentFrame = 0;
    double              m_elapsed = 0;     // ms within current frame
    double              m_totalDuration = 0; // total loop duration in ms
    double              m_speed = 1.0;
    uint32_t            m_loopCount = 0;   // 0 = infinite
    uint32_t            m_loopsCompleted = 0;
    bool                m_paused = false;
    bool                m_complete = false;
};

} // namespace kv
