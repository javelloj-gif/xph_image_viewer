#pragma once

#include <memory>
#include <string>
#include <stop_token>

namespace kv {

// A fresh allocation identifies one request, not just a path. UI-thread consumers
// retain it with the item across moves, drop it on removal and consume it on delivery.
// Callbacks also own it, preventing address reuse while an old completion is pending.
using ImageRequestToken = std::shared_ptr<const std::wstring>;

// UI models own this lease; workers/callbacks borrow it weakly, so removal cancels only that request.
using ImageLoadLifetime = std::shared_ptr<const bool>;

// CPU-only cancellation snapshot, safe on workers; never calls into UI or extends consumer lifetime.
struct ImageCancellation {
    std::weak_ptr<const bool> owner;
    std::stop_token shutdown;
    bool scoped = false;

    // An empty lease preserves the legacy unscoped API; an expired nonempty lease cancels work.
    ImageCancellation(std::weak_ptr<const bool> lifetime = {}, std::stop_token stop = {})
        : owner(std::move(lifetime)), shutdown(stop),
          scoped(owner.owner_before(std::weak_ptr<const bool>{}) ||
                 std::weak_ptr<const bool>{}.owner_before(owner)) {}
    // May be checked between WIC operations; cannot interrupt a codec already executing.
    bool IsCancelled() const { return shutdown.stop_requested() || (scoped && owner.expired()); }
};

} // namespace kv
