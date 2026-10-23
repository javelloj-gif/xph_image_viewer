#pragma once

#include <memory>
#include <string>

namespace kv {

// A fresh allocation identifies one request, not just a path. UI-thread consumers
// retain it with the item across moves, drop it on removal and consume it on delivery.
// Callbacks also own it, preventing address reuse while an old completion is pending.
using ImageRequestToken = std::shared_ptr<const std::wstring>;

} // namespace kv
