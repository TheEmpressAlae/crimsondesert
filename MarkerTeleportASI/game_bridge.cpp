#include "game_bridge.h"

namespace marker_teleport {

bool GameBridge::Initialize() {
    // Fail closed until the Crimson Desert 1.16.0 player, destination-marker,
    // safety-context, and optional protection resolvers are independently
    // identified and signature uniqueness is verified against the live image.
    ready_ = false;
    return false;
}

void GameBridge::Shutdown() {
    ready_ = false;
}

bool GameBridge::Ready() const noexcept {
    return ready_;
}

TeleportResult GameBridge::TeleportToMarker(const TeleportRequest&) {
    if (!ready_) {
        return TeleportResult::NotReady;
    }

    // Implementation intentionally withheld until the unresolved interfaces in
    // RE_NOTES.md have stable, current-build signatures and runtime validation.
    return TeleportResult::NotReady;
}

void GameBridge::ServiceProtectionExpiry(std::uint64_t) {
    // No protection state is touched until its pointer and lifecycle are known.
}

}  // namespace marker_teleport
