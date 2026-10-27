// ============================================================================
// Class: FrameSequencer
// Description: FrameSequencer manages frame timing, speed adjustments, and repeat counts for animated images in Fotassa.
// Responsibility: Track active frame indices and elapsed frame delays for GIF/APNG/WebP animations.
// ============================================================================
#include "common/pch.h"
#include "frame_sequencer.h"

namespace kv {

void FrameSequencer::SetFrames(const std::vector<double>& frameDelaysMs, uint32_t loopCount) {
    m_frameDelays = frameDelaysMs;
    m_loopCount = loopCount;
    m_totalDuration = 0;
    for (double d : m_frameDelays) m_totalDuration += d;
    Reset();
}

bool FrameSequencer::Update(double deltaTimeSec) {
    if (m_frameDelays.empty() || m_paused || m_complete) return false;

    double dt = deltaTimeSec * 1000.0 * m_speed; // convert to ms and apply speed
    m_elapsed += dt;

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
        return true;
    }
    return false;
}

void FrameSequencer::Pause()  { m_paused = true; }
void FrameSequencer::Resume() { m_paused = false; }
void FrameSequencer::TogglePause() { m_paused = !m_paused; }

void FrameSequencer::Reset() {
    m_currentFrame = 0;
    m_elapsed = 0;
    m_loopsCompleted = 0;
    m_complete = false;
    m_paused = false;
}

void FrameSequencer::SetSpeed(double speed) { m_speed = std::max(0.1, speed); }
double FrameSequencer::GetSpeed() const { return m_speed; }

int      FrameSequencer::GetCurrentFrame()    const { return m_currentFrame; }
int      FrameSequencer::GetFrameCount()      const { return static_cast<int>(m_frameDelays.size()); }
bool     FrameSequencer::IsPaused()           const { return m_paused; }
bool     FrameSequencer::IsComplete()         const { return m_complete; }
double   FrameSequencer::GetTotalDuration()   const { return m_totalDuration; }
uint32_t FrameSequencer::GetLoopsCompleted()  const { return m_loopsCompleted; }

double FrameSequencer::GetProgress() const {
    if (m_totalDuration <= 0) return 0;
    double pos = 0;
    for (int i = 0; i < m_currentFrame; ++i) pos += m_frameDelays[i];
    pos += m_elapsed;
    return std::clamp(pos / m_totalDuration, 0.0, 1.0);
}

} // namespace kv
