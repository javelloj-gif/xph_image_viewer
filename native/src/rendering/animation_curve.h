// ============================================================================
// Class: AnimationCurve
// Description: AnimationCurve provides mathematical easing curves to drive smooth UI animations in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Implement easing curves (cubic, expo, quadratic, elastic back, etc.).
// ============================================================================
#pragma once

#include "common/types.h"

namespace kv {

namespace Easing {

float Linear(float t);
float EaseInQuad(float t);
float EaseOutQuad(float t);
float EaseInOutQuad(float t);
float EaseInCubic(float t);
float EaseOutCubic(float t);
float EaseInOutCubic(float t);
float EaseInExpo(float t);
float EaseOutExpo(float t);
float EaseInOutExpo(float t);
float EaseOutBack(float t);
float EaseInBack(float t);
float EaseOutElastic(float t);
float EaseInElastic(float t);

} // namespace Easing

using EasingFunction = float(*)(float);

} // namespace kv
