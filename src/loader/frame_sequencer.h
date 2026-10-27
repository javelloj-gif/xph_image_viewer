// ============================================================================
// Class: FrameSequencer
// Description: FrameSequencer manages frame timing, speed adjustments, and repeat counts for animated images in Fotassa.
// Responsibility: Track active frame indices and elapsed frame delays for GIF/APNG/WebP animations.
// ============================================================================
#pragma once

#include "common/types.h"
#include <vector>

namespace kv {

class FrameSequencer {
public:
    FrameSequencer() = default;
    ~FrameSequencer() = default;

    void SetFrames(const std::vector<double>& frameDelaysMs, uint32_t loopCount = 0);
    bool Update(double deltaTimeSec);

    void Pause();
    void Resume();
    void TogglePause();
    void Reset();

    void SetSpeed(double speed);
    double GetSpeed() const;

    int      GetCurrentFrame()    const;
    int      GetFrameCount()      const;
    bool     IsPaused()           const;
    bool     IsComplete()         const;
    double   GetTotalDuration()   const;
    uint32_t GetLoopsCompleted()  const;
    double   GetProgress()        const;

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
